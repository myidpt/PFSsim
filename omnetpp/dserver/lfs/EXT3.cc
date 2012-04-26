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

#include "EXT3.h"

EXT3::EXT3(int id, int deg, long long disksize, int pagesize, int blksize, const char * inpath, const char * outpath,
		int n_ext_s, int n_ext_g)
		:ILFS(id, deg, disksize, pagesize, blksize, inpath, outpath) {

//	memset(extmap, 0, MAX_LFILE_NUM * sizeof(char *)); // Initialize.
	brs = new map<PageRequest *, br_t *>();
	pageReqQ = new list<PageRequest *>();
	outstanding = 0;
	new_ext_size = n_ext_s;
	new_ext_gap = n_ext_g;
}

EXT3::br_t::br_t(long id, long subid, long long s, long long e){
	reqid = id;
	reqsubid = subid;
	start = s;
	end = e;
	extindex = 0;
}

// Insert the request to the queue.
// Create a corresponding block range, insert the br into brs.
void EXT3::newReq(PageRequest * pagereq){
#ifdef LFS_DEBUG
	printf("EXT3: newreq - pagereq [%ld %ld].\n", pagereq->getPageStart(), pagereq->getPageEnd());
	fflush(stdout);
#endif
	// Push the page request to the wait queue.
	pageReqQ->push_back(pagereq);
	// Create the block-range for the request.
	br_t * br = new br_t(pagereq->getID(), pagereq->getSubID(),
			pagereq->getPageStart() * (long long)(page_size / blk_size),
			pagereq->getPageEnd() * (long long)(page_size / blk_size));
	brs->insert(std::pair<PageRequest *, br_t *>(pagereq, br));

	int fid = pagereq->getFileId();

	if(extlist[fid] == NULL){ // A new file is created.
	    extlist[fid] = (ext_t *)malloc(MAX_EXT_ENTRY_NUM * sizeof(ext_t));
        for(int i = 0; i < MAX_EXT_ENTRY_NUM; i ++){ // init
            extlist[fid][i].logistart = -1;
            extlist[fid][i].accessed = -1;
        }
	}
}

// Return NULL if waiting for response or the queue is empty.
// Otherwise, return the next request.
BlkRequest * EXT3::dispatchNext(){
	if(outstanding >= degree)
		return NULL;
	// Get a page request which is not pending.
	PageRequest * pagereq = NULL;
	for(list<PageRequest *>::iterator it = pageReqQ->begin(); it != pageReqQ->end(); it ++){
		if(!((PageRequest *)(*it))->getPending()){
			pagereq = (PageRequest *)(*it);
			pagereq->setPending(true);
			break;
		}
	}
	if(pagereq == NULL)
		return NULL;

	outstanding ++;
	// Find the block range.
	map<PageRequest *, br_t *>::iterator it;
	it = brs->find(pagereq);
	if(it == brs->end())
		PrintError::print("EXT3 - dispatchNext", "Can not find request in map brs.", pagereq->getID());
	br_t * br = it->second;

#ifdef LFS_DEBUG
	printf("EXT3: dispatchNext - br (file address space) [%lld %lld].\n", br->start, br->end);
	fflush(stdout);
#endif

	int fid = pagereq->getFileId();


    int extindex = findExtEntry(fid, br->start); // The index of the extent.
    if(extindex < 0){
        return NULL;
    }else if(extindex == extentryNum[fid]){ // Not allocated yet.
    	  printf("EXT3: dispatchNext - extent not allocated yet - extlist[%d][%d]\n", fid, extindex);
#ifdef LFS_DEBUG
            printf("EXT3: dispatchNext - extent not allocated yet - extlist[%d][%d]\n", fid, extindex);
            fflush(stdout);
#endif
//        cout << "ERROR--->" << fid << " " << br->start << " " << br->end - br->start << endl;
//        while(1)sleep(5);
        extlist[fid][extindex].logistart = br->start; // Allocate an extent.
        extlist[fid][extindex].length = new_ext_size;
        if(extindex == 0)
        	extlist[fid][extindex].phystart = new_ext_gap;
        else
        	extlist[fid][extindex].phystart = extlist[fid][extindex - 1].phystart + new_ext_gap;
        extlist[fid][extindex].accessed = -1;
        extentryNum[fid] ++; // Increment the extent number record.
    }
    br->extindex = extindex;
    int extlength = extlist[fid][extindex].length;
    int offinext = br->start - extlist[fid][extindex].logistart; // The offset inside the extent.
    br->extoff = offinext;


	long long start, end; // The start and end of the next access.
	long reqsubid = pagereq->getSubID(); // The SubID of the request, metadata -> 1, data-> 0 at the bit of LFS_SUB_ID_BASE.

	if(extlist[fid][extindex].accessed == -1){ // This extent was not accessed before. Access the meta-data first.
#ifdef LFS_DEBUG
	printf("EXT3: dispatchNext - extent not accessed before - extlist[%d][%d]\n", fid, extindex);
	cout << "extlist[fid][extindex].phystart == " << extlist[fid][extindex].phystart << endl;
	fflush(stdout);
#endif

	    start = extlist[fid][extindex].phystart - 1; // Access the meta-data, which is just before the data.
		end = start + 1;

		reqsubid += 0;
	}else{ // This extent was accessed. This time we only access the data.
#ifdef LFS_DEBUG
	printf("EXT3: dispatchNext - extent accessed before - extlist[%d][%d] logistart[%ld] phystart[%ld] length[%d]\n", fid, extindex,
	        extlist[fid][extindex].logistart, extlist[fid][extindex].phystart, extlist[fid][extindex].length);
	fflush(stdout);
#endif
		start = extlist[fid][extindex].phystart + offinext;
		end = br->end - br->start + start;
		if(end > extlist[fid][extindex].phystart + extlength)
			end = extlist[fid][extindex].phystart + extlength;

		reqsubid += 1 * LFS_SUB_ID_BASE;
	}
#ifdef LFS_DEBUG
	printf("EXT3: dispatchNext - (disk address space) start[%lld], end[%lld]\n", start, end);
	fflush(stdout);
#endif

	BlkRequest * blkreq = new BlkRequest("BLK_REQ", BLK_REQ);
	blkreq->setID(pagereq->getID());
	blkreq->setSubID(reqsubid);
	blkreq->setFileId(pagereq->getFileId());
	blkreq->setBlkStart(start);
	blkreq->setBlkEnd(end);
	blkreq->setRead(pagereq->getRead());
	return blkreq;
}

PageRequest * EXT3::finishedReq(BlkRequest * blkresp){
#ifdef LFS_DEBUG
	printf("EXT3: finishedReq - received BlkRequest - %ld\n", blkresp->getID());
	fflush(stdout);
#endif
	// Find the original request.
	long prid = blkresp->getID();
	long prsubid = blkresp->getSubID() / VFS_SUB_ID_BASE * VFS_SUB_ID_BASE; // The subID is on a higher level.
	PageRequest * pageresp = NULL; // Get the page request from the blkresp.
	for(list<PageRequest *>::iterator it = pageReqQ->begin(); it != pageReqQ->end(); it ++){
		if(((PageRequest *)(*it))->getID() == prid && ((PageRequest *)(*it))->getSubID() == prsubid){
			pageresp = (PageRequest *)(*it);
			break;
		}
	}

	if(pageresp == NULL){
		PrintError::print("EXT3 - finishedReq", "Can not find PageRequest from pageReqQ.", prid);
		return NULL;
	}
	pageresp->setPending(false); // Not pending any more.

	outstanding --;

	// Find the block range.
	map<PageRequest *, br_t *>::iterator it;
	it = brs->find(pageresp);
	if(it == brs->end())
		PrintError::print("EXT3 - finishedReq", "Can not find request in map brs.\n", prid);
	br_t * br = it->second;

	int fid = blkresp->getFileId();
//	int extindex = blkresp->getExtent();
	long long blkstart = blkresp->getBlkStart();
	long long blkend = blkresp->getBlkEnd();
	delete blkresp;
	blkresp = NULL;

	// Update the block range.
	if(extlist[fid][br->extindex].phystart - 1 == blkstart && blkend - blkstart == 1){ // Meta-data, no need to update.
#ifdef LFS_DEBUG
	printf("EXT3: finishedReq - metadata\n");
	fflush(stdout);
#endif
		if(extlist[fid][br->extindex].accessed == 1)
			PrintError::print("EXT3 - finishedReq", "received metadata, but not expecting it.");
		extlist[fid][br->extindex].accessed = 1; // Mark the map.
	}else if(blkstart == extlist[fid][br->extindex].phystart + br->extoff){ // Data

#ifdef LFS_DEBUG
	printf("EXT3: finishedReq - data\n");
	fflush(stdout);
#endif

		br->start += blkend - blkstart;
		if(br->start == br->end){ // All done.
			brs->erase(it);
			delete br;
			br = NULL;
			pageReqQ->remove(pageresp);

			pageresp->setName("BLK_RESP");
			pageresp->setKind(BLK_RESP);
			return pageresp;
		}
	}else
		PrintError::print("EXT3 - finishedReq", "Received unexpected blkresp.", blkresp->getBlkStart());
	return NULL;
}

EXT3::~EXT3() {
	if(brs != NULL){
		brs->clear();
		delete brs;
	}
}
