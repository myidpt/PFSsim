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
 * Currently the DSD_M just adds up some processing delay to the incoming (from eth) packets.
 */

// Message kind conversions:
// Read request comes:
// JOB_REQ -> SELF_JOB_DISP_LAST
// Small write request comes:
// JOB_REQ -> SELF_JOB_DISP_LAST
// Write request comes:
// JOB_REQ -> SELF_JOB_REQ -> JOB_RESP
// Write data comes (not the last one):
// JOB_DISP -> SELF_JOB_DISP
// Write data comes (the last one):
// JOB_DISP_LAST -> SELF_JOB_DISP_LAST

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

void DSD_M::handleMessage(cMessage * cmsg) {
	/*
	 * 			        JOB_DISP >				 LFILE_REQ >
	 *  router/switch <------------> DSD_M <--------------> VirtualFS
	 *                  < JOB_FIN  			 	< LFILE_RESP
	 */
	gPacket * gpkt = (gPacket *)cmsg;
	if(((gPacket *)cmsg)->getClientID() > 1000){
		PrintError::print("DSD_M", "ClientID > 1000", ((gPacket *)cmsg)->getClientID());
		cerr << "!!!Kind=" << ((gPacket *)cmsg)->getKind() << ", ID=" << ((gPacket *)cmsg)->getID() << endl;
		while(1)sleep(5);
	}

	switch(gpkt->getKind()){
	case JOB_REQ: // First-round packets for write or read requests, or small write requests.
//		cout << "time=" << SIMTIME_DBL(simTime()) << " Server #" << myID << " packet #" << gpkt->getID() << " size=" << gpkt->getByteLength() << endl;
		handle_JobReq(gpkt);
		break;
	case SELF_JOB_REQ:
		send_JobResp(gpkt);
		break;

	case JOB_DISP: // Data packet.
	case JOB_DISP_LAST: // Last data packet.
//		cout << "time=" << SIMTIME_DBL(simTime()) << " Server #" << myID << " packet #" << gpkt->getID() << " size=" << gpkt->getByteLength() << endl;
		handle_JobDisp(gpkt);
		break;
	case SELF_JOB_DISP:
	case SELF_JOB_DISP_LAST:
		enqueue_dispatch_VFSReqs(gpkt);
		break;

	case LFILE_RESP:
		handle_VFSResp(gpkt);
		break;
	case JOB_FIN:
	case JOB_FIN_LAST:
		sendToEth(gpkt);
		break;
	}
}

void DSD_M::handle_JobReq(gPacket * gpkt){
#ifdef DSD_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": handle_JobReq. New Arrived Job #" << gpkt->getID() << endl;
	fflush(stdout);
#endif

	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));
	if(gpkt->getSize() < small_io_size_threshold && !gpkt->getRead()){ // Small write packet, you need to process both the metadata and the data.
		gpkt->setKind(SELF_JOB_DISP_LAST);
		if(write_data_proc_time + write_metadata_proc_time != 0 && gpkt->getSize() < small_io_size_threshold )
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + write_data_proc_time + write_metadata_proc_time), gpkt);
		else
			enqueue_dispatch_VFSReqs(gpkt);
	}
	else if(gpkt->getRead()){ // Read request packet, you need to process the read it triggers and return all of them.
		gpkt->setKind(SELF_JOB_DISP_LAST);
		if(read_metadata_proc_time != 0) // Consider the delay.
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + read_metadata_proc_time), gpkt);
		else
			enqueue_dispatch_VFSReqs(gpkt);
	}
	else{ // Write request packet. Return it, it does not trigger any writes it self (but the following packets do).
		gpkt->setKind(SELF_JOB_REQ);
//		if(!gpkt->getRead()){
		if(write_metadata_proc_time != 0) // Consider the delay.
			scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + write_metadata_proc_time), gpkt);
		else
			send_JobResp(gpkt);
//		}else{
//			if(read_metadata_proc_time != 0) // Consider the delay.
//				scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + read_metadata_proc_time), gpkt);
//			else
//				sendJobResp(gpkt);
//		}
	}
}

void DSD_M::send_JobResp(gPacket * gpkt){
	gpkt->setDispatchtime(SIMTIME_DBL(simTime()));
	gpkt->setFinishtime(SIMTIME_DBL(simTime()));
	gpkt->setKind(JOB_RESP);
	sendToEth(gpkt);
}

void DSD_M::handle_JobDisp(gPacket * gpkt){
#ifdef DSD_DEBUG
//	if(myID == 4)
	cout << "[" << SIMTIME_DBL(simTime()) << "] DSD_M#" << myID << ": New Arrived Job #" << gpkt->getID() << endl;
	fflush(stdout);
#endif
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));

	if(gpkt->getKind() == JOB_DISP) // Write not last one.
		gpkt->setKind(SELF_JOB_DISP);
	else if(gpkt->getKind() == JOB_DISP_LAST) // Write last one.
		gpkt->setKind(SELF_JOB_DISP_LAST);

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
