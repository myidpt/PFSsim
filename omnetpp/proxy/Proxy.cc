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

#include "Proxy.h"

Define_Module(Proxy);

int Proxy::proxyID = 0;

Proxy::Proxy() {
}

void Proxy::initialize(){
	myID = proxyID;
	proxyID ++;

	algorithm = par("algorithm").longValue();
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	int numapps = par("numApps").longValue();

	switch(algorithm){
	case FIFO_ALG:
		queue = new FIFO(myID, degree);
		break;
	case SFQ_ALG:
		queue = new SFQ(myID, degree, numapps);
		break;
	default:
		PrintError::print("DSproxy", "Undefined algorithm.");
		deleteModule();
	}
}

/*
 * We map the packet kinds to the SELF event family for internal use:
 * JOB_DISP			SELF_JOB_DISP
 * JOB_DISP_LAST	SELF_JOB_DISP_LAST
 * JOB_FIN			SELF_JOB_FIN
 * JOB_FIN_LAST		SELF_JOB_FIN_LAST
 * JOB_REQ			SELF_JOB_REQ
 * JOB_RESP			SELF_JOB_RESP
 * They translated back in sendSafe().
 */

void Proxy::handleMessage(cMessage * cmsg) {
	switch(cmsg->getKind()){
	case JOB_REQ: // This one needs to be scheduled.
		handleJobReq((gPacket *)cmsg);
		break;
	case SELF_JOB_REQ:
		handleJobReq2((gPacket *)cmsg);
		break;

	case JOB_DISP: // No need to put in queue.
	case JOB_DISP_LAST:
		handleJobDisp((gPacket *)cmsg);
		break;
	case SELF_JOB_DISP:
	case SELF_JOB_DISP_LAST:
		sendSafe((gPacket *)cmsg);
		break;

	case JOB_RESP:
	case JOB_FIN_LAST:
		handleJobRespFinComp((gPacket *)cmsg);
		break;
	case SELF_JOB_FIN_LAST:
	case SELF_JOB_RESP:
		handleJobRespFinComp2((gPacket *)cmsg);
		break;

	case JOB_FIN: // No need to go through the queue.
		handleJobFin((gPacket *)cmsg);
		break;
	case SELF_JOB_FIN:
		sendSafe((gPacket *)cmsg);
		break;

	case PROP_SCH: // inter-scheduler message, currently unused.
		handleInterSchedulerPacket((sPacket *)cmsg);
		break;
	}
}

void Proxy::handleJobReq(gPacket * gpkt){
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));
	gpkt->setKind(SELF_JOB_REQ);
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobReq ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif
	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		handleJobReq2(gpkt);
	}
}

void Proxy::handleJobReq2(gPacket * gpkt){
	queue->pushWaitQ(gpkt);
	scheduleJobs();
}

// JOB_DISP or JOB_DISP_LAST:
// The packet does not go through the queue.
void Proxy::handleJobDisp(gPacket * gpkt) { // from the client
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));
	if(gpkt->getKind() == JOB_DISP)
		gpkt->setKind(SELF_JOB_DISP);
	else if(gpkt->getKind() == JOB_DISP_LAST)
		gpkt->setKind(SELF_JOB_DISP_LAST);

#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobDisp ID=" << gpkt->getID() << endl;
#endif

	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		sendSafe(gpkt);
	}
}

void Proxy::handleJobRespFinComp(gPacket * gpkt){ // from the data server
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobRespFinComp ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif
	if(gpkt->getKind() == JOB_RESP)
		gpkt->setKind(SELF_JOB_RESP);
	else if(gpkt->getKind() == JOB_FIN_LAST)
		gpkt->setKind(SELF_JOB_FIN_LAST);

	if(finjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	} else {
		handleJobRespFinComp2(gpkt);
	}
}

void Proxy::handleJobRespFinComp2(gPacket * gpkt){
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobRespFinComp2 ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif

	if(gpkt->getKind() == SELF_JOB_RESP)
		gpkt->setKind(JOB_RESP);
	else if(gpkt->getKind() == SELF_JOB_FIN_LAST)
		gpkt->setKind(JOB_FIN_LAST);

	queue->popOsQ(gpkt->getID());
	sendSafe(gpkt); // to the client

	scheduleJobs();
}

void Proxy::handleJobFin(gPacket * gpkt){
	gpkt->setKind(SELF_JOB_FIN);
	if(finjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	} else {
		sendSafe(gpkt);
	}
}

void Proxy::scheduleJobs(){
	gPacket * jobtodispatch = NULL;
	while(1){
		jobtodispatch = (gPacket *)queue->dispatchNext();
		if(jobtodispatch != NULL){
			jobtodispatch->setScheduletime(SIMTIME_DBL(simTime()));
			sendSafe(jobtodispatch);
		}
		else
			break;
	}
}

void Proxy::sendSafe(gPacket * gpkt){
	switch(gpkt->getKind()){
	case SELF_JOB_DISP:
		gpkt->setKind(JOB_DISP);
		break;
	case SELF_JOB_DISP_LAST:
		gpkt->setKind(JOB_DISP_LAST);
		break;
	case SELF_JOB_FIN:
		gpkt->setKind(JOB_FIN);
		break;
	case SELF_JOB_FIN_LAST:
		gpkt->setKind(JOB_FIN_LAST);
		break;
	case SELF_JOB_REQ:
		gpkt->setKind(JOB_REQ);
		break;
	case SELF_JOB_RESP:
		gpkt->setKind(JOB_RESP);
		break;
	}

	cChannel * cch = gate("g$o")->getTransmissionChannel();
	gpkt->setScheduletime(SIMTIME_DBL(simTime()));
	if(cch->isBusy()){
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(gpkt, "g$o");
}

void Proxy::handleInterSchedulerPacket(sPacket * spkt){
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Scheduler #" << myID << ": Receive interscheduler packet. app="
			<< spkt->getApp() << ", length=" << spkt->getLength() << endl;
	fflush(stdout);
#endif
	if(algorithm != DSFQF_ALG && algorithm != DSFQA_ALG && algorithm != DSFQD_ALG){
		PrintError::print("Proxy", "distributed scheduling algorithm not selected, calling handleInterSchedulerPacket is prohibited.");
		return;
	}
	queue->receiveSPacket(spkt);
	delete spkt;
}

void Proxy::propagateSPacket(sPacket * spkt){
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": send interProxy packet. app="
			<< spkt->getApp() << ", length=" << spkt->getLength() << endl;
	fflush(stdout);
#endif
	send(spkt, "schg$o");
}

void Proxy::finish(){
#ifdef SCH_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": finish." << endl;
	fflush(stdout);
#endif
	if(queue != NULL)
		delete queue;
}

Proxy::~Proxy() {
}
