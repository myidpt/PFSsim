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

#include "dserver/lfs/LocalFS.h"

Define_Module(LocalFS);

LocalFS::LocalFS() {
}

void LocalFS::initialize(){
	page_size = par("page_size").longValue();
	blk_size = par("blk_size").longValue();
    degree  = par("degree").longValue();
	fileReqQ = new FIFO(12345, degree);
	diskIOs = new map<long, PageRequest *>();
}

void LocalFS::handleMessage(cMessage * cmsg){
	/*
	 * 			   LFILE_REQ >   	       BLK_REQ >
	 *  DSdaemon <------------> LocalFS <-------------> Disk
	 *            < LFILE_RESP            < BLK_RESP
	 *                                     PAGE_REQ >
	 *                          LocalFS <-------------> DiskCache
	 *                                    < PAGE_RESP
	 */
	switch(cmsg->getKind()){
	case LFILE_REQ:
		handleNewFileReq((gPacket *)cmsg);
		break;
	case BLK_REQ:
		handleDiskReqFromCache((PageRequest *)cmsg);
		break;
	case PAGE_RESP:
		handlePageResp((PageRequest *)cmsg);
		break;
	case BLK_RESP:
		handleBlkResp((DiskRequest *)cmsg);
		break;
	}
}

void LocalFS::handleNewFileReq(gPacket * req){
#ifdef DEBUG
	printf("handleNewFileReq: %ld, %ld, %d, %d.\n", req->getID(), req->getHighoffset() * LOWOFFSET_RANGE + req->getLowoffset(), req->getSize(), req->getRead());
	fflush(stdout);
#endif
	fileReqQ->pushWaitQ(req);

	dispatchNextFileReq();
}

// Currently filesystem-specific data layout is not supported.
void LocalFS::handleDiskReqFromCache(PageRequest * req){
	// Get the position on disk.
	// Do a simple layout: contiguous.
	diskIOs->insert(std::pair<long, PageRequest *>(req->getID(), req));
	DiskRequest * diskreq = new DiskRequest("BLK_REQ");
	diskreq->setKind(BLK_REQ);
	diskreq->setID(req->getID());
	diskreq->setFileId(req->getFileId());
	diskreq->setBlkStart(req->getFileId()*1024*100 + req->getPageStart()); // 400M file size on disk.
	diskreq->setBlkEnd(req->getFileId()*1024*100 + req->getPageEnd());
	diskreq->setRead(req->getRead());
	sendToDisk(diskreq);
}

void LocalFS::handlePageResp(PageRequest * resp){
	gPacket * fileresp = (gPacket *)fileReqQ->popOsQ(resp->getID());
	fileresp->setFinishtime(SIMTIME_DBL(simTime()));
	fileresp->setName("LFILE_RESP");
	fileresp->setKind(LFILE_RESP);
	sendToDSD(fileresp);
	delete resp;
}

void LocalFS::handleBlkResp(DiskRequest * resp){
	map<long, PageRequest *>::iterator it = diskIOs->find(resp->getID());
	if(it == diskIOs->end()){
		fprintf(stderr, "[ERROR] LocalFS: can not find PageRequest #%ld from diskIOs.\n", resp->getID());
		fflush(stderr);
		return;
	}
	PageRequest * pageresp = it->second;
	diskIOs->erase(it);
	pageresp->setName("BLK_RESP");
	pageresp->setKind(BLK_RESP);
	sendToDiskCache(pageresp);
	delete resp;
}

void LocalFS::dispatchNextFileReq(){
	gPacket * filereqpkt = (gPacket *)fileReqQ->dispatchNext();
	if(filereqpkt == NULL) // Currently exceeds the degree.
		return;

	filereqpkt->setDispatchtime(SIMTIME_DBL(simTime()));
	if(filereqpkt->getRead()) // If it is read, we need to simulate the packet length when it returns.
		filereqpkt->setByteLength(100 + filereqpkt->getSize());

	PageRequest * pagereq = new PageRequest("PAGE_REQ");
	pagereq->setKind(PAGE_REQ);
	long pagestart = (filereqpkt->getHighoffset() * LOWOFFSET_RANGE + filereqpkt->getLowoffset()) / page_size;

	long pageend = (filereqpkt->getHighoffset() * LOWOFFSET_RANGE + filereqpkt->getLowoffset() +
			filereqpkt->getSize()) / page_size;
	if((filereqpkt->getHighoffset() * LOWOFFSET_RANGE + filereqpkt->getLowoffset() +
			filereqpkt->getSize()) % page_size > 0) // Remainder exists
			pageend ++;
	pagereq->setID(filereqpkt->getID());
	pagereq->setPageStart(pagestart);
	pagereq->setPageEnd(pageend);
	pagereq->setRead(filereqpkt->getRead());
	pagereq->setFileId(filereqpkt->getFileId());
#ifdef DEBUG
	printf("LocalFS->DiskCache------------------%ld, %ld.\n", pagereq->getPageStart(), pagereq->getPageEnd());
	fflush(stdout);
#endif

	sendToDiskCache(pagereq);
}


void LocalFS::sendToDisk(DiskRequest * req){
	send(req, "disk$o");
}

void LocalFS::sendToDSD(gPacket * req){
	send(req, "dsd$o");
}

void LocalFS::sendToDiskCache(PageRequest * req){
	send(req, "diskcache$o");
}

void LocalFS::finish(){
	free(fileReqQ);
	free(diskIOs);
}

LocalFS::~LocalFS() {
}
