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

int DiskCache::idInit = 0;
// Register the internal event types:
// CACHE_ACCESS: cache access notifier
// DIRTY_PAGE_EXPIRE: background write back notifier

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
	readahead_list = NULL;
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
	readahead_list = NULL;
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
			comp1->PG_lru == comp2->PG_lru);
}

DiskCache::file_t::file_t(int i, bool o_s, bool o_d, pr_t * pr_h):
		id(i), o_sync(o_s), o_direct(o_d), pr_head(pr_h){
	pr_last = NULL;
	f_ra = new file_ra_state();
}

DiskCache::file_ra_state::file_ra_state(){
	ra_pages = file_ra_state::POSIX_FADV_SEQUENTIAL;
	RA_FLAG_MISS = false;
	RA_FLAG_INCACHE = false;
	cache_hit = 0;
	prev_page = -1; // Record it is never accessed before.
	start = 0;
	size = 0;
	ahead_start = 0;
	ahead_size = 0;
	start_IO_ahead_window = false;
	started_ra_range = 0;
}

void DiskCache::file_ra_state::page_cache_readahead(long s, int length){ // Implemented according to UTK(V3) page 647.
	if(size == -1 || RA_FLAG_INCACHE){ // Read-ahead permanently disabled, or file already in cache?
		started_ra_range = 0;
		return;
	}

	if(s == 0 && prev_page == -1){ // First access to file, and first requested page at offset 0?
		// Set the current window starting from the first required page and length.
		int tmp = length - 1; // ------------ RA, Config
		size = 2;
		while(tmp >>= 1 != 0)size *= 2;
		if(size > ra_pages) // Upper bound of the current window size.
			size = ra_pages;
#ifdef DISKCACHE_RA_DEBUG
		cout << "DiskCache-RA: first read && offset 0. length=" << length << ", size=" << size << endl;
#endif
		prev_page = length - 1;
		return;
	}

	if(s == prev_page && length == 1) // Access to the same page as in the previous invocation and just one page required?
		return;

	if(s == prev_page + 1){ // Sequential access?
#ifdef DISKCACHE_RA_DEBUG
			cout << "DiskCache-RA: sequential access. s=" << s << "." << endl;
#endif
		if(size == -1){ // Read-ahead temporarily disabled?
			// Set the current window starting from the first required page and length.
			start = s;
			int tmp = length - 1; // ------------ RA, Config
			size = 2;
			while(tmp >>= 1 != 0)size *= 2;
			if(size > ra_pages) // Upper bound of the current window size.
				size = ra_pages;
#ifdef DISKCACHE_RA_DEBUG
			cout << "DiskCache-RA: sequential, ra was disabled. length=" << length << ", size=" << size << endl;
#endif
		}else{ // Read-ahead is not disabled.
			if(ahead_size == 0){ // Empty ahead window?
				start_IO_ahead_window = true;
				ahead_start = start + size; // Create a new ahead window following the current window.
				if(RA_FLAG_MISS){ // The size of the new ahead window depends on the RA_FLAG_MISS flag.
					ahead_size = (size - 2) > 4 ? (size - 2) : 4;
				}else{
					ahead_size = (2 * size) > MAX_RA ? MAX_RA : (2 * size);
				}
#ifdef DISKCACHE_RA_DEBUG
				cout << "DiskCache-RA: Empty ahead window. ahead_start=" << ahead_start << ", ahead_size=" << ahead_size << endl;
#endif
			}

			if(s + length > ahead_start){ // Some of the required page in the ahead window?
				start_IO_ahead_window = true;
				// Replace the current window with the ahead window.
				start = ahead_start;
				size = ahead_size;
				// Create a new ahead window.
				ahead_start = start + size; // Create a new ahead window following the current window.
				if(RA_FLAG_MISS){ // The size of the new ahead window depends on the RA_FLAG_MISS flag.
					ahead_size = (size - 2) > 4 ? size - 2 : 4;
				}else{
					ahead_size = (2 * size) > MAX_RA ? MAX_RA : (2 * size);
				}
#ifdef DISKCACHE_RA_DEBUG
				cout << "DiskCache-RA: Some of the required page in the ahead window. start=" << start << ", size=" << size << ". ahead_start="
						<< ahead_start << ", ahead_size=" << ahead_size << endl;
#endif
			}

		}
	}else{ // Random access.
#ifdef DISKCACHE_RA_DEBUG
		cout << "DiskCache-RA: Random access" << endl;
#endif
		start = s; // Reset the current window and the ahead window. Start I/O on the required pages.
		size = -1; // Temporarily disable read ahead.
		ahead_start = 0;
		ahead_size = 0;
	}
	prev_page = s + length - 1;
}


// Note that the pr->end may also be changed to include the RA range.
void DiskCache::file_ra_state::setReadahead(pr_t * pr){
/*
	if(pr->end - pr->start < RA_THRESHOLD) // No read-ahead is triggered.
		return;

	map<int, file_t *>::iterator it = files->find(pr->fileid);
	if(it == files->end()){
		PrintError::print("DiskCache - getReadahead", "cannot get the file object with ID", pr->fileid);
		return;
	}

	int ra_pages = it->second->f_ra->ra_pages;
	int total = ra_pages + 32 * ((pr->end - pr->start - RA_THRESHOLD - 1) / 32 + 1);
*/
	if(size == -1 || RA_FLAG_INCACHE){ // Read-ahead permanently disabled, or file already in cache?
		started_ra_range = 0;
		return;
	}
	// Note that the pr->end is also changed to include the RA range.
	if(start+size < pr->end && start_IO_ahead_window == false){
		// Number of required pages larger than the maximum size of the current window.
		// And it is not the case "Some of the required page in the ahead window".
		start_IO_ahead_window = true;
		if(ahead_size == 0){ // Empty ahead window?
			ahead_start = start + size; // Create a new ahead window following the current window.
			if(RA_FLAG_MISS){ // The size of the new ahead window depends on the RA_FLAG_MISS flag.
				ahead_size = (size - 2) > 4 ? (size - 2) : 4;
			}else{
				ahead_size = (2 * size) > MAX_RA ? MAX_RA : (2 * size);
			}
#ifdef DISKCACHE_RA_DEBUG
			cout << "DiskCache-RA: Empty ahead window. ahead_start=" << ahead_start << ", ahead_size=" << ahead_size << endl;
#endif
		}
		// Replace the current window with the ahead window.
		start = ahead_start;
		size = ahead_size;
		// Create a new ahead window.
		ahead_start = start + size; // Create a new ahead window following the current window.
		if(RA_FLAG_MISS){ // The size of the new ahead window depends on the RA_FLAG_MISS flag.
			ahead_size = (size - 2) > 4 ? size - 2 : 4;
		}else{
			ahead_size = (2 * size) > MAX_RA ? MAX_RA : (2 * size);
		}
#ifdef DISKCACHE_RA_DEBUG
		cout << "DiskCache-RA: Number of required pages larger than the maximum size of the current window. start=" << start << ", size=" << size << ". ahead_start="
				<< ahead_start << ", ahead_size=" << ahead_size << endl;
#endif
	}

	long ra_start = pr->end;
	long ra_end = -1;
	if(start_IO_ahead_window){
		ra_end = ahead_start + ahead_size;
	}else{
		ra_end = start + size;
	}
	start_IO_ahead_window = false; // reset
	pr_t * readahead = NULL;
	if(started_ra_range < ra_end && // At least some of the ra pages are not retrieved by other requests.
			pr->end < ra_end){ // Request is smaller than the ahead window.
		// It may have overlap with other read aheads, but it can be solved in readCache function.
		readahead = new pr_t(pr->fileid, ra_start, ra_end, pr->PG_dirty, NULL, NULL);
		pr->end = ra_end;
		started_ra_range = pr->end;
	}
	pr->readahead_list = readahead;
#ifdef DISKCACHE_RA_DEBUG
	cout << "DiskCache-RA: setReadahead. pr->start=" << pr->start << ", pr->end=" << pr->end;
	if(readahead != NULL)
		// Note that the content in readahead list may be already in cache; in this case, it will be omitted when walking the cache.
		cout << ", ra_list->start=" << readahead->start << ", ra_list->end=" << readahead->end << endl;
	else
		cout << ", ra_list == NULL." << endl;
#endif
}

// Update the windows in read ahead
void DiskCache::file_ra_state::update_flag_incache(long s, int length, bool hit){
#ifdef DISKCACHE_RA_DEBUG
	cout << "DiskCache-RA: update_flag_incache: ";
#endif
	if(hit){
		cache_hit += length;
#ifdef DISKCACHE_RA_DEBUG
		cout << "hit. cache_hit=" << cache_hit << endl;
#endif
		if(cache_hit >= RA_FLAG_INCACHE_SEILING){ // Were all the 256 pages lastly requested already in the page cache?
			RA_FLAG_INCACHE = true; // Reset all. Consider the file as already in cache.
			ahead_start = 0;
			ahead_size = 0;
			cache_hit = 0;
		}
		// Condition omitted: Number of required pages larger than the maximum size of the current window?
	}else{
		cache_hit = 0;
		RA_FLAG_INCACHE = false;
#ifdef DISKCACHE_RA_DEBUG
		cout << "not hit. cache_hit=" << cache_hit << endl;
#endif
	}
}

void DiskCache::initialize()
{
	myID = idInit ++;
	// receive parameters from ini file.
	total_pages = par("total_pages").longValue();
	total_usable_pages = par("usable_pages").longValue();
	free_pages = total_usable_pages;
	dirty_pages = 0;
	cache_r_speed = par("cache_r_speed").doubleValue();
	cache_w_speed = par("cache_w_speed").doubleValue();
	dirty_ratio = par("dirty_ratio").doubleValue();
	dirty_background_ratio = par("dirty_background_ratio").doubleValue();
	dirty_threshold = dirty_ratio * total_usable_pages; // dirty_threshold is based on the total_usable_pages
	dirty_background_threshold = dirty_background_ratio * total_pages;
	writeout_batch = par("writeout_batch").longValue();
	diskread_batch = par("diskread_batch").longValue();
	disable_ra = (bool)par("disable_ra").longValue();

	dirty_expire_secs = par("dirty_expire_centisecs").doubleValue() / 100;
	if(dirty_expire_secs >= 0){
		bg_writeout = new PageRequest("DIRTYEXP_TIMER", DIRTY_PAGE_EXPIRE);
		bg_writeout_active = 0;
	}else{ // Disabled.
		bg_writeout = NULL;
	}

	cacheAccess = new PageRequest("cacheAccess", CACHE_ACCESS);

	reqQ = new list<PageRequest *>();
	prs = new map<PageRequest *, pr_t *>();
	files = new map<int, file_t *>();
	lru_list = NULL;

	num_files = 0;

#ifdef MONITOR_DIRTYPAGE
	// Mem Debug
	cMessage * dbgmsg = new cMessage("M_DIRTYPAGE", M_DIRTYPAGE);
	scheduleAt((simtime_t)(simTime() + 0.2), dbgmsg);
#endif

#ifdef MONITOR_CACHEDPAGE
	// Mem Debug
	cMessage * dbgmsg = new cMessage("M_CACHEDPAGE", M_CACHEDPAGE);
	scheduleAt((simtime_t)(simTime() + 0.2), dbgmsg);
#endif

	trimCacheCounter = 0;
	trimCacheInt = 500;

	checkHealthCounter = 0;
	checkHealthInt = 1000;
}

void DiskCache::handleMessage(cMessage *msg)
{
    switch(msg->getKind()){
    case PAGE_REQ:
    	handlePageReq((PageRequest *)msg);
    	break;
    case CACHE_ACCESS:
    	handleCacheAccessResp((PageRequest *)msg);
    	break;
    case DIRTY_PAGE_EXPIRE:
    	handleDirtyPageExpiration();
    	break;
    case BLK_RESP:
    	handleDiskAccessResp((PageRequest *)msg);
    	break;
#ifdef MONITOR_DIRTYPAGE
    case M_DIRTYPAGE:
    	printf("%ld\n", dirty_pages);fflush(stdout);
    	scheduleAt((simtime_t)(simTime() + MONITOR_INT), msg);
    	break;
#endif
#ifdef MONITOR_CACHEDPAGE
    case M_CACHEDPAGE:
    	printf("%ld\n", total_usable_pages-free_pages);fflush(stdout);
    	scheduleAt((simtime_t)(simTime() + MONITOR_INT), msg);
    	break;
#endif
    }
}

void DiskCache::handlePageReq(PageRequest * req){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": handlePageReq. ID[" << req->getID() << "], FID[" << req->getFileId() <<
	        "], startpage[" << req->getPageStart() << "], endpage[" << req->getPageEnd() << "]." << endl;;
	fflush(stdout);
#endif
	if(req->getID() == BG_WRITEOUT_ID){
		PrintError::print("DiskCache", myID, "Reserved ID should not be overlapped with other ones", BG_WRITEOUT_ID);
		return;
	}
	if(req->getPageEnd() - req->getPageStart() <= 0){
		PrintError::print("DiskCache", myID, "PageReq size <= 0", req->getPageEnd() - req->getPageStart());
		return;
	}

	struct file_t * file;
	map<int, file_t *>::iterator it = files->find(req->getFileId());
	if(it == files->end()){ // A new file!
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": handlePageReq. Create file ID=" << req->getFileId() << endl;;
	fflush(stdout);
#endif
		file = new file_t(req->getFileId(), req->getSYNC(), req->getDIRECT(), NULL);
		files->insert(std::pair<int, file_t *>(file->id, file));
		fileIDs[num_files ++] = file->id;
	}else{
		file = it->second;
	}
	req->setPending(false);
	reqQ->push_back(req);

	pr_t * pr = new pr_t(req->getFileId(), req->getPageStart(), req->getPageEnd(), !req->getRead(), NULL, NULL);

	if(req->getRead() && !disable_ra){ // Read-ahead setting is done here.
		file->f_ra->page_cache_readahead(req->getPageStart(), req->getPageEnd() - req->getPageStart());
		file->f_ra->setReadahead(pr); // Note that the pr->end may be changed to include the RA range.
	}

	prs->insert(std::pair<PageRequest *, pr_t *>(req, pr));

	accessCache();
}

// Read or write on the cache.
// Read cache -> no increase in cache size.
// Write cache -> increase the cache size. Flags do change.
// Read-ahead does not go through this function, because there's no cache access triggered by read-ahead.
void DiskCache::handleCacheAccessResp(PageRequest * resp){
#ifdef DISKCACHE_DEBUG
    	cout << "DiskCache #" <<  myID << ": Cache access done. resp: [" << resp->getPageStart() << ","
    	        << resp->getPageEnd() << "]" << endl;
#endif
	// Update the requested pr. If the request is finished, send it back.
	long id = resp->getID();
	long subid = resp->getSubID();
	list<PageRequest *>::iterator it;
	PageRequest * origreq;
	map<PageRequest *, pr_t *>::iterator it2;

	for(it = reqQ->begin(); it != reqQ->end(); it ++){
		// Find the request with this ID.
		if((*it)->getID() == id && (*it)->getSubID() == subid){
			origreq = *it;
			origreq->setPending(false); // it's not pending any more.

			// Find the page range.
			it2 = prs->find(origreq);
			if(it2 == prs->end()){
				PrintError::print("DiskCache", myID, "can not find request in map prs.", (*it)->getID());
				return;
			}
			// Later modified, Yonggang.
			// The page range and original requests need to be updated.
			it2->second->end = resp->getPageEnd();
			origreq->setPageEnd(resp->getPageEnd());

			if(resp->getPageEnd() == origreq->getPageEnd()){ // The original part is done.
	            pr_t * pr = it2->second;
				if(pr->readahead_list != NULL){ // RA is undone.
#ifdef DISKCACHE_DEBUG
					cout << "DiskCache #" <<  myID << ": Just the original part is done. PageReq finished: ["
							<< resp->getPageStart() << "," << resp->getPageEnd() << "]" << endl;
					fflush(stdout);
#endif
					PageRequest * rareq = new PageRequest(*origreq); // Create a page request for read ahead.
					rareq->setSubID(RA_SUBID); // The original request is solely for the read ahead purpose.
					rareq->setPageEnd(rareq->getPageStart()); // Mark the length of the new request to be 0.
					reqQ->push_back(rareq); // Insert the relationship.
					prs->insert(std::pair<PageRequest *, pr_t *>(rareq, pr));
					pr->start = resp->getPageEnd();
				}else{ // RA is done or does not exist.
#ifdef DISKCACHE_DEBUG
					cout << "DiskCache #" <<  myID << ": handleFinishedPageReq. PageReq finished: ["
							<< origreq->getPageStart() << "," << origreq->getPageEnd() << "]" << endl;
					fflush(stdout);
#endif
					delete pr;
					pr = NULL; // Nullify it.
				}
				reqQ->remove(origreq); // Remove the original request.
				prs->erase(it2); // Remove the position from the map.
				// Send the original request.
				origreq->setName("PAGE_RESP");
				origreq->setKind(PAGE_RESP);
				send(origreq, "vfs$o");
			}
			break;
		}
	}

	if(it == reqQ->end()){
		PrintError::print("DiskCache", myID, "can not find record in reQ when handling cache access response.");
		return;
	}

	// update the cache.
	struct pr_t resppr = pr_t(resp->getFileId(), resp->getPageStart(), resp->getPageEnd(), !resp->getRead(), NULL, NULL);
	walkCache_update(resp->getFileId(), &resppr, false, false);// This operation releases the locks.
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": page unlocked: [" << resp->getPageStart() << "," <<  resp->getPageEnd() << "]." << endl;
	fflush(stdout);
#endif
	accessCache();
}

// It might be a read from disk, or a write to disk.
void DiskCache::handleDiskAccessResp(PageRequest * resp){
#ifdef DISKCACHE_DEBUG
    cout << "DiskCache #" <<  myID << ": handleDiskAccessResp. ID[" << resp->getID()
   			<< "], pagestart[" << resp->getPageStart() << "], pageend[" << resp->getPageEnd() << "]." << endl;
#endif
	long id = resp->getID();
	long subid = resp->getSubID();
	list<PageRequest *>::iterator it;
	map<PageRequest *, pr_t *>::iterator it2;
	PageRequest * origreq;

	// Find the request from the request ID.
	for(it = reqQ->begin(); it != reqQ->end(); it ++){
		if((*it)->getID() == id && (*it)->getSubID() == subid){
			origreq = *it;
			origreq->setPending(false); // it's not pending any more.
			break;
		}
	}

	// Find the corresponding page range.
	it2 = prs->find(origreq); // Find the pr from the request.
	if(it2 == prs->end()){
		PrintError::print("DiskCache", myID, "handleDiskAccessResp - Can not find mapping from request", origreq->getID());
		return;
	}
	pr_t * pr = it2->second;

	// Check the writeout list.
	if(pr->writeout_list != NULL){ // If not NULL, it must be the write out. Update the write out page range.
		if(resp->getPageStart() != pr->writeout_list->start || resp->getPageEnd() > pr->writeout_list->end){
			PrintError::print("DiskCache", myID,
				"handleDiskAccessResp - the response range does not match the expected write out range.");
			return;
		}

		if(resp->getPageEnd() == pr->writeout_list->end){
			pr_t * tmp = pr->writeout_list->next;
			delete pr->writeout_list;
			pr->writeout_list = tmp;
		}else{ // resp->getPageEnd() < pr->writeout_list->end
			pr->writeout_list->end = resp->getPageEnd();
		}

		// IF background write out: see if we need to add more pages to write out.
		if(resp->getID() == BG_WRITEOUT_ID && pr->writeout_list == NULL){
			setBGWriteoutPR(pr);
		}
	}

	// check the read ahead list.
	if(pr->readahead_list != NULL){ // If not NULL, part of it may be the read ahead. Update the corresponding page range.
		if(resp->getPageEnd() > pr->readahead_list->start){ // Involves the read ahead pages.
			if(resp->getPageEnd() == pr->end){ // Read ahead is done.
				pr->end = pr->readahead_list->start;
				delete pr->readahead_list;
				pr->readahead_list = NULL;
				if(origreq->getSubID() == RA_SUBID){ // The original request is pure RA request. Delete the the original request.
#ifdef DISKCACHE_DEBUG
					cout << "DiskCache #" <<  myID << ": handleFinishedPageReq. RA req finished: ["
							<< resp->getPageStart() << "," << resp->getPageEnd() << "]" << endl;
					fflush(stdout);
#endif
					reqQ->remove(origreq);
					prs->erase(it2); // Remove the position from the map.
					delete origreq;
					origreq = NULL;
					delete pr;
					pr = NULL;
				}
			}else if(resp->getPageEnd() > pr->end){
				PrintError::print("DiskCache", myID, "The blk response end page is bigger than the page range.");
				cerr << "pr->end=" << pr->end << ", resp->end=" << resp->getPageEnd() << endl;
			}else{ // resp->getPageEnd() < pr->end
				PrintError::print("DiskCache", myID, "The blk response end page is smaller than the page range.");
				cerr << "pr->end=" << pr->end << ", resp->end=" << resp->getPageEnd() << endl;
			}
		}
	}

	struct pr_t resppr = pr_t(resp->getFileId(), resp->getPageStart(), resp->getPageEnd(), !resp->getRead(), NULL, NULL);
	walkCache_update(resp->getFileId(),&resppr, false, true);// This operation releases the locks.
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": page unlocked: [" << resp->getPageStart() << "," <<  resp->getPageEnd() << "]." << endl;
	fflush(stdout);
#endif

	delete resp;
//	if(it != reqQ->end() && (*it)->getRead()){ // This means it is triggered by the request. We need to further read the content from cache.
//		scheduleCacheAccess((*it)->getID(), (*it)->getFileId(), resp->getPageStart(), resp->getPageEnd(), true);
//	}
	accessCache(); // The same content in cache will be accessed later.
}

// Start the back-ground write out.
void DiskCache::handleDirtyPageExpiration(){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": handleDirtyPageExpiration." << endl;
#endif
	bg_writeout->setID(BG_WRITEOUT_ID); // Set this special ID for internal use. This ID is unique inside DiskCache.
	bg_writeout->setRead(0); // Write
	bg_writeout->setPending(false);

	reqQ->push_back(bg_writeout);

	// Create page range for bg_writeout.
	pr_t * pr = new pr_t(0, 0, 0, 1, NULL, NULL);
	pr->writeout_list = setWriteout(0, writeout_batch); // Start from file 0, do the write out.
	if(pr->writeout_list == NULL){
		PrintError::print("DiskCache", myID, "handleDirtyPageExpiration - dirty page expired, but cannot write back any pages.");
		return;
	}

	prs->insert(std::pair<PageRequest *, pr_t *>(bg_writeout, pr));

#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": Background writeback " << pr->writeout_list->fileid << " - ["
			<< pr->writeout_list->start << "," << pr->writeout_list->end << "]" << endl;
	fflush(stdout);
#endif

	bg_writeout_active = 1; // Now background write out is active.
	scheduleDiskAccess(bg_writeout->getID(), 0, pr->writeout_list->fileid, pr->writeout_list->start, pr->writeout_list->end, false);

	bg_writeout->setPending(true);
}

void DiskCache::setBGWriteoutPR(pr_t * pr){
	pr->writeout_list = setWriteout(0, writeout_batch); // Start from file 0, do the write out.
	if(pr->writeout_list != NULL){

#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": setBGWriteoutPR " << pr->writeout_list->fileid << " - ["
			<< pr->writeout_list->start << "," << pr->writeout_list->end << "]" << endl;
	fflush(stdout);
#endif
		return;
	}
//#ifdef DISKCACHE_DEBUG
	printf("DiskCache: setBGWriteout done now.\n");
	fflush(stdout);
//#endif
	// Background write out done. Set flag to be inactive.
	bg_writeout_active = 0;

	map<PageRequest *, pr_t *>::iterator it;
	// Delete the page range.
	it = prs->find(bg_writeout);
	if(it == prs->end())
		PrintError::print("DiskCache", myID, "can not find request in map prs.", bg_writeout->getID());
	delete pr;
	pr = NULL;
	prs->erase(it); // Remove the position from the map.
	reqQ->remove(bg_writeout);
}


// Checks if cache is busy. If not, read/write cache.
void DiskCache::accessCache(){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": accessCache." << endl;
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
			if(req->getRead()){
				if(readCache(req)) {
					it = reqQ->begin(); // This means a request was just finished, or a page range is locked.
				}
			}else {
				writeCache(req);
			}
		}
	}
}

// writeCache and readCache:
// find the first access that should be conducted on cache/disk in request.
// set request pending, get the right access size.
// Important notice: writeCache and readCache do not change the current cache.
void DiskCache::writeCache(PageRequest * req){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": Write cache. ID: " << req->getID();
	fflush(stdout);
#endif
	map<PageRequest *, pr_t *>::iterator it1 = prs->find(req);
	if(it1 == prs->end()){
		PrintError::print("DiskCache", myID, "writeCache - Can not find the page-range from map for PageRequest", req->getID());
		return;
	}

	pr_t * rpr = it1->second; // Requested pr.

	if(rpr->writeout_list != NULL){ // You have explicit writeout tasks - finish them first.
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": writeback " << rpr->writeout_list->fileid << " - ["
			<< rpr->writeout_list->start << "," << rpr->writeout_list->end << "]" << endl;
	fflush(stdout);
#endif
		scheduleDiskAccess(req->getID(), req->getSubID(), rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end, false);
		return;
	}

	// You don't have explicit write out tasks - write to the cache.

	// First set the dirty page expiration clock, if not set before and background write out is inactive.
	if(bg_writeout != NULL && bg_writeout_active == 0 && !bg_writeout->isScheduled()){
		scheduleAt(simTime()+dirty_expire_secs,bg_writeout);
		cout << "schedule bg_writeout at " << simTime()+dirty_expire_secs << endl;
	}

	long start = rpr->start;
	long end = rpr->end;
	end = start + getAccessSize(req->getFileId(),start,end,false,true);

	int dirty_increase = walkCache_countMissing(req->getFileId(), start, end);
//	cout << "dp+di=" << dirty_pages+dirty_increase << ", dt=" << dirty_threshold << endl;
	if(dirty_pages + dirty_increase >= dirty_threshold){ // Dirty page threshold is exceeded. You should start to write out.
		rpr->writeout_list = setWriteout(req->getFileId(), writeout_batch);
//		cout << "Write out." << endl;
		if(rpr->writeout_list == NULL)
			PrintError::print("DiskCache", myID, "writeCache - drity_ratio achieved, but cannot write back any pages.");
#ifdef DISKCACHE_DEBUG
	cout << "writeback " << rpr->writeout_list->fileid << " - ["
			<< rpr->writeout_list->start << "," << rpr->writeout_list->end << "]" << endl;
	fflush(stdout);
#endif
		scheduleDiskAccess(req->getID(), req->getSubID(), rpr->writeout_list->fileid, rpr->writeout_list->start, rpr->writeout_list->end, false);
	}else{
#ifdef DISKCACHE_DEBUG
	cout << req->getFileId() << " - [" << start << "," << end << "]" << endl;
	fflush(stdout);
#endif
		scheduleCacheAccess(req->getID(), req->getSubID(), req->getFileId(), start, end, false);
	}
}

int DiskCache::readCache(PageRequest * req){
	pr_t * cpr, * rpr;

	// Find the corresponding page range from the request.
	map<PageRequest *, pr_t *>::iterator it1 = prs->find(req);

	if(it1 == prs->end()){
		PrintError::print("DiskCache", myID, "readCache - Can not find the page-range from map for PageRequest", req->getID());
		return -1;
	}

	// Find the page ranges of the cached data from the page table.
	map<int, file_t *>::iterator it2 = files->find(req->getFileId());
	if(it2 == files->end()){
		PrintError::print("DiskCache", myID, "readCache - can not find the file from map for file ID.\n", req->getFileId());
		return -1;
	}

	file_t * file = it2->second;
	cpr = file->pr_head; // Cached page-range.

	long start, end;
	rpr = it1->second;

	start = rpr->start;
	end = rpr->end;


#ifdef DISKCACHE_DEBUG

	cout << "DiskCache #" <<  myID << ": readCache. FID=" << file->id << ", req: [" << req->getPageStart() << "," << req->getPageEnd()
			<< "] rpr: [" << start << "," << end << "]" << endl;
	fflush(stdout);
#endif

	// Find out the next read access range.
	while(cpr != NULL && (rpr->start != rpr->end)){
		if(!(cpr->start >= rpr->end || cpr->end <= rpr->start)){ // They intersect
			if(cpr->PG_locked){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": pagerange is locked. [" << cpr->start << "," << cpr->end << "]." << endl;
	fflush(stdout);
#endif
				req->setPending(false); // Wait for the locking operation to be done.
				return 0;
			}

			// Get the higher start - intersection starts
			if(rpr->start < cpr->start){ // beginning not cached.
				if(rpr->end < cpr->start) {
					end = rpr->end;
				}
				else {
					end = cpr->start;
				}
				end = start + getAccessSize(req->getFileId(), start, end, true, false);
				scheduleDiskAccess(req->getID(), req->getSubID(), req->getFileId(), start, end, true);
				if(!disable_ra) {
					file->f_ra->update_flag_incache(start, end-start, false); // Update the read ahead INCACHE flag
				}
			}else{ // beginning cached. - need to see if it is RA? If it is, ignore it.
				if(cpr->end < rpr->end) {
					end = cpr->end;
				}
				else {
					end = rpr->end;
				}
				end = start + getAccessSize(req->getFileId(), start, end, true, true);

				if(rpr->readahead_list == NULL ||
						(rpr->readahead_list != NULL && rpr->readahead_list->start > rpr->start)){
				    // You still have original read task undone.
					// cacheaccess_end = start + getAccessSize(req->getFileId(), start, cacheaccess_end, true, true);
				    // Yonggang, later committed.
                    scheduleCacheAccess(req->getID(), req->getSubID(), req->getFileId(), start, end, true); // Yonggang, later modified.
					// scheduleCacheAccess(req->getID(), req->getSubID(), req->getFileId(), start, cacheaccess_end, true);
					if(!disable_ra) {
					    file->f_ra->update_flag_incache(start, end - start, true); // Yonggang, later modified.
						// file->f_ra->update_flag_incache(start, cacheaccess_end - start, true);
					}
					// Update the read ahead INCACHE flag
				}else{ // ONLY READ-AHEAD modifies rpr.
					// This happens if the read ahead content is already got by other requests.
					rpr->start = end; // Read-ahead does not access on-cache data.
					start = rpr->start;
					end = rpr->end;
					continue;
				}
			}
			return 0;
		}else if(cpr->end <= rpr->start){ // Do not intersect, go to next.
			cpr = cpr->next;
		}else{ // rpr->end < cpr->start
			// The requested pages do not exist in cache! - the requested ones fall into a hole in the cached ones.
			end = start + getAccessSize(req->getFileId(), start, end, true, false);
			scheduleDiskAccess(req->getID(), req->getSubID(), req->getFileId(), start, end, true);
			if(!disable_ra) {
				file->f_ra->update_flag_incache(start, end-start, false); // Update the read ahead INCACHE flag
			}
			return 0;
		}
	}
	if(cpr == NULL){
		// The requested pages do not exist in cache! - all cached ones are lower than the requested one.
		end = start + getAccessSize(req->getFileId(), start, end, true, false);
		scheduleDiskAccess(req->getID(), req->getSubID(), req->getFileId(), start, end, true);
		if(!disable_ra) {
			file->f_ra->update_flag_incache(start, end-start, false); // Update the read ahead INCACHE flag
		}
	}
	else{ // rpr->start == rpr->end. All done for this request. And rpr is read-ahead range.
#ifdef DISKCACHE_DEBUG
		cout << "DiskCache #" <<  myID << ": handleFinishedPageReq. PageReq finished: ["
				<< req->getPageStart() << "," << req->getPageEnd() << "]" << endl;
		fflush(stdout);
#endif
		// rpr->start = rpr->end means the readahead_list is done.
		if(it1->second->readahead_list != NULL){
			delete it1->second->readahead_list;
			it1->second->readahead_list = NULL;
		}

		if(req->getSubID() != RA_SUBID){ // This means req is a request sent from client. Send it back.
			reqQ->remove(req);
			req->setName("PAGE_RESP");
			req->setKind(PAGE_RESP);
			send(req, "vfs$o");
		}else{ // This means req is a request just for read ahead. Delete it.
			reqQ->remove(req);
			delete req;
			req = NULL;
		}
		delete rpr;
		rpr = NULL;
		prs->erase(it1); // Remove the position from the map.
		return 1;
	}
	return 0;
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
	if(!incache){ // If it is a disk access
		if(read){
			if(end - start > diskread_batch)
				return diskread_batch;
		}else{
			if(end - start > writeout_batch)
				return writeout_batch;
		}
	}
	return end - start; // Otherwise, we do not control.
}

// schedulDiskAccess and scheduleCacheAccess:
// Change the cache. Change global variables here.
void DiskCache::scheduleDiskAccess(long id, long subid, int fid, long start, long end, bool read){
	if(end - start <= 0){ // Check the disk access size.
		PrintError::print("DiskCache", myID, "dispatched diskreq length <= 0", end-start);
		cerr << "ID=" << id << ", start[" << start << "], end[" << end << "]" << endl;
		return;
	}
	PageRequest * req = new PageRequest("BLK_REQ");
	req->setID(id);
	req->setSubID(subid);
	req->setKind(BLK_REQ);
	req->setFileId(fid);
	req->setPageStart(start);
	req->setPageEnd(end);
	req->setRead(read);
	send(req, "lfs$o");

#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": scheduleDiskAccess. ["
			<< req->getPageStart() << "," << req->getPageEnd() << "]" << endl;
	fflush(stdout);
#endif
	preOpOnCache(fid, start, end, read, true);
}

void DiskCache::scheduleCacheAccess(long id, long subid, int fid, long start, long end, bool read){
	// No read-ahead
	if(end <= start){
		PrintError::print("DiskCache",  myID, "scheduleCacheAccess - cacheAccess end page <= start page.");
		return;
	}

	if(cacheAccess->isScheduled()){
		PrintError::print("DiskCache", myID, "scheduleCacheAccess - cacheAcess message busy.");
		return;
	}

	cacheAccess->setID(id);
	cacheAccess->setSubID(subid);
	cacheAccess->setFileId(fid);
	cacheAccess->setPageStart(start);
	cacheAccess->setPageEnd(end);
	cacheAccess->setRead(read);
	double cache_acc_speed;
	if(read)
		cache_acc_speed = cache_r_speed;
	else
		cache_acc_speed = cache_w_speed;
	scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + (end - start) * cache_acc_speed), cacheAccess); // Set the timer.
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": scheduleCacheAccess. [" << start << "," << end << "]" << endl;
	fflush(stdout);
#endif
	preOpOnCache(fid, start, end, read, false);
}


void DiskCache::preOpOnCache(int fid, long start, long end, bool read, bool disk){
	struct pr_t lockpr = pr_t(fid, start, end, !read, NULL, NULL);
	walkCache_update(fid, &lockpr, true, disk);

#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": page locked: [" << start << "," << end << "]" << endl;
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
		PrintError::print("DiskCache", myID, "walkCache_update - Can not find the file from map for file ID", fid);
		return;
	}

	file_t * file = it->second;

	// This is for performance purpose.
	if(file->pr_last != NULL && file->pr_last->start < addp->start){
		cachedp = file->pr_last;
	}else{
		cachedp = file->pr_head;
	}

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
                fprintf(stderr, "preop=%d, disk=%d, read=%d. [%ld %ld]\n", preop, disk, read, addp->start, addp->end);
                printCache(fid);
				PrintError::print("DiskCache", myID, " Internal error - cache increase operation not permitted!");
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
				addp = NULL; // signal to jump out.
			}else{
				// ...{+++[ --> ...[+tmp+][
				tmp->end = cachedp->start;
				addp->start = cachedp->start; // Update addp to move forward.
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

			file->pr_last = precachedp; // Record the last access location.

			if(addp == NULL)
				break;

			precachedp = tmp;
		}

		// Requested pages cached.
		// You need to care about the existing cache status.
		if(addp->start < cachedp->end){ // [___{---]...
#ifdef DISKCACHE_DEBUG
			cout << "DiskCache #" <<  myID << ": walkCache_update. addp:[" << addp->start << "," << addp->end << "], cachedp:["
					<< cachedp->start << "," << cachedp->end << "]." << endl;
			fflush(stdout);
#endif
			if(preop && disk && read){
				PrintError::print("DiskCache", myID, "read the pages already in cache from disk.");
				fprintf(stderr, "%d, %d, %d. [%ld %ld]\n", preop, disk, read, addp->start, addp->end);
				fprintf(stderr, "addp->start = %ld, end = %ld, cachedp->start = %ld, cachedp->end = %ld, preop = %d, disk = %d, read = %d\n",
						addp->start, addp->end, cachedp->start, cachedp->end, preop, disk, read);
				printCache(fid);
				printLRU();
				while(1)wait(1);
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
					PrintError::print("DiskCache", myID, "walkCache_Update - writing back pages which are not dirty.");
					fprintf(stderr, "[%ld, %ld]", addp->start, addp->end);
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
			if(!(disk && !read && !preop)){ // Not after write back.
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

					delete cachedp;
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

					if(preop && !read && !disk){ // Write to cache, updates the LRU list order.
						lru_push(tmp); // Update the LRU list order.
					}else{
						lru_insert(tmp, cachedp); // tmp goes to the same position.
					}

					precachedp = cachedp;
					cachedp = tmp; // Move cachedp to tmp.
					addp->start = cachedp->end; // Update addp to move forward.
				}
			}

			// Update global variables.
			if(preop && disk && !read){ // Before write back to disk
				dirty_pages -= tmp->end - tmp->start;
			}
			file->pr_last = precachedp;

			if(addp == NULL) { // Receive the break signal.
				break;
			}
		}
		if(addp->start == addp->end) { // addp size is zero, done.
			break;
		}
		// If you are read + cache + preop, that means you may only partially read the cache. After this hit, break.
		if (preop && !disk && read) {
		    break;
		}
		precachedp = cachedp;
		cachedp = cachedp->next;
	}

#ifdef DISKCACHE_TRIMCACHE
	// Note: trimCahe is enabled for performance purpose.
	trimCacheCounter ++;
	if(trimCacheCounter >= trimCacheInt){
	    trimCacheCounter = 0;
	    trimAllCache();
	}
#endif

#ifdef DISKCACHE_CHECKHEALTH
	checkHealthCounter ++;
	if(checkHealthCounter >= checkHealthInt){
		checkHealthCounter = 0;
		checkListHealth();
	}
#endif

#ifdef VERBOSE
	cout << "Total_pages=" << total_pages << ". Free_pages=" << free_pages << ". Dirty_pages=" << dirty_pages << "." << endl;
	fflush(stdout);
	printCache(fid);
	printLRU();
#endif
}

void DiskCache::trimAllCache(){
	map<int, file_t *>::iterator it;
	for(int i = 0; i < num_files; i ++){
		it = files->find(fileIDs[i]);
		if(it == files->end()){
			PrintError::print("DiskCache", myID, "trimAllCache - Can not find the file from map for file ID", fileIDs[i]);
			return;
		}
		trimCache(it->second);
	}

}

// Note: trimCahe is enabled for performance purpose.
// Must take care of the LRU list, otherwise memory leak may happen.
void DiskCache::trimCache(file_t * file){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" << myID << ": trimCache." << endl;
	fflush(stdout);
#endif
	file->pr_last = NULL; // Nullify the last position pointer, because it may be deleted.
	pr_t * cachedp = file->pr_head;
	pr_t * nextcachedp = NULL;

	while(cachedp && cachedp->next){
		nextcachedp = cachedp->next;
//		cout << "Comparing: [" << cachedp->start << "," << cachedp->end <<
//				"] - [" << nextcachedp->start << "," << nextcachedp->end << "]" << endl;
		fflush(stdout);
		if(pr_t::attrcomp(cachedp, nextcachedp) &&
				cachedp->end == nextcachedp->start &&
				cachedp->lru_prev == nextcachedp){ // This mean it is also adjacent in the LRU list.
//			cout << "Good" << endl;
			cachedp->end = nextcachedp->end;

			cachedp->next = nextcachedp->next;
			if(nextcachedp->next != NULL){
				nextcachedp->next->prev = cachedp;
			}

			cachedp->lru_prev = nextcachedp->lru_prev;
			if(nextcachedp->lru_prev != NULL){
				nextcachedp->lru_prev->lru_next = cachedp;
			}else{ // nextcachedp is the head of LRU list.
				lru_list = cachedp;
			}

			delete nextcachedp;
		}else{
			cachedp = cachedp->next;
		}
	}
#ifdef VERBOSE
	printCache(fid);
#endif
}

int DiskCache::printCache(int fid){
	pr_t * cachedp = files->find(fid)->second->pr_head;
	printf("--------------PRINT CACHE FOR FILE %d---------------\n", fid);
	printf("Total usable pages: %ld, Free pages: %ld, Dirty pages: %ld\n",
			total_usable_pages, free_pages, dirty_pages);
	int i = 0;
	while(true){
		if(i > 2000){
			PrintError::print("DiskCache", myID, "printCache - page_cache is too long.");
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

// FLUSH THE DIRTY PAGES
// Starting from the current fid, find out n dirty pages to write back.
// It does not mutate the cache.
DiskCache::pr_t * DiskCache::setWriteout(int fid, int n){
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
				PrintError::print("DiskCache", myID, "setWriteout - Can not find the file from map for file ID", fileIDs[i]);
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
	cout << "DiskCache #" <<  myID << ": pagetable_delete. FID=" << pr->fileid << " ["
			<< pr->start << "," << pr->end << "]" << endl;
	fflush(stdout);
#endif
	if(pr == NULL){
		PrintError::print("DiskCache", myID, "pagetable_delete - received pr is NULL");
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
	// If arrives here: an error happened.
	PrintError::print("DiskCache", myID, "pagetable_delete - can not find pr in file", fid);
	printCache(fid);
	printLRU();
	return false;
}

// A simple LRU algorithm implementation
// Only mutates the lru_list.
bool DiskCache::lru_delete(pr_t * del_p){
#ifdef VERBOSE
	cout << "DiskCache #" <<  myID << ": lru_delete. FID " << del_p->fileid
			<< " [" << del_p->start << "," <<  del_p->end << "]" << endl;
	fflush(stdout);
#endif
	if(del_p == NULL){
		PrintError::print("DiskCache", myID, "lru_delete - received del_p is NULL");
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
	PrintError::print("DiskCache", myID, "lru_delete - cannot find the pr from lru_list!");
	fprintf(stderr,"fid: %d, [%ld, %ld].\n", del_p->fileid, del_p->start,del_p->end);
	return false;
}

// Only mutates lru_list.
void DiskCache::lru_push(pr_t * new_p){
	if(new_p == NULL){
		PrintError::print("DiskCache", myID, "lru_push - the new pr is NULL!");
		return;
	}

	new_p->lru_prev = NULL;
	new_p->lru_next = lru_list;
	if(lru_list != NULL)
		lru_list->lru_prev = new_p;
	else // lru_list was empty
		lru_list_end = new_p;
	lru_list = new_p;
}

// Insert pr1 before pr2 in LRU list.
void DiskCache::lru_insert(pr_t * pr1, pr_t * pr2){
	pr1->lru_prev = pr2->lru_prev;
	pr2->lru_prev = pr1;
	pr1->lru_next = pr2;
	if(pr1->lru_prev != NULL)
		pr1->lru_prev->lru_next = pr1;
	else
		lru_list = pr1;
}

// Use pr1 to substitiute pr2.
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
	cout << "DiskCache #" <<  myID << ": lru_free_pages." << endl;
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
				delete pr;
				pr = prev_pr;
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
//	FILE * fp = fopen("debugoutput", "w+");
	printf("--------------PRINT LRU_LIST -------------------------\n");
	while(true){
		if(cachedp == NULL)
			break;
//		fprintf(fp, "[%ld %ld] FID: %d C:%d DIRTY:%d WB:%d LCKED:%d REF:%d MOD:%d\n",
//				cachedp->start, cachedp->end, cachedp->fileid, cachedp->_count, cachedp->PG_dirty, cachedp->PG_writeback,
//				cachedp->PG_locked, cachedp->PG_referenced, cachedp->modified);
		printf("[%ld %ld] FID: %d C:%d DIRTY:%d WB:%d LCKED:%d REF:%d MOD:%d\n",
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
#ifdef DISKCACH_DEBUG
	cout << "DiskCache #" << myID << ": checkListHealth." << endl;
#endif
	pr_t * pr;
	int total = 0;
	long pages = 0;
	for(int i = 0; i < num_files; i ++){
		pr = files->find(fileIDs[i])->second->pr_head;
		while(pr != NULL){
			if(pr->prev != NULL && pr->prev->next != pr){
				PrintError::print("DiskCache", myID, "checkListHealth - File pagetable list link error.", fileIDs[i]);
				fprintf(stderr, "(file %d)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
						fileIDs[i], pr->prev->start, pr->prev->end, pr->start, pr->end);
				fflush(stderr);
				return false;
			}
			if(pr->next != NULL && pr->next->prev != pr){
				PrintError::print("DiskCache", myID, "checkListHealth - File pagetable list link error.", fileIDs[i]);
				fprintf(stderr, "(file %d)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
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
		if(pr->lru_next != NULL && pr->lru_next->lru_prev != pr){
			PrintError::print("DiskCache", myID, "checkListHealth - LRU list link error.");
			fprintf(stderr, "(lru)doubly-linked list pr[%ld,%ld] <-> pr[%ld,%ld]\n",
					pr->start, pr->end, pr->lru_next->start, pr->lru_next->end);
			fflush(stderr);
			// Temp
			printCache(0);
			printLRU();
			while(1)sleep(10);
			return false;
		}
		pr = pr->lru_next;
		total --;
	}
	if(total != 0){
		PrintError::print("DiskCache", myID, "total pagetable pr != total lru pr.");
		for(int i = 0; i < num_files; i ++){
			printCache(fileIDs[i]);
		}
		printLRU();
		while(1)sleep(10);
		return false;
	}
	return true;
}

void DiskCache::finish(){
#ifdef DISKCACHE_DEBUG
	cout << "DiskCache #" <<  myID << ": finish." << endl;
	fflush(stdout);
#endif
	pr_t * tmp;
	pr_t * lru_p = lru_list;
	while(lru_p != NULL){ // Delete the entire table.
		tmp = lru_p->next;
		delete lru_p;
		lru_p = tmp;
	}
	if(cacheAccess != NULL)
		cancelAndDelete(cacheAccess);
	if(bg_writeout != NULL)
		cancelAndDelete(bg_writeout);
	if(reqQ != NULL){
		reqQ->clear();
		delete reqQ;
	}
	if(prs != NULL){
		prs->clear();
		delete prs;
	}
	if(files != NULL){
		files->clear();
		delete files;
	}
	cout << "DiskCache finish end." << endl;
	fflush(stdout);
}

DiskCache::~DiskCache(){
}
