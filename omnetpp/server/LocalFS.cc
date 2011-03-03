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

#include "LocalFS.h"

Define_Module(LocalFS);

LocalFS::LocalFS() {
}

void LocalFS::initialize(){

}

void LocalFS::handleMessage(cMessage * cmsg){
	/*
	 * 			    LFS_REQ >   	       DATA_REQ >
	 *  DSdaemon <------------> LocalFS <-------------> Disk
	 *             < LFS_RESP             < DATA_RESP
	 */
	gPacket * gpkt = (gPacket *)cmsg;
	switch(gpkt->getKind()){
	case LFILE_REQ:
		handleLFileReq(gpkt);
		break;
	case BLK_RESP:
		handleBlkResp(gpkt);
		break;
	}
}

void LocalFS::handleLFileReq(gPacket * req){
	long long dataoffset = req->getApp() * APP_DATA_RANGE +
			req->getHighoffset() * LOWOFFSET_RANGE +
			req->getLowoffset();
	req->setHighoffset(dataoffset / LOWOFFSET_RANGE);
	req->setLowoffset(dataoffset % LOWOFFSET_RANGE);
	req->setKind(BLK_REQ);
	if(!checkCache(req))
		sendToDisk(req);
}

void LocalFS::handleBlkResp(gPacket * resp){
	long long fileoffset = resp->getHighoffset() * LOWOFFSET_RANGE +
				resp->getLowoffset() - resp->getApp() * APP_DATA_RANGE;
	resp->setHighoffset(fileoffset/LOWOFFSET_RANGE);
	resp->setLowoffset(fileoffset%LOWOFFSET_RANGE);
	resp->setKind(LFILE_RESP);
	sendToDSD(resp);
}

bool LocalFS::checkCache(gPacket * gpkt){
	// TODO: implement the mapping.
	return false;
}

void LocalFS::sendToDisk(gPacket * req){
//	cChannel * cch = gate("disk$o")->getTransmissionChannel();
//	if(cch->isBusy())
//		sendDelayed(req, cch->getTransmissionFinishTime() - simTime(), "disk$o");
//	else
		send(req, "disk$o");
}

void LocalFS::sendToDSD(gPacket * req){
//	cChannel * cch = gate("dsd$o")->getTransmissionChannel();
//	if(cch->isBusy())
//		sendDelayed(req, cch->getTransmissionFinishTime() - simTime(), "dsd$o");
//	else
		send(req, "dsd$o");
}

LocalFS::~LocalFS() {
}
