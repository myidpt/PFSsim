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

/*
 * The LocalFS translates the application-level {app_ID, offset} to the block-level {offset}
 * and manages a cache module. It also direct the request to the Disk if there's a cache-miss.
 */

#include "dserver/LocalFS.h"

Define_Module(LocalFS);

LocalFS::LocalFS() {
}

LocalFS::filereq_type::filereq_type(long id){
	diskread = NULL;
	diskwrite = NULL;
	curaccess = NULL;
	fileReqId = id;
	readpr = false;
	blkReqId = 0;
	accessingCache = false;
}

void LocalFS::initialize(){

	cache = new NRU(MAX_PAGENUM);
	fileReqQ = new FIFO(LOCALFS_DEGREE);
	fileReqList = new list<filereq_type *>;
}

void LocalFS::handleMessage(cMessage * cmsg){
	/*
	 * 			   LFILE_REQ >   	        BLK_REQ >
	 *  DSdaemon <------------> LocalFS <-------------> Disk
	 *            < LFILE_RESP             < BLK_RESP
	 */
	gPacket * gpkt = (gPacket *)cmsg;
	switch(gpkt->getKind()){
	case LFILE_REQ:
		handleNewFileReq(gpkt);
		break;
	case BLK_RESP:
		handleBlkResp(gpkt);
		break;
	case SELF_EVENT:
		handleCacheAccessFinish(gpkt);
		break;
	}
}

void LocalFS::handleNewFileReq(gPacket * req){
	fileReqQ->pushWaitQ(req);
	filereq_type * tmp = new filereq_type(req->getId());
	fileReqList->push_back(tmp);
	dispatchNextFileReq();
}

LocalFS::filereq_type * LocalFS::findFileReq(long id){
	filereq_type * fr = NULL;
	list<filereq_type*>::iterator i;
	for(i=fileReqList->begin(); i != fileReqList->end(); i++){
		if((*i)->fileReqId == id){
			fr = *i;
			break;
		}
	}
	if(fr == NULL){
		fprintf(stderr, "ERROR LocalFS: Can't find filereq_type element in fileReqList with id = %ld.\n", id);
	}
	return fr;
}

void LocalFS::dispatchNextFileReq(){
	gPacket * filereqpkt = fileReqQ->dispatchNext();
	if(filereqpkt == NULL) // Currently busy
		return;

	filereqpkt->setDispatchtime(SIMTIME_DBL(simTime()));
	if(filereqpkt->getRead())
		filereqpkt->setByteLength(100 + filereqpkt->getSize()); // If it is read, we need to simulate the packet length when it returns.

	filereq_type * fr = findFileReq(filereqpkt->getId());

	filereqpkt->setKind(SELF_EVENT);
	// Assumption: the data for one application are stored continuously on the disk.
	// The physical address in its disk area is calculated by the following.
	long long req_data_pageoffset =
		(filereqpkt->getHighoffset() * LOWOFFSET_RANGE + filereqpkt->getLowoffset()) / PAGESIZE;
	long req_app_pageoffset = filereqpkt->getApp() * APP_DATA_RANGE;

	// Get the start and end page indexes of the request - just divide the offsets by PAGESIZE.
	long long req_pagestart = req_data_pageoffset + req_app_pageoffset;
	long req_pagenum = filereqpkt->getSize() / PAGESIZE;
	if(filereqpkt->getSize() % PAGESIZE > 0) // You should consider the remainder as another page.
		req_pagenum ++;
	long long req_pageend = req_pagestart + req_pagenum;
	ICache::pr_type * req_pagerange = new ICache::pr_type(req_pagestart, req_pageend,
			true, !filereqpkt->getRead(), NULL);

	int cachedsize = 0; // Unit: page
	if(filereqpkt->getRead()){ // If it is a read operation, you need to measure how much data are in the cache.
		cachedsize = cache->getCachedSize(req_pagerange);
		if(cachedsize != 0){
			fr->accessingCache = true;
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) +
					cachedsize * CACHE_R_SPEED), filereqpkt); // Schedule to the future according to the read speed.
		}
	}else{ // If it is a write operation, the size is the entire data to be written.
		cachedsize = filereqpkt->getSize();
		if(cachedsize != 0){
			fr->accessingCache = true;
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) +
					cachedsize * CACHE_W_SPEED), filereqpkt); // Schedule to the future according to the write speed.
		}
	}

	/*
	 * If read from a file:
	 * 1. If the cache is full of dirty pages, write back the dirty pages to give room to the data read from disk
	 * (this case is rare because in Linux the dirty page ratio threshold is 40%, but it may happen in extreme setups).
	 * 2. The cache reads the data from the disk.
	 * 3. The application reads the data from the cache.
	 *
	 * If write to a file:
	 * 1. If the cache is full of dirty pages, write back the dirty pages to give room to the data write to cache
	 * (this case is rare because in Linux the dirty page ratio threshold is 40%, but it may happen in extreme setups).
	 * 2. The application writes to the cache.
	 * 3. If the cache reaches the dirty page threshold, write back to the disk.
	 */

	// The following two functions performs the memory access.
	// diskread is the data ranges you need to read from the disk.
	// diskwrite is the data ranges you need to write to the disk.
	// For write operation, diskread is NULL.
	if(filereqpkt->getRead()){
		fr->diskread = cache->readCache(req_pagerange);
	}
	else{
		cache->writeCache(req_pagerange);
		fr->diskread = NULL;
	}
	fr->diskwrite = cache->flushCache();

	fr->readpr = false;
	fr->curaccess = fr->diskwrite; // No matter read-op or write-op, always write back first.
	dispatchNextDiskReq(fr);
}

/*
 * Check which step you are at and transit to the next step.
 * If finished, return true, otherwise, return false.
 */
bool LocalFS::checkStep(filereq_type * fr){
	if(fr->curaccess == NULL){
		if((!fr->readpr) && (fr->diskread != NULL)){ // Write-back step of read operation done, go to the read step.
			fr->readpr = true;
			fr->curaccess = fr->diskread;
			return false;
		}
		else{ // Done.
			if(fr->accessingCache) {
				// The cache access has not finished yet. The finish of the cache access will trigger the reply message.
			}
			else{
				// pass the result back to the dataserver daemon. Clean everything.
				gPacket * gpkt = fileReqQ->popOsQ(fr->fileReqId);
				gpkt->setKind(LFILE_RESP);
				fileReqList->remove(fr);
				delete fr;
				gpkt->setFinishtime(SIMTIME_DBL(simTime()));
				sendToDSD(gpkt);
				dispatchNextFileReq();
			}
			return true;
		}
	}
	return false;
}

void LocalFS::dispatchNextDiskReq(filereq_type * fr){
	if(checkStep(fr))
		return;
	gPacket * diskreq = NULL;
	diskreq = new gPacket("DiskRequest");
	diskreq->setId(fr->fileReqId * MAX_PR_PER_REQ + fr->blkReqId); // The disk requests must have IDs to distinguish.
	fr->blkReqId ++;
	diskreq->setKind(BLK_REQ);
	// Disk access is also in the unit of KB.
	diskreq->setHighoffset(fr->curaccess->start / LOWOFFSET_RANGE);
	diskreq->setLowoffset(fr->curaccess->start % LOWOFFSET_RANGE);
	diskreq->setSize(fr->curaccess->end - fr->curaccess->start);
	diskreq->setRead(fr->curaccess->modified?0:1);
	sendToDisk(diskreq);

	fr->curaccess = fr->curaccess->next;
}

void LocalFS::handleBlkResp(gPacket * resp){
	// The response comes back.
	filereq_type * fr = findFileReq(resp->getId()/MAX_PR_PER_REQ);
	if(fr == NULL)
		return;
	delete resp;
	dispatchNextDiskReq(fr);
}

void LocalFS::sendToDisk(gPacket * req){
	send(req, "disk$o");
}

void LocalFS::sendToDSD(gPacket * req){
	send(req, "dsd$o");
}

void LocalFS::handleCacheAccessFinish(gPacket * gpkt){
	filereq_type * fr = findFileReq(gpkt->getId());
	if(fr == NULL)
		return;
	fr->accessingCache = false;
	if(fr->diskread == NULL && fr->diskwrite == NULL){ // This request contains cache access only. Send the response.
		fileReqQ->popOsQ(gpkt->getId());
		gpkt->setKind(LFILE_RESP);
		gpkt->setFinishtime(SIMTIME_DBL(simTime()));
		sendToDSD(gpkt);
		dispatchNextFileReq();
	}
	// Otherwise, the response is triggered by the disk access results.
}

LocalFS::~LocalFS() {
}
