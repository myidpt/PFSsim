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

#include "dserver/dsd/DSD_M.h"

int DSD_M::idInit = 0;

Define_Module(DSD_M);
DSD_M::DSD_M() {
}

void DSD_M::initialize(){
	myID = idInit ++;
	write_data_proc_time = par("write_data_proc_time").doubleValue();
	parallel_job_proc_time = par("parallel_job_proc_time").doubleValue();
	write_metadata_proc_time = par("write_metadata_proc_time").doubleValue();
	read_metadata_proc_time = par("read_metadata_proc_time").doubleValue();
    small_io_size_threshold = par("small_io_size_threshold").doubleValue();
    O_DIRECT = par("O_DIRECT").longValue();

	const char * pfsName = par("pfsname").stringValue();
	int degree = par("degree").longValue();
	int obj_size = par("object_size").longValue();
	int max_subreq_size = par("max_subreq_size").longValue();

	if(!strcmp(pfsName, "pvfs2")){
		dsd = new PVFS2DSD(myID, degree, obj_size, max_subreq_size);
	}else{
		PrintError::print("DSD_M", string("Sorry, parallel file system type ")+pfsName+" is not supported.");
		deleteModule();
	}
}

/*
 * Receive from Ethernet:
 * PFS_W_REQ, PFS_R_REQ, PFS_W_DATA, PFS_W_DATA_LAST
 * Inside:
 * SELF_PFS_W_REQ, SELF_PFS_W_DATA, SELF_PFS_W_DATA_LAST, PFS_R_DATA, PFS_R_DATA_LAST, PFS_W_FIN
 * Receive from VFS:
 * LFILE_RESP
 */
void DSD_M::handleMessage(cMessage * cmsg) {
	gPacket * gpkt = (gPacket *)cmsg;
	if(((gPacket *)cmsg)->getClientID() > 1000){
		PrintError::print("DSD_M", "ClientID > 1000", ((gPacket *)cmsg)->getClientID());
	}
#ifdef MSG_CLIENT
	cout << "DSD[" << myID << "] " << MessageKind::getMessageKindString(gpkt->getKind()) << " ID=" << gpkt->getID() << endl;
#endif
	switch(gpkt->getKind()){
	// From Ethernet:
	case PFS_W_REQ: // First-round packets for write or read requests, or small write requests.
	case PFS_R_REQ:
		handleReadWriteReq(gpkt);
		break;

	case PFS_W_DATA:
	case PFS_W_DATA_LAST:
		handleWriteDataPacket(gpkt);
		break;

	// From inside:
	case SELF_PFS_W_REQ:
		handleSelfWriteReq(gpkt);
		break;

	case SELF_PFS_W_DATA:
	case SELF_PFS_W_DATA_LAST:
		enqueue_dispatch_VFSReqs(gpkt);
		break;

	case PFS_R_DATA:
	case PFS_R_DATA_LAST:
	case PFS_W_FIN:
		sendToEth(gpkt);
		break;

	// From VFS:
	case LFILE_RESP:
		handle_VFSResp(gpkt);
		break;

	}
}


void DSD_M::handleReadWriteReq(gPacket * gpkt) {
#ifdef DSD_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": handleReadWriteReq. New Arrived Job #"
			<< gpkt->getID() << endl;
	fflush(stdout);
#endif
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));
	if(gpkt->getSize() < small_io_size_threshold && gpkt->getKind() == PFS_W_REQ){ // Small write packet, you need to process both the metadata and the data.
		gpkt->setKind(SELF_PFS_W_REQ);
		if(write_data_proc_time + write_metadata_proc_time != 0) {
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + write_data_proc_time + write_metadata_proc_time), gpkt);
		} else {
			enqueue_dispatch_VFSReqs(gpkt);
		}
	}
	else if(gpkt->getKind() == PFS_R_REQ) { // Read request packet, you need to process the read it triggers and return all of them.
		gpkt->setKind(SELF_PFS_R_REQ);
		if(read_metadata_proc_time != 0) {// Consider the delay.
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + read_metadata_proc_time), gpkt);
		} else {
			enqueue_dispatch_VFSReqs(gpkt);
		}
	}
	else{ // Write request packet. Return it, it does not trigger any writes it self (but the following packets do).
		gpkt->setKind(SELF_PFS_W_REQ);
		if(write_metadata_proc_time != 0) { // Consider the delay.
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + write_metadata_proc_time), gpkt);
		} else {
			sendWriteResp(gpkt);
		}
	}
}

void DSD_M::handleSelfWriteReq(gPacket * gpkt) {
	if(gpkt->getSize() < small_io_size_threshold && gpkt->getKind() == PFS_W_REQ) { // Small write packet.
		enqueue_dispatch_VFSReqs(gpkt);
	} else { // big write request
		sendWriteResp(gpkt);
	}
}

void DSD_M::sendWriteResp(gPacket * gpkt){
	gpkt->setDispatchtime(SIMTIME_DBL(simTime()));
	gpkt->setFinishtime(SIMTIME_DBL(simTime()));
	gpkt->setKind(PFS_W_RESP);
	sendToEth(gpkt);
}

void DSD_M::handleWriteDataPacket(gPacket * gpkt){
#ifdef DSD_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": handleWriteDataPacket packet #"
			<< gpkt->getID() << endl;
	fflush(stdout);
#endif
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));

	if(gpkt->getKind() == PFS_W_DATA) { // Write not last one.
		gpkt->setKind(SELF_PFS_W_DATA);
	} else if(gpkt->getKind() == PFS_W_DATA_LAST) {
		gpkt->setKind(SELF_PFS_W_DATA_LAST);
	}

	if(write_data_proc_time != 0) // Consider the delay.
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + write_data_proc_time), gpkt);
	else
		enqueue_dispatch_VFSReqs(gpkt);
}

void DSD_M::enqueue_dispatch_VFSReqs(gPacket * gpkt){
	dsd->newReq(gpkt);
	dispatch_VFSReqs();
}

void DSD_M::dispatch_VFSReqs(){
	gPacket * jobtodispatch = NULL;
	while(1){
		jobtodispatch = dsd->dispatchNext();
		if(jobtodispatch != NULL){
#ifdef DSD_DEBUG
//			if(myID == 4)
			cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": dispatch Job #" << jobtodispatch->getID() << " off = " << jobtodispatch->getLowoffset()
					<< ", size = " << jobtodispatch->getSize() << std::endl;
			fflush(stdout);
#endif
			jobtodispatch->setKind(LFILE_REQ);
			jobtodispatch->setDispatchtime(SIMTIME_DBL(simTime()));
			jobtodispatch->setODIRECT(O_DIRECT);
			sendToVFS(jobtodispatch);
		}else
			break;
	}
}

void DSD_M::handle_VFSResp(gPacket * gpkt){
#ifdef DSD_DEBUG
//	if(myID == 4)
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": handle_VFSResp. ID=" << gpkt->getID() << ", subID="
			<< gpkt->getSubID() << ", size=" << gpkt->getSize() << std::endl;
	fflush(stdout);
#endif
	gpkt = dsd->finishedReq(gpkt); // Note: gpkt's Kind is changed inside dsd.

	if(gpkt != NULL){ // You have packet to be sent back to client.
		gpkt->setFinishtime(SIMTIME_DBL(simTime()));

		if(gpkt->getRead()) // If it is read, we need to simulate the packet length when it returns.
			gpkt->setByteLength(DATA_HEADER_LENGTH + gpkt->getSize());
		else
			gpkt->setByteLength(DATA_HEADER_LENGTH);

		if(parallel_job_proc_time != 0 && (gpkt->getDispatchtime() + parallel_job_proc_time > SIMTIME_DBL(simTime()))){
			// Parallel process delay, wait for the process delay if the current time is still early.
			scheduleAt((simtime_t)(gpkt->getDispatchtime() + parallel_job_proc_time), gpkt);
		}
		else{
			sendToEth(gpkt);
		}
	}

	dispatch_VFSReqs();
}

void DSD_M::sendToVFS(gPacket * gpkt){
	if(gpkt->getClientID() > 1000){
		PrintError::print("DSD_M", "ClientID > 1000", gpkt->getClientID());
		cerr << "To VFS Kind=" << gpkt->getKind() << ", ID=" << gpkt->getID() << endl;
	}
	send(gpkt, "vfs$o");
}

void DSD_M::sendToEth(gPacket * gpkt){
	cChannel * cch = gate("eth$o")->getTransmissionChannel();
	if(gpkt->getClientID() > 1000){
		PrintError::print("DSD_M", "ClientID > 1000", gpkt->getClientID());
		cerr << "Kind=" << gpkt->getKind() << ", ID=" << gpkt->getID() << endl;
	}

	if(cch->isBusy())
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "eth$o");
	else
		send(gpkt, "eth$o");
}

void DSD_M::finish(){
#ifdef DSD_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": finish." << endl;
	fflush(stdout);
#endif
}

DSD_M::~DSD_M(){
	cout << "DSD_M finish." << endl;
	fflush(stdout);
	if(dsd != NULL)
		delete dsd;
	cout << "DSD_M finish end." << endl;
	fflush(stdout);
}
