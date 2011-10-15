//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "DiskCache.h"

Define_Module(DiskCache);

DiskCache::pr_t::pr_t(int fid, long st, long e, bool dirty, struct pr_t * n, struct pr_t * p):
							fileid(fid),start(st),end(e),PG_dirty(dirty),next(n),prev(p){
	_count = 1; // Increment by 1 automatically.
	PG_writeback = false;
	PG_locked = PG_dirty;
	PG_referenced = true;
	modified = PG_dirty;
	PG_lru = false;
	lru_next = NULL;
	lru_prev = NULL;
	writeout_list = NULL;
}

DiskCache::pr_t::pr_t(pr_t * oldpr){
	fileid = oldpr->fileid;
	start = oldpr->start;
	end = oldpr->end;
	PG_dirty = oldpr->PG_dirty;
	next = oldpr->next;
	prev = oldpr->prev;
	_count = oldpr->_count;
	PG_writeback = oldpr->PG_writeback;
	PG_locked = oldpr->PG_locked;
	PG_referenced = oldpr->PG_referenced;
	modified = oldpr->modified;
	PG_lru = oldpr->PG_lru;
	lru_next = oldpr->lru_next;
	lru_prev = oldpr->lru_prev;
	writeout_list = NULL;
}

bool DiskCache::pr_t::attrcomp(pr_t* comp1, pr_t* comp2){
	return (comp1->fileid == comp2->fileid &&
			comp1->_count == comp2->_count &&
			comp1->PG_dirty == comp2->PG_dirty &&
			comp1->PG_writeback == comp2->PG_writeback &&
			comp1->PG_locked == comp2->PG_locked &&
			comp1->PG_referenced == comp2->PG_referenced &&
			comp1->modified == comp2->modified &&
			comp1->PG_lru == comp2->PG_lru
			);
}

DiskCache::file_t::file_t(int i, bool o_s, bool o_d, pr_t * pr_h):
		id(i), o_sync(o_s), o_direct(o_d), pr_head(pr_h){
	f_ra = new file_ra_state();
}

DiskCache::file_ra_state::file_ra_state(){
	ra_pages = file_ra_state::POSIX_FADV_NORMAL;
	RA_FLAG_MISS = false;
	RA_FLAG_INCACHE = false;
	cache_hit = 0;
	prev_page = -1;
	start = 0;
	size = 0;
	ahead_start = 0;
	ahead_size = 0;
}

void DiskCache::initialize()
{
	// receive parameters from ini file.
	total_pages = par("total_pages").longValue();
	free_pages = total_pages;
	dirty_pages = 0;
	cache_r_speed = par("cache_r_speed").doubleValue();
	cache_w_speed = par("cache_w_speed").doubleValue();
	dirty_ratio = par("dirty_ratio").doubleValue();
	dirty_background_ratio = par("dirty_background_ratio").doubleValue();
	dirty_threshold = dirty_ratio * total_pages;
	dirty_background_threshold = dirty_background_ratio * total_pages;
	writeout_batch = par("writeout_batch").longValue();

	cacheAccess = new PageRequest("cacheAccess");
	cacheAccess->setKind(SELF_EVENT);

	reqQ = new list<PageRequest *>();
	prs = new map<PageRequest *, pr_t *>();
	files = new map<int, file_t *>();
	lru_list = NULL;

	num_files = 0;
}

void DiskCache::handleMessage(cMessage *msg)
{
    switch(msg->getKind()){
    case PAGE_REQ:
#ifdef DEBUG
    	printf("Receive PAGE_REQ.\n");
#endif
    	handlePageReq((PageRequest *)msg);
    	break;
    case SELF_EVENT:
#ifdef DEBUG
    	printf("Receive SELF_EVENT.\n");
#endif
    	handleCacheAccessResp((PageRequest *)msg);
    	break;
    case BLK_RESP:
#ifdef DEBUG
    	printf("Receive BLK_RESP.\n");
#endif
    	handleDiskAccessResp((PageRequest *)msg);
    	break;
    }
}

void DiskCache::handlePageReq(PageRequest * req){
#ifdef DEBUG
	printf("DiskCache: handlePageReq.\n");
	fflush(stdout);
#endif
	struct file_t * file;
	map<int, file_t *>::iterator it = files->find(req->getFileId());
	if(it == files->end()){ // A new file!
		file = new file_t(req->getFileId(), req->getSYNC(), req->getDIRECT(), NULL);
		files->insert(std::pair<int, file_t *>(file->id, file));
		fileIDs[num_files ++] = file->id;
	}else{
		file = it->second;
	}
	req->setPending(false);
	reqQ->push_back(req);
	prs->insert(std::pair<PageRequest *, pr_t *>
		(req, new pr_t(req->getFileId(), req->getPageStart(), req->getPageEnd(), !req->getRead(), NULL, NULL)));
	accessCache();
}


// Read or write on the cache.
// Read cache -> no increase in cache size.
// Write cache -> increase the cache size. Flags do change.
void DiskCache::handleCacheAccessResp(PageRequest * resp){
	// Update the requested pr. If the request is finished, send it back.
	long id = resp->getID();
	pr_t * pr;
	list<PageRequest *>::iterator it;
	map<PageRequest *, pr_t *>::iterator it2;
	for(it = reqQ->begin(); it != reqQ->end(); it ++){
		if((*it)->getID() == id){
			(*it)->setPending(false); // it's not pending any more.

			it2 = prs->find(*it);
			if(it2 == prs->end()){
				fprintf(stderr, "[ERROR] DiskRequest: can not find request #%ld in map prs.\n", (*it)->getID());
			}
			pr = it2->second;

			if(resp->getPageEnd() == pr->end){ // done for this page request
#ifdef DEBUG
				printf("resp->getPageEnd() == pr->end == %ld.\n", resp->getPageEnd());
				fflush(stdout);
#endif
				(*it)->setName("PAGE_RESP");
				(*it)->setKind(PAGE_RESP);
				send(*it, "lfs$o");
				free(pr);

				reqQ->erase(it);
				prs->erase(it2); // Remove the position from the map.
			}else{ // Partially done.
#ifdef DEBUG
				printf("pr->start = resp->getPageEnd() == %ld. pr->end == %ld. \n", resp->getPageEnd(), pr->end);
				fflush(stdout);
#endif
				pr->start = resp->getPageEnd();
			}
			break;
		}
	}

	if(it == reqQ->end()){
		fprintf(stderr, "[ERROR] DiskCache: can not find record in reQ when handling cache access response.\n");
	}

	// update the cache.
	struct pr_t * resppr = new pr_t(resp->getFileId(), resp->getPageStart(), resp->getPageEnd(), !resp->getRead(), NULL, NULL);
	walkCache_update(resp->getFileId(),resppr, false, false);// This operation releases the locks.
	free(resppr);
	accessCache();
}

// It might be a read from disk, or a write to disk.
void DiskCache::handleDiskAccessResp(PageRequest * resp){
	long id = resp->getID();
	list<PageRequest *>::iterator it;
	map<PageRequest *, pr_t *>::iterator it2;
	for(it = reqQ->begin(); it != reqQ->end(); it ++){ // Find the request from the request ID.
		if((*it)->getID() == id){
			(*it)->setPending(false); // it's not pending any more.
			break;
		}
	}

	// Update the cache.
	it2 = prs->find((*it)); // Find the pr from the request.
	if(it2 == prs->end()){
		fprintf(stderr, "[ERROR] DiskCache: handleDiskAccessResp - can not find mapping from request #%ld.\n", (*it)->getID());
		fflush(stderr);
		return;
	}

	pr_t * pr = it2->second;

	if(pr->writeout_list != NULL){ // If not NULL, it must be the writeout. Update the writeout page range.
		if(resp->getPageStart() != pr->writeout_list->start || resp->getPageEnd() > pr->writeout_list->end){
			fprintf(stderr, "[ERROR] DiskCache: handleDiskAccessResp - the response range [%ld %ld]"
					" does not match the expected writeout range.\n", resp->getPageStart(), resp->getPageEnd());
			fflush(stderr);
			return;
		}

		if(resp->getPageEnd() == pr->writeout_list->end){
			pr_t * tmp = pr->writeout_list->next;
			free(pr->writeout_list);
			pr->writeout_list = tmp;
		}else{ // resp->getPageEnd() < pr->writeout_list->end
			pr->writeout_list->end = resp->getPageEnd();
		}
	}

	struct pr_t * resppr = new pr_t(resp->getFileId(), resp->getPageStart(), resp->getPageEnd(), !resp->getRead(), NULL, NULL);
	walkCache_update(resp->getFileId(),resppr, false, true);// This operation releases the locks.
	free(resppr);
	delete resp;
//	if(it != reqQ->end() && (*it)->getRead()){ // This means it is triggered by the request. We need to further read the content from cache.
//		scheduleCacheAccess((*it)->getID(), (*it)->getFileId(), resp->getPageStart(), resp->getPageEnd(), true);
//	}
	accessCache(); // The same content in cache will be accessed later.
}

// Checks if cache is busy. If not, read/write cache.
void DiskCache::accessCache(){
#ifdef DEBUG
	printf("DiskCache: accessCache.\n");
	fflush(stdout);
#endif
	if(cacheAccess->isScheduled()) // Cache is busy.
		return;
	PageRequest * req;
	for(list<PageRequest *>::iterator it = reqQ->begin(); it != reqQ->end(); it ++){
		if(cacheAccess->isScheduled()) // Cache is busy.
			return;
		req = (PageRequest *)(*it);
		if(!req->getPending()){
			req->setPending(true);
			if(req->getRead())
				readCache(req);
			else
				writeCache(req);
		}
	}
#ifdef DEBUG
	printf("DiskCache: accessCache done.\n");
	fflush(stdout);
#endif
}

// writeCache and readCache:
// find the first access that should be conducted on cache/disk in request.
// set request pending, get the right access size.
// Important notice: writeCache and readCache do not change the current cache.
void DiskCache::writeCache(PageRequest * req){
#ifdef DEBUG
	printf("DiskCache: Write cache. ID:%ld ", req->getID());
	fflush(stdout);
#endif
	map<PageRequest *, pr_t *>::iterator it1 = prs->find(req);
	if(it1 == prs->end()){
		fprintf(stderr, "[ERROR] DiskCache: can not find the page-range from map for PageRequest #%ld.\n", req->getID());
		return;
	}
	pr_t * rpr = it1->second; // Requested pr.
	if(rpr->writeout_list != NULL){
#ifdef DEBUG
	printf("writeback FID:%d [%ld,%ld]\n", rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end);
	fflush(stdout);
#endif
		scheduleDiskAccess(req->getID(), rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end, false);
		return;
	}
	long start = rpr->start;
	long end = rpr->end;
	end = start + getAccessSize(req->getFileId(),start,end,false,true);
	int dirty_increase = walkCache_countMissing(req->getFileId(), start, end);
	if(dirty_pages + dirty_increase >= dirty_threshold){
		rpr->writeout_list = startWriteout(req->getFileId(), writeout_batch);
		if(rpr->writeout_list == NULL){
			fprintf(stderr, "[ERROR] drity_ratio achieved, but cannot write back any pages.\n");
			fflush(stderr);
		}
#ifdef DEBUG
	printf("writeback FID:%d [%ld,%ld]\n", rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end);
	fflush(stdout);
#endif
		scheduleDiskAccess(req->getID(), rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end, false);
	}else{
#ifdef DEBUG
	printf("FID:%d [%ld,%ld]\n", req->getFileId(), start, end);
	fflush(stdout);
#endif
		scheduleCacheAccess(req->getID(), req->getFileId(), start, end, false);
	}
}

void DiskCache::readCache(PageRequest * req){
	map<PageRequest *, pr_t *>::iterator it1 = prs->find(req);

#ifdef DEBUG
	printf("DiskCache: readCache. req: [%ld,%ld]\n",req->getPageStart(), req->getPageEnd());
	fflush(stdout);
#endif

	if(it1 == prs->end()){
		fprintf(stderr, "[ERROR] DiskCache: can not find the page-range from map for PageRequest #%ld.\n", req->getID());
		return;
	}
	pr_t * rpr = it1->second; // Requested pr.

	map<int, file_t *>::iterator it2 = files->find(req->getFileId());
	if(it2 == files->end()){
		fprintf(stderr, "[ERROR] DiskCache-rc: can not find the file from map for file ID == #%d.\n", req->getFileId());
		fflush(stderr);
		return;
	}
	pr_t * cpr = it2->second->pr_head; // Cached pr.

	long start = rpr->start;
	long end = rpr->end;

#ifdef DEBUG
	printf("DiskCache: readCache. rpr [%ld,%ld]\n",start, end);
	fflush(stdout);
#endif

	while(cpr != NULL){
		if(!(cpr->start >= rpr->end || cpr->end <= rpr->start)){ // They intersect
			// Get the higher start - intersection starts
			if(rpr->start < cpr->start){ // beginning not cached.
				if(rpr->end < cpr->start)
					end = rpr->end;
				else
					end = cpr->start;
				end = start + getAccessSize(req->getFileId(), start, end, true, false);
				scheduleDiskAccess(req->getID(), req->getFileId(), start, end, true);
			}else{ // beginning cached.
				if(cpr->end < rpr->end)
					end = cpr->end;
				else
					end = rpr->end;
				end = start + getAccessSize(req->getFileId(), start, end, true, true);
				scheduleCacheAccess(req->getID(), req->getFileId(), start, end, true);
			}
			return;
		}else if(cpr->end <= rpr->start){ // Do not intersect.
			cpr = cpr->next;
		}else{ // rpr->end < cpr->start
			// The requested pages do not exist in cache! - the requested ones fall into a hole in the cached ones.
			end = start + getAccessSize(req->getFileId(), start, end, true, false);
			scheduleDiskAccess(req->getID(), req->getFileId(), start, end, true);
			return;
		}
	}
	// The requested pages do not exist in cache! - all cached ones are lower than the requested one.
	end = start + getAccessSize(req->getFileId(), start, end, true, false);
	scheduleDiskAccess(req->getID(), req->getFileId(), start, end, true);
	return;
}

// Very important if you have a read-ahead implementation.
int DiskCache::getAccessSize(int fid, long start, long end, bool read, bool incache){
	/*
	if(!read && incache){
		// Check if the write back event will be invoked by this operation.
		long max_dirty = total_pages * dirty_ratio;
		long threshold = max_dirty - dirty_pages + start;
		if(end > threshold)
			end = threshold;
	}
	*/
	// Currently we just limit it to 32 pages.
	if(end - start > 32)
		return 32;
	else
		return end - start;
}

// schedulDiskAccess and scheduleCacheAccess:
// Change the cache. Change global variables here.
void DiskCache::scheduleDiskAccess(long id, int fid, long start, long end, bool read){
	int ra_pages = files->find(fid)->second->f_ra->ra_pages;
	PageRequest * req = new PageRequest("BLK_REQ");
	req->setID(id);
	req->setKind(BLK_REQ);
	req->setFileId(fid);
	req->setPageStart(start);
	if(end - start > ra_pages)
		end = start + ra_pages;
	req->setPageEnd(end);
	req->setRead(read);
	send(req, "lfs$o");
#ifdef DEBUG
	printf("scheduleDiskAccess\n");
	fflush(stdout);
#endif
	cacheAccessPreop(fid, start, end, read, true);
}

void DiskCache::scheduleCacheAccess(long id, int fid, long start, long end, bool read){
	// No read-ahead
	if(end <= start){
		fprintf(stderr, "[ERROR] DiskCache: cacheAcess end page %ld <= start page %ld.\n", end, start);
		return;
	}
	if(cacheAccess->isScheduled()){
		fprintf(stderr, "[ERROR] DiskCache: cacheAcess message busy.\n");
		return;
	}

	cacheAccess->setID(id);
	cacheAccess->setFileId(fid);
	cacheAccess->setPageStart(start);
	cacheAccess->setPageEnd(end);
	cacheAccess->setRead(read);
	double cache_acc_speed;
	if(read)
		cache_acc_speed = cache_r_speed;
	else
		cache_acc_speed = cache_w_speed;
	scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + (end - start) * cache_acc_speed), cacheAccess);
#ifdef DEBUG
	printf("scheduleCacheAccess\n");
	fflush(stdout);
#endif
	cacheAccessPreop(fid, start, end, read, false);
}


void DiskCache::cacheAccessPreop(int fid, long start, long end, bool read, bool disk){
	struct pr_t * lockpr = new pr_t(fid, start, end, !read, NULL, NULL);
	walkCache_update(fid, lockpr, true, disk);
	free(lockpr);
#ifdef DEBUG
	printf("cacheAccessPreop done.\n");
	fflush(stdout);
#endif
}


/* Walk the cache: update pages. Global sizes are changed outside before preops.
 * @Parameters preop: the operation to be done before the actual operation.
 * Must increase cache:
 * preop && disk && read: before reading data from disk. Set PG_lock, set counter = 1. free_pages change
 * May increase cache:
 * preop && cache && write: before writing data to cache. Set PG_lock, PG_dirty, increase counter. dirty_pages change
 * Must not increase cache:
 * preop && disk && write: before writing data to disk. Set PG_writeback, increase counter. dirty_pages change
 * preop && cache && read: before reading data from cache. increase the counter.
 * !preop && disk && write: after writing data to disk. Clear PG_writeback, PG_dirty, decrease counter.
 * !preop && disk && read: after reading data from disk. Clear PG_lock, Decrease the counter.
 * !preop && cache && write: after writing data to cache. Clear PG_lock, Decrease the counter.
 * !preop && cache && read: after reading data from cache. Decrease the counter.
 */

void DiskCache::walkCache_update(const int fid, pr_t * addp, const bool preop, const bool disk){
	struct pr_t * precachedp, * cachedp, * tmp, * tmp2;
	// Set cachedp: the pointer to the cached page-range list of the file.
	map<int, file_t *>::iterator it = files->find(fid);
	if(it == files->end()){
		fprintf(stderr, "[ERROR] DiskCache-wcu: can not find the file from map for file ID == #%d.\n", fid);
		fflush(stderr);
		return;
	}
	file_t * file = it->second;
	cachedp = file->pr_head;
	precachedp = NULL;
//	bool read = !cachedp->PG_dirty;
	bool read = !addp->PG_dirty;

	addp->_count = 1; // Enforce the correct parameter.
	addp->prev = NULL;
	addp->next = NULL;

	// addp->start: {, addp->end: }, precachedp/cachedp->start: [, precachedp/cachedp->end:].
	// Have data, no marks area: +++, no cached marks area: ---, uncertain about marks area: ...,
	// uncertain about data but no marks area: ___
	while(true){
		if(cachedp == NULL || addp->start < cachedp->start){ // ...{---[
			// Requested pages not cached.
			// You will enlarge the cache, you need to check free_pages.
			// If there are not enough free pages, you need to reclaim some by calling the lru_free_pages function.
			if((!preop) || (preop && disk && !read) || (preop && !disk && read)){
				fprintf(stderr, "[ERROR] DiskCache: cache increase operation not permitted!"
						"preop=%d, disk=%d, read=%d. [%ld %ld]\n", preop, disk, read, addp->start, addp->end);
				fflush(stderr);
				return;
			}

			// Initialize the newly added pr.
			tmp = new pr_t(addp);
			tmp->PG_locked = true;
			if(!read) // a write.
				tmp->PG_dirty = true;
			// Update links.
			if(cachedp != NULL){
				tmp->next = cachedp;
				cachedp->prev = tmp;
			}
			if(precachedp != NULL){
				tmp->prev = precachedp;
				precachedp->next = tmp;
			}else{
				file->pr_head = tmp; // No element before tmp, set the tmp as the head.
			}


			if(cachedp == NULL || addp->end <= cachedp->start){
				// ...{+++} 	--> ...[+tmp+]
				// ...{+++}___[ --> [+tmp+]___[
//				if(precachedp == NULL)
//					file->pr_head = tmp; // No element previously, set the head.
				addp = NULL; // signal to jump out.
			}else{
				// ...{+++[ --> ...[+tmp+][
				tmp->end = cachedp->start;
				addp->start = cachedp->start; // Update addp to move forward.
//				if(precachedp == NULL)
//					file->pr_head = tmp;
			}

			// Update global variables.
			if(free_pages < (tmp->end - tmp->start)){
				lru_free_pages((tmp->end - tmp->start) > DEFAULT_PAGE_FREE_BATCH ?
						(tmp->end - tmp->start) : DEFAULT_PAGE_FREE_BATCH);
			}

			free_pages -= tmp->end - tmp->start;
			if(!read) // Write to cache.
				dirty_pages += tmp->end - tmp->start;

			lru_push(tmp); // Update LRU list. Totally new.

			if(addp == NULL)
				break;

			precachedp = tmp;
		}

		// Requested pages cached.
		// You need to care about the existing cache status.
		if(addp->start < cachedp->end){ // [___{---]...
			if(preop && disk && read){
				fprintf(stderr, "[ERROR] DiskCache: read the pages already in cache from disk. %d, %d, %d. [%ld %ld]\n",
						preop, disk, read, addp->start, addp->end);
				fflush(stderr);
				printf("addp->start = %ld, end = %ld, cachedp->start = %ld, cachedp->end = %ld, preop = %d, disk = %d, read = %d\n",
						addp->start, addp->end, cachedp->start, cachedp->end, preop, disk, read);
				printCache(fid);
				return;
			}

			// Initialize the newly added pr.
			tmp = new pr_t(addp);
			if(!disk && !read){ // writing to cache.
				tmp->PG_dirty = true;
				if(preop){ // before *
					tmp->PG_locked = true;
				}else{ // after *
					tmp->PG_locked = false;
				}
			}

			if(disk && !read){ // writing to disk
				if(tmp->PG_dirty == false){
					fprintf(stderr, "[ERROR] DiskCache: writing back pages which are not dirty. [%ld %ld]\n",
							addp->start, addp->end);
					fflush(stderr);
					return;
				}
				if(preop){ // before writing to disk
					tmp->PG_writeback = true;
				}else{ // after writing to disk
					tmp->PG_writeback = false;
					tmp->PG_dirty = false;
					tmp->PG_locked = false;
				}
			}
			if(!preop && disk && read){ // after reading from disk
				tmp->PG_locked = false;
			}
			if(!preop){
				tmp->_count --;
			}
			if(!(disk && !read & !preop)){ // Not after write back.
				tmp->PG_dirty |= cachedp->PG_dirty; // Take care of the existing status.
				tmp->modified |= cachedp->modified;
			}
			///////////////////////////////

			if(addp->start == cachedp->start){ // [{---]
				// Update link with precached, or more important, pr_head.
				if(precachedp != NULL){
					tmp->prev = precachedp;
					precachedp->next = tmp;
				}else{ // head
					file->pr_head = tmp;
				}

				if(addp->end < cachedp->end){ // [{++}++] --> [+tmp+][+cachedp+]
					cachedp->start = addp->end;
					tmp->next = cachedp;
					cachedp->prev = tmp;
					addp = NULL; // Signal to break
					if(preop && !read && !disk){ // Write to cache, updates the LRU list order.
						lru_push(tmp);
					}else{
						lru_insert(tmp, cachedp);// Write to disk or read from disk, don't update LRU order.
					}
				}else{ // [{+++]... --> [+tmp+]... put tmp here, free cachedp
					addp->start = cachedp->end; // Update addp to move forward.
					tmp->end = cachedp->end;

					tmp->next = cachedp->next;
					if(tmp->next != NULL)
						tmp->next->prev = tmp;

					if(preop && !read && !disk){ // Write to cache, updates the LRU list order.
						lru_delete(cachedp); // Update the LRU list, because you are deleting the pr.
						lru_push(tmp); // Update the LRU list order.
					}else{
						lru_substitute(tmp, cachedp); // tmp goes to the same position.
					}

					free(cachedp);
					cachedp = tmp;
				}
			}

			else{ // [+++{---]
				if(addp->end < cachedp->end){ // [+++{+++}++] --> [+cachedp+][+tmp+][+tmp2+]
					tmp2 = new pr_t(cachedp);
					cachedp->end = tmp->start;
					tmp2->start = tmp->end;

					tmp2->prev = tmp;
					tmp2->next = cachedp->next;

					tmp->prev = cachedp;
					tmp->next = tmp2;

					cachedp->next = tmp;

					if(tmp2->next != NULL)
						tmp2->next->prev = tmp2;

					if(preop && !read && !disk){ // Write to cache, updates the LRU list order.
						// tmp2 is new, but not accessed, take care of it in the LRU list.
						// ... <-> tmp2 <-> cachedp <-> ...
						lru_insert(tmp2, cachedp);
						lru_push(tmp); // Update the LRU list order.
					}else{
						lru_insert(tmp, cachedp); // tmp <--> cachedp.
						lru_insert(tmp2, tmp); // tmp2 <--> tmp.
					}

					addp = NULL; // Signal to break
				}else{ // [+++{+++]... --> [+cachedp+][+tmp+]...
					tmp->end = cachedp->end;
					cachedp->end = tmp->start;

					tmp->prev = cachedp;
					tmp->next = cachedp->next;
					cachedp->next = tmp;
					if(tmp->next != NULL)
						tmp->next->prev = tmp;

					precachedp = cachedp;
					cachedp = tmp; // Move cachedp to tmp.

					addp->start = cachedp->end; // Update addp to move forward.

					if(preop && !read && !disk){ // Write to cache, updates the LRU list order.
						lru_push(tmp); // Update the LRU list order.
					}else{
						lru_insert(tmp, cachedp); // tmp goes to the same position.
					}
				}
			}

			// Update global variables.
			if(preop && disk && !read){ // Before write back to disk
				dirty_pages -= tmp->end - tmp->start;
			}
			if(addp == NULL) // Receive the break signal.
				break;
		}
		if(addp->start == addp->end) // addp size is zero, done.
			break;
		precachedp = cachedp;
		cachedp = cachedp->next;
	}
//	trimCache(fid);
	checkListHealth();
#ifdef VERBOSE
	printf("Total_pages=%ld. Free_pages=%ld. Dirty_pages=%ld.\n", total_pages, free_pages, dirty_pages);
	fflush(stdout);
#endif
#ifdef DEBUG
	printCache(fid);
	printLRU();
	checkListHealth();
#endif
}




void DiskCache::trimCache(int fid){
	// Merge the adjacent elements with the same attributes.
	pr_t * cachedp = files->find(fid)->second->pr_head;
	pr_t * nextcachedp = NULL;
	while(cachedp && cachedp->next){
		nextcachedp = cachedp->next;
		if(pr_t::attrcomp(cachedp, nextcachedp) && cachedp->end == nextcachedp->start){
			cachedp->end = nextcachedp->end;
			cachedp->next = nextcachedp->next;
			if(nextcachedp->next != NULL){
				nextcachedp->next->prev = cachedp;
			}
			free(nextcachedp);
		}
		cachedp = cachedp->next;
	}
#ifdef DEBUG
	printCache(fid);
#endif
}

int DiskCache::printCache(int fid){
	pr_t * cachedp = files->find(fid)->second->pr_head;
	printf("--------------PRINT CACHE FOR FILE %d---------------\n", fid);
	printf("Total pages: %ld, Free pages: %ld, Dirty pages: %ld\n",
			total_pages, free_pages, dirty_pages);
	int i = 0;
	while(true){
		if(i > 200){
			fprintf(stderr, "page_cache is too long.\n");
			break;
		}
		if(cachedp == NULL)
			break;
		printf("[%ld %ld] C:%d DIRTY:%d WB:%d LCKED:%d REF:%d MOD:%d\n",
				cachedp->start, cachedp->end, cachedp->_count, cachedp->PG_dirty, cachedp->PG_writeback,
				cachedp->PG_locked, cachedp->PG_referenced, cachedp->modified);
		cachedp = cachedp->next;
		i ++;
	}
	printf("--------------PRINT CACHE END----------------------\n");
	fflush(stdout);
	return i;
}

// Walk the cache: count the pages missing in the cache.
int DiskCache::walkCache_countMissing(int fid, long start, long end){
	struct pr_t * cachedp;
	int miss_size = 0;
	// Set cachedp: the pointer to the cached page-range list of the file.
	cachedp = files->find(fid)->second->pr_head;

	// start: {, end: }, precachedp/cachedp->start: [, precachedp/cachedp->end:].
	// Have data, no marks area: +++, no cached marks area: ---, uncertain about marks area: ...,
	// uncertain about data but no marks area: ___
	while(true){
		if(cachedp == NULL || start < cachedp->start){ // ...{---[
			if(cachedp == NULL){
				miss_size += end - start;
				break;
			}
			else if(end <= cachedp->start){ // ...{+++}___[ --> [+++]___[
				miss_size += end - start;
				break;
			}else{ // ...{+++[ --> ...[+++][
				miss_size += cachedp->start - start;
				start = cachedp->start; // Update addp to move forward.
			}
		}
		if(start < cachedp->end){ // [___{---]...
			if(start == cachedp->start){ // [{---]
				if(end < cachedp->end){ // [{++}++] --> [+cachedp+][+tmp+]
					// All covered.
					break;
				}else{ // [{+++]... --> [+cachedp+]... No need to create new
					start = cachedp->end; // Update addp to move forward.
				}
			}else{ // [+++{---]
				if(end < cachedp->end){ // [+++{+++}++] --> [+cachedp+][+tmp+][+tmp2+]
					// All covered.
					break;
				}else{ // [+++{+++]... --> [+cachedp+][+tmp+]...
					start = cachedp->end; // Update addp to move forward.
				}
			}
		}
		if(start == end) // addp size is zero.
			break;
		cachedp = cachedp->next;
	}
	return miss_size;
}

// Starting from the current fid, find out n dirty pages to write back.
// It does not mutate the cache.
DiskCache::pr_t * DiskCache::startWriteout(int fid, int n){
	map<int, file_t *>::iterator it;
	pr_t * wop = NULL;
	pr_t * tmp = NULL;
	pr_t * writeout = NULL;

	bool firsttime = true;
	for(int i = 0; ; i ++){ // For all files, start from the current one.
		if(i == num_files)
			i = 0;

		if(!firsttime && fileIDs[i] == fid) // one round already.
			return writeout; // Not enough pages to writeout.

		if(( fileIDs[i] == fid && firsttime) || !firsttime ){
			firsttime = false;

			it = files->find(fileIDs[i]);
			if(it == files->end()){
				fprintf(stderr, "[ERROR] DiskCache: startWriteout - can not find the file from map for file ID == #%d.\n", fileIDs[i]);
				fflush(stderr);
				return NULL;
			}

			for(pr_t * cachedp = it->second->pr_head;cachedp != NULL; cachedp = cachedp->next){
				if(cachedp->PG_dirty && ! cachedp->PG_writeback && ! cachedp->PG_locked && cachedp->_count == 0){ // choose it to write back.
					tmp = new pr_t(cachedp);
					tmp->prev = NULL;
					tmp->next = NULL; // Initialize.
					if(wop == NULL){
						wop = tmp;
						writeout = wop;
					}else{ // link them.
						wop->next = tmp;
						tmp->prev = wop;
						wop = tmp;
					}

					if(tmp->end - tmp->start >= n){ // Finished.
						tmp->end = tmp->start + n;
						return writeout;
					}else{
						n -= tmp->end - tmp->start;
					}
				}
			}
		}
	}
	return writeout;
}



bool DiskCache::pagetable_delete(pr_t * pr){
#ifdef VERBOSE
	printf("pagetable_delete. FID %d [%ld %ld]\n", pr->fileid, pr->start, pr->end);
	fflush(stdout);
#endif
	if(pr == NULL){
		fprintf(stderr, "[ERROR] DiskCache: pagetable_delete received pr is NULL\n");
		fflush(stderr);
		return false;
	}
	int fid = pr->fileid;
	pr_t * cachedp = files->find(fid)->second->pr_head;
	for(; cachedp != NULL; cachedp = cachedp->next){
		if(cachedp->start == pr->start && cachedp->end == pr->end){
			// Found it.
			if(cachedp->prev != NULL){
				cachedp->prev->next = cachedp->next;
			}else{ // Deleting the head
				files->find(fid)->second->pr_head = cachedp->next;
			}
			if(cachedp->next != NULL){
				cachedp->next->prev = cachedp->prev;
			}else{ // Deleting the end

			}
			return true;
		}
	}
	fprintf(stderr, "[ERROR] DiskCache: pagetable_delete can not find pr (start: %ld end: %ld) in file %d\n.",
			pr->start, pr->end, fid);
	fflush(stderr);
	printCache(fid);
	printLRU();
	while(1);
	return false;
}

// A simple LRU algorithm implementation
// Only mutates the lru_list.
bool DiskCache::lru_delete(pr_t * del_p){
#ifdef VERBOSE
	printf("lru_delete. FID %d [%ld %ld]\n", del_p->fileid, del_p->start, del_p->end);
	fflush(stdout);
#endif
	if(del_p == NULL){
		fprintf(stderr, "[ERROR] DiskCache: lru_delete received del_p is NULL\n");
		fflush(stderr);
		return false;
	}
	for(pr_t * lru_p = lru_list_end; lru_p != NULL; lru_p = lru_p->lru_prev){
		if(lru_p->start == del_p->start
				&& lru_p->end == del_p->end
				&& lru_p->fileid == del_p->fileid){ // As long as del_p and lru_p map to the same range in the same file.
			// Update its neighbors.
			if(lru_p->lru_prev != NULL){
				lru_p->lru_prev->lru_next = lru_p->lru_next;
			}else{ // The one to be deleted is the head.
				lru_list = lru_p->lru_next;
			}

			if(lru_p->lru_next != NULL){
				lru_p->lru_next->lru_prev = lru_p->lru_prev;
			}else{ // The one to be deleted is the end.
				lru_list_end = lru_p->lru_prev;
			}
			return true;
		}
	}
	fprintf(stderr, "[ERROR] DiskCache: lru_delete cannot find the pr fid: %d "
			"(start:%ld end:%ld) from lru_list!\n", del_p->fileid, del_p->start,del_p->end);
	fflush(stderr);
	return false;
}

// Only mutates lru_list.
void DiskCache::lru_push(pr_t * new_p){
	if(new_p == NULL){
		fprintf(stderr, "[ERROR] DiskCache: lru_push the new pr is NULL!\n");
		fflush(stderr);
	}
	new_p->lru_prev = NULL;
	new_p->lru_next = lru_list;
	if(lru_list != NULL)
		lru_list->lru_prev = new_p;
	else // lru_list was empty
		lru_list_end = new_p;
	lru_list = new_p;
}

void DiskCache::lru_insert(pr_t * pr1, pr_t * pr2){
	pr1->lru_prev = pr2->lru_prev;
	pr2->lru_prev = pr1;
	pr1->lru_next = pr2;
	if(pr1->lru_prev != NULL)
		pr1->lru_prev->lru_next = pr2;
	else
		lru_list = pr1;
}

void DiskCache::lru_substitute(pr_t * pr1, pr_t * pr2){
	pr1->lru_next = pr2->lru_next;
	pr1->lru_prev = pr2->lru_prev;
	if(pr1->lru_next == NULL){
		lru_list_end = pr1;
	}else{
		pr1->lru_next->lru_prev = pr1;
	}
	if(pr1->lru_prev == NULL){
		lru_list = pr1;
	}else{
		pr1->lru_prev->lru_next = pr1;
	}
}

// Mutates both the lru_list and page_table.
// Frees n pages, no matter dirty or not.
// currently we only do non-dirty pages.
// Return true if n pages are freed, otherwise, return false.
bool DiskCache::lru_free_pages(int n){
#ifdef VERBOSE
	printf("lru_free_pages.\n");
	fflush(stdout);
#endif
	int prevn = n;
	pr_t * prev_pr;
	for(pr_t * pr = lru_list_end; pr != NULL; pr = prev_pr){
		if(n == 0)
			break;
		prev_pr = pr->lru_prev;
		if(!(pr->PG_dirty || pr->PG_locked || pr->_count)){ // Just a read.
			if(pr->end - pr->start <= n){
				n -= pr->end - pr->start;
				// delete it
				pagetable_delete(pr);
				lru_delete(pr);
				free(pr);
			}else{ // Free part of it.
				pr->start = pr->start + n;
				n = 0;
			}
		}
	}
	free_pages += prevn - n;
	if(n == 0)
		return true;
	else
		return false;
}

bool DiskCache::lru_free_pages(){
	return lru_free_pages(DEFAULT_PAGE_FREE_BATCH);
}


int DiskCache::printLRU(){
	int i = 0;
	pr_t * cachedp = lru_list;
	FILE * fp = fopen("debugoutput", "w+");
	printf("--------------PRINT LRU_LIST -------------------------\n");
	while(true){
		if(cachedp == NULL)
			break;
		fprintf(fp, "[%ld %ld] FID: %d C:%d DIRTY:%d WB:%d LCKED:%d REF:%d MOD:%d\n",
				cachedp->start, cachedp->end, cachedp->fileid, cachedp->_count, cachedp->PG_dirty, cachedp->PG_writeback,
				cachedp->PG_locked, cachedp->PG_referenced, cachedp->modified);
		cachedp = cachedp->lru_next;
		i ++;
	}
	printf("--------------PRINT LRU_LIST END----------------------\n");
	fflush(stdout);
	return i;
}

bool DiskCache::checkListHealth(){
	pr_t * pr;
	int total = 0;
	long pages = 0;
	for(int i = 0; i < num_files; i ++){
		pr = files->find(fileIDs[i])->second->pr_head;
		while(pr != NULL){
			if(pr->prev != NULL && pr->prev->next != pr){
				fprintf(stderr, "[ERROR] checkListHealth: (file %d)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
						fileIDs[i], pr->prev->start, pr->prev->end, pr->start, pr->end);
				fflush(stderr);
				return false;
			}
			if(pr->next != NULL && pr->next->prev != pr){
				fprintf(stderr, "[ERROR] checkListHealth: (file %d)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
						fileIDs[i], pr->start, pr->end, pr->next->start, pr->next->end);
				fflush(stderr);
				return false;
			}
			pages += pr->end - pr->start;
			total ++;
			pr = pr->next;
		}
	}
	pr = lru_list;
	while(pr != NULL){
		if(pr->lru_prev != NULL && pr->lru_prev->lru_next != pr){
			fprintf(stderr, "[ERROR] checkListHealth: (lru)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
					pr->lru_prev->start, pr->lru_prev->end, pr->start, pr->end);
			fflush(stderr);
			return false;
		}
		if(pr->lru_next != NULL && pr->lru_next->lru_prev != pr){
			fprintf(stderr, "[ERROR] checkListHealth: (lru)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
					pr->start, pr->end, pr->lru_next->start, pr->lru_next->end);
			fflush(stderr);
			return false;
		}
		pr = pr->lru_next;
		total --;
	}
	if(total != 0){
		fprintf(stderr, "[ERROR] DiskCache: total pagetable pr != total lru pr.\n");
		fflush(stderr);
		for(int i = 0; i < num_files; i ++){
			printCache(fileIDs[i]);
		}
		printLRU();
	}
	return true;
}

void DiskCache::finish(){
	cancelAndDelete(cacheAccess);
}
