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

// The Disk module has to communicate with the Disksim process through TCP.
#include "dserver/disk/Disk.h"

Define_Module(Disk);

int Disk::idInit = 0;

Disk::Disk() {
}

void Disk::initialize(){
	degree = par("degree").longValue();
	disk_r_speed = par("disk_r_speed").doubleValue();
	disk_w_speed = par("disk_w_speed").doubleValue();

	myId = idInit ++;

	queue = new FIFO(12345, degree);
}

void Disk::handleMessage(cMessage *cmsg){
	/*
	 *           DATA_REQ >
	 * LocalFS ------------- Disk
	 *          < DATA_RESP
	 */
	switch(cmsg->getKind()){
	case BLK_REQ:
		handleBlockReq((DiskRequest *)cmsg);
		break;
	case BLK_RESP:
		handleBlockResp((DiskRequest *)cmsg);
		break;
	}
}

void Disk::handleBlockReq(DiskRequest * datareq){
	queue->pushWaitQ(datareq);
	dispatchJobs();
}

void Disk::handleBlockResp(DiskRequest * dataresp){
	if(queue->popOsQ(dataresp->getID()) == NULL){
		fprintf(stderr, "[ERROR] Disk: handleBlockResp cannot find request #%ld from the OsQ.\n", dataresp->getID());
		fflush(stderr);
	}
	sendSafe(dataresp);
	dispatchJobs();
}

#define MAX_BLK_BATCH 128

void Disk::dispatchJobs(){
	DiskRequest * req;
	while(1){
		if((req = (DiskRequest *)queue->dispatchNext()) == NULL)
			break;
		req->setName("BLK_RESP");
		req->setKind(BLK_RESP);
		int length = req->getBlkEnd() - req->getBlkStart();
		if(length > MAX_BLK_BATCH || length <= 0 || req->getBlkStart() > MAX_DISK_OFFSET || req->getBlkStart() < 0){
			fprintf(stderr, "[ERROR] Disk: Invalid block request to Disksim. [offset: %ld, length: %d]\n",
					syncJob->off, syncJob->len);
			fflush(stderr);
			return;
		}
		if(req->getRead())
			scheduleAt(SIMTIME_DBL(simTime()) + length * disk_r_speed, req);
		else
			scheduleAt(SIMTIME_DBL(simTime()) + length * disk_w_speed, req);
	}
}

void Disk::sendSafe(DiskRequest * req){
	send(req, "g$o");
}

void Disk::finish(){
	free(syncJob);
	free(syncNojob);
	free(syncEnd);
}

int Disk::getID(){
	return myId;
}

Disk::~Disk() {
}
