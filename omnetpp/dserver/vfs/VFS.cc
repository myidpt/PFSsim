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

#include "dserver/vfs/VFS.h"

Define_Module(VFS);

VFS::VFS() {
}

void VFS::initialize(){
	page_size = par("page_size").longValue();
	degree = par("degree").longValue();
	fileReqQ = new FIFO(12345, degree);
	pageReqQ = new FIFO(12345, 1000); // We don't control the pageReqQ.
}

void VFS::handleMessage(cMessage * cmsg){
	/*
	 * 			   LFILE_REQ >   	         PAGE_REQ >
	 *  DSdaemon <------------> VFS <-------------> DiskCache
	 *            < LFILE_RESP              < PAGE_RESP
	 */

	switch(cmsg->getKind()){
	case LFILE_REQ:
		if(((gPacket *)cmsg)->getClientID() > 1000){
			PrintError::print("VFS", "ClientID > 1000", ((gPacket *)cmsg)->getClientID());
			cerr << "Kind=" << ((gPacket *)cmsg)->getKind() << ", ID=" << ((gPacket *)cmsg)->getID() << endl;
			while(1)sleep(5);
		}
		handleNewFileReq((gPacket *)cmsg);
		break;
	case PAGE_RESP:
		handlePageResp((PageRequest *)cmsg);
		break;
	}
}

void VFS::handleNewFileReq(gPacket * req){
#ifdef VFS_DEBUG
	cout << "VFS: {handleNewFileReq} ID[" << req->getID() << "], SubID[" << req->getSubID()
			<< "], offset[" << req->getLowoffset() << "], size[" << req->getSize() << "], read[" << req->getRead() << "]." << endl;
	fflush(stdout);
#endif
	if(req->getSize() <= 0){
		PrintError::print("VFS", "gPacket size <= 0", req->getSize());
		return;
	}
	fileReqQ->pushWaitQ(req);
	dispatchPageReqs();
}

void VFS::handlePageResp(PageRequest * resp){
#ifdef VFS_DEBUG
	cout << "VFS: {handlePageResp} ID[" << resp->getID() << "], SubID[" << resp->getSubID() << "], startpage[" << resp->getPageStart()
			<< "], endpage[" << resp->getPageEnd() << "], read[" << resp->getRead() << "]." << endl;
	fflush(stdout);
#endif
	if(pageReqQ->popOsQ(resp->getID(), resp->getSubID()/VFS_SUB_ID_BASE*VFS_SUB_ID_BASE) == NULL){ // Must see the return.
		PrintError::print("VFS", "Cannot find the page request back. ID=", resp->getSubID());
		return;
	}
	if(resp->getSubID() / VFS_SUB_ID_BASE % 10 != 0){ // This bit is not zero. A read triggered by a write comes back.
		gPacket * fileresp = (gPacket *)fileReqQ->queryJob(resp->getID()); // Find the original request.
		fileresp->setSubReqNum(fileresp->getSubReqNum() - 1);

		if(fileresp->getSubReqNum() == 0){ // We need to issue the write request finally.
			double dstart = (double)LOWOFFSET_RANGE / (double)page_size * (double)fileresp->getHighoffset()
					+ (double)fileresp->getLowoffset() / (double)page_size;
			double dend = dstart + (double)(fileresp->getSize()) / page_size;
			long pagestart = dstart;
			long pageend = dend;

			PageRequest * pagereq = new PageRequest("PAGE_REQ", PAGE_REQ);
			pagereq->setID(fileresp->getID());
			pagereq->setSubID(fileresp->getSubID());
			pagereq->setPageStart(pagestart);
			pagereq->setPageEnd(pageend);
			pagereq->setRead(fileresp->getRead());
			pagereq->setFileId(fileresp->getFileId());
			pageReqQ->pushWaitQ(pagereq);
#ifdef VFS_DEBUG
			cout << "VFS: {push pageReqQ} The original request. ID[" << pagereq->getID() << ", SubID[" << pagereq->getSubID() << "], pagestart["
					<< pagereq->getPageStart() << "], pageend[" << pagereq->getPageEnd() << "], read[" << pagereq->getRead() << "]." << endl;
			fflush(stdout);
#endif
		}
	}else{ // Finished file request.
		gPacket * fileresp = (gPacket *)fileReqQ->popOsQ(resp->getID()); // Pop out the file request.
		if(fileresp == NULL){
			PrintError::print("VFS", "Cannot find the file response from fileReqQ. ID=", resp->getID());
		}
		fileresp->setName("LFILE_RESP");
		fileresp->setKind(LFILE_RESP);
#ifdef VFS_DEBUG
		cout << "VFS: {send back fileresp} The original request. ID[" << fileresp->getID() << "], SubID["
				<< fileresp->getSubID() << "], size[" << fileresp->getSize() << "]." << endl;
		fflush(stdout);
#endif
		sendToDSD(fileresp);
	}
	delete resp;

	dispatchPageReqs();
}

void VFS::dispatchPageReqs(){
	dispatchNextFileReq();
	PageRequest * pgreq;
	while(1){
		pgreq = (PageRequest *)pageReqQ->dispatchNext();
		if(pgreq == NULL)
			break;
		sendToDiskCache(pgreq);
	}
}

// If the request is a write, and it involves writing to A PORTION of a page, it should firstly conduct reads to these pages.
// When these pages are read, the write request is issued.
// Otherwise, it just issue the write to the target pages.
void VFS::dispatchNextFileReq(){
	gPacket * filereqpkt = (gPacket *)fileReqQ->dispatchNext();
	if(filereqpkt == NULL) // Currently exceeds the degree.
		return;

//	long long offset = (long long)req->getHighoffset() * LOWOFFSET_RANGE + (long long)req->getLowoffset();
	double dstart = (double)LOWOFFSET_RANGE / (double)page_size * (double)filereqpkt->getHighoffset()
			+ (double)filereqpkt->getLowoffset() / (double)page_size;
	double dend = dstart + (double)(filereqpkt->getSize()) / page_size;
	long pagestart = dstart;
	long pageend = dend;

	int subreqnum = 0;
	// If you write to only A PORTION OF a page, you need to conduct read on that page first.
	// This may happen if the start offset or the end offset can not be divided by the page size.
	if(!filereqpkt->getRead() && dstart - pagestart > 0.0000001){ // Write and beginning has skew.
		PageRequest * r_for_w = new PageRequest("PAGE_REQ", PAGE_REQ);
		r_for_w->setID(filereqpkt->getID());
		r_for_w->setSubID(1 * VFS_SUB_ID_BASE); // 1 -> front page skew read
		r_for_w->setPageStart(pagestart);
		r_for_w->setPageEnd(pagestart + 1);
		r_for_w->setFileId(filereqpkt->getFileId());
		r_for_w->setRead(true);
		pageReqQ->pushWaitQ(r_for_w);
#ifdef VFS_DEBUG
		cout << "VFS: {push pageReqQ} Beginning has skew, read the beginning block. ID[" << r_for_w->getID() << "], SubID[" << r_for_w->getSubID() <<
				"], pagestart[" << r_for_w->getPageStart() << "], pageend[" << r_for_w->getPageEnd() << "]." << endl;
		fflush(stdout);
#endif
		/*
		sendToDiskCache(r_for_w);
		*/
		subreqnum ++;
	}

	if(dend - pageend > 0.0000001){ // Last page has skew.
		pageend ++;
		if(!filereqpkt->getRead()){ // Write and end has skew.
			PageRequest * r_for_w2 = new PageRequest("PAGE_REQ", PAGE_REQ);
			r_for_w2->setID(filereqpkt->getID());
			r_for_w2->setSubID(2 * VFS_SUB_ID_BASE); // 2 -> last page skew read
			r_for_w2->setPageStart(pageend - 1);
			r_for_w2->setPageEnd(pageend);
			r_for_w2->setFileId(filereqpkt->getFileId());
			r_for_w2->setRead(true);
			pageReqQ->pushWaitQ(r_for_w2);
#ifdef VFS_DEBUG
			cout << "VFS: {push pageReqQ} End has skew, read the end block. ID[" << r_for_w2->getID() << "], SubID[" << r_for_w2->getSubID() <<
					"], pagestart[" << r_for_w2->getPageStart() << "], pageend[" << r_for_w2->getPageEnd() << "]." << endl;
			fflush(stdout);
#endif
		/*
		sendToDiskCache(r_for_w2);
		*/
			subreqnum ++;
		}
	}
	// TODO: subreqnum is not necessary.
	filereqpkt->setSubReqNum(subreqnum); // Update the subreqnum: it can be 0, 1, 2.
	if(subreqnum == 0){ // Read or write without skew.
		PageRequest * pagereq = new PageRequest("PAGE_REQ", PAGE_REQ);
		pagereq->setID(filereqpkt->getID());
		pagereq->setSubID(filereqpkt->getSubID());
		pagereq->setPageStart(pagestart);
		pagereq->setPageEnd(pageend);
		pagereq->setRead(filereqpkt->getRead());
		pagereq->setFileId(filereqpkt->getFileId());
		pageReqQ->pushWaitQ(pagereq);
#ifdef VFS_DEBUG
		cout << "VFS: {push the orig pageReqQ} The original request. ID[" << pagereq->getID() << "], SubID[" << pagereq->getSubID() << "], pagestart["
				<< pagereq->getPageStart() << "], pageend[" << pagereq->getPageEnd() << "], read[" << pagereq->getRead() << "]." << endl;
		fflush(stdout);
#endif
//		sendToDiskCache(pagereq);
	}
}

void VFS::sendToDSD(gPacket * req){
	send(req, "dsd$o");
}

void VFS::sendToDiskCache(PageRequest * req){
#ifdef	VFS_DEBUG
	printf("VFS: {sendToDiskCache} ID[%ld], SubID[%ld], pagestart[%ld], pageend[%ld], read[%d].\n",
			req->getID(), req->getSubID(), req->getPageStart(), req->getPageEnd(), req->getRead());
	fflush(stdout);
#endif
	send(req, "diskcache$o");
}

void VFS::finish(){
#ifdef VFS_DEBUG
	cout << "VFS - finish." << endl;
	fflush(stdout);
#endif
	delete fileReqQ;
	delete pageReqQ;
}

VFS::~VFS() {
}
