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
 * The LFS_M translates the application-level {app_ID, offset} to the block-level {offset}
 * and manages a cache module. It also direct the request to the Disk if there's a cache-miss.
 */

#include "dserver/lfs/LFS_M.h"

Define_Module(LFS_M);

int LFS_M::idInit = 0;
int LFS_M::nbDisk = 0;

LFS_M::LFS_M() {
}

void LFS_M::initialize(){
	int page_size = par("page_size").longValue();
	int blk_size = par("blk_size").longValue();
	int degree = par("degree").longValue();
	long long disk_size = par("disk_size").longValue();

	const char * fsName = par("fsname").stringValue();
	int new_ext_size = par("new_ext_size").longValue();
	int new_ext_gap = par("new_ext_gap").longValue();
	const char * inextpath = par("ext_in_path_prefix").stringValue();
	const char * outextpath = par("ext_out_path_prefix").stringValue();
	//In order to rebuild simulation and parse the correct files
	if(nbDisk == 0){//Parse only one time cause it's a static
	    nbDisk = par("numDservers").longValue();//Suppose that there is 1 Disk per Server, If not change the omnet.ini and parse the correct value
	}
	if(idInit >= nbDisk){
	    idInit = 0;
	}
	if(!strcmp(fsName, "ext3"))
		lfs = new EXT3(idInit, degree, disk_size, page_size, blk_size, inextpath, outextpath, new_ext_size, new_ext_gap);
	else
		PrintError::print("LFS_M", string("Sorry, file system type ")+fsName+" is not supported.");

	idInit ++;
}

void LFS_M::handleMessage(cMessage * cmsg){
	/*
	 * 			    PAGE_REQ >     	        BLK_REQ >
	 *  DiskCache <------------> LFS_M <-------------> Disk
	 *             < PAGE_RESP              < BLK_RESP
	 */
	switch(cmsg->getKind()){
	case BLK_REQ:
		handlePageReq((PageRequest *)cmsg);
		break;
	case BLK_RESP:
		handleBlkResp((BlkRequest *)cmsg);
		break;
	default:
		char sentence[50];
		sprintf(sentence, "Unknown message type %d.", cmsg->getKind());
		PrintError::print("LFS_M", sentence);
		break;
	}
}

// Currently filesystem-specific data layout is not supported.
void LFS_M::handlePageReq(PageRequest * req){
#ifdef LFS_DEBUG
		cout << "LFS_M: handlePageReq, ID[" << req->getID() << "], SubID[" << req->getSubID() << "]" << endl;
		fflush(stdout);
#endif
	lfs->newReq(req);

	if(req->getPageEnd() - req->getPageStart() <= 0){ // Check
		PrintError::print("LFS_M", "coming pagereq length <= 0", req->getPageEnd() - req->getPageStart());
		return;
	}
	dispatchNextDiskReq();
}

void LFS_M::dispatchNextDiskReq(){
	BlkRequest * diskreq;
	while((diskreq = lfs->dispatchNext())!=NULL){
		if(diskreq->getBlkEnd() - diskreq->getBlkStart() <= 0){ // Check
			PrintError::print("LFS_M", "dispatched diskreq length <= 0", diskreq->getBlkEnd() - diskreq->getBlkStart());
			cerr << "ID=" << diskreq->getID() << ", start[" << diskreq->getBlkStart() << "], end["
					<< diskreq->getBlkEnd() << "]" << endl;
			return;
		}

		sendToDisk(diskreq);
#ifdef LFS_DEBUG
		cout << "LFS_M: LFS_M->Disk, ID[" << diskreq->getID() << "], SubID[" << diskreq->getSubID() <<
				"], blkstart[" << diskreq->getBlkStart() << "], blkend[" << diskreq->getBlkEnd() << "]." << endl;
		fflush(stdout);
#endif
	}
}

void LFS_M::handleBlkResp(BlkRequest * blkresp){
#ifdef LFS_DEBUG
	cout << "LFS_M: handleblkResp. ID [" << blkresp->getID() << "], SubID[" << blkresp->getSubID() << "]" << endl;
	fflush(stdout);
#endif
	PageRequest * pagereq = lfs->finishedReq(blkresp);
	if(pagereq != NULL){
		if(pagereq->getODIRECT())
			sendToVFS(pagereq);
		else
			sendToDiskCache(pagereq);
	}
	dispatchNextDiskReq();
}

void LFS_M::sendToDiskCache(PageRequest * req){
#ifdef LFS_DEBUG
		cout << "LFS_M: LFS_M->DiskCache, ID[" << req->getID() << "], SubID[" << req->getSubID() << "]." << endl;
		fflush(stdout);
#endif
	send(req, "diskcache$o");
}

void LFS_M::sendToDisk(BlkRequest * req){
	send(req, "disk$o");
}

void LFS_M::sendToVFS(PageRequest * req){
#ifdef LFS_DEBUG
		cout << "LFS_M: LFS_M->VFS, ID[" << req->getID() << "], SubID[" << req->getSubID() << "]." << endl;
		fflush(stdout);
#endif
	send(req, "vfs$o");
}

void LFS_M::finish(){
#ifdef LFS_DEBUG
	cout << "LFS_M - finish." << endl;
	fflush(stdout);
#endif
	if(lfs != NULL)
		delete lfs;
	cout << "LFS_M finish end." << endl;
	fflush(stdout);
}

LFS_M::~LFS_M() {
}
