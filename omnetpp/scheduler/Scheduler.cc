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

#include "Scheduler.h"

Define_Module(Scheduler);

int Scheduler::schedulerID = 0;

Scheduler::Scheduler() {
}

void Scheduler::initialize(){
	myID = schedulerID;
	schedulerID ++;

	algorithm = par("algorithm").longValue();
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	numClients = par("numClients").longValue();

	switch(algorithm){
	case FIFO_ALG:
		queue = new FIFO(myID, degree);
		break;
	case SFQ_ALG:
		queue = new SFQ(myID, degree, numClients);
		break;
	case DSFQF_ALG:
		queue = new DSFQF(myID, degree, numClients);
		break;
	case DSFQA_ALG:
		queue = new DSFQA(myID, degree, numClients);
		break;
	case DSFQD_ALG:
		queue = new DSFQD(myID, degree, numClients);
		break;
	case SSFQ_ALG:
		queue = new SSFQ(myID, degree, numClients);
		break;
	case SDSFQA_ALG:
		queue = new SDSFQA(myID, degree, numClients);
		break;
	default:
		fprintf(stderr, "ERROR DSscheduler: Undefined algorithm.\n");
		deleteModule();
	}
}

void Scheduler::handleMessage(cMessage * cmsg) {
	switch(cmsg->getKind()){
	case JOB_REQ: // from the client
		handleNewJob((gPacket *)cmsg);
		break;
	case JOB_FIN: // from the data server
		handleFinishedJob((gPacket *)cmsg);
		break;
	case JOB_DISP: // to simulate the delay of scheduler processing.
		handleJobDispatch((gPacket *)cmsg);
		break;
	case JOB_RESP:
		handleJobResp((gPacket *)cmsg);
		break;
	case PROP_SCH: // inter-scheduler message
		handleInterSchedulerPacket((sPacket *)cmsg);
		break;
	}
}

void Scheduler::handleNewJob(gPacket * gpkt) { // from the client
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));
	gpkt->setKind(JOB_DISP); // let the router know: to the data server

#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d new job: %ld.", myID, gpkt->getID());
		fflush(stdout);
	}
#endif

	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		handleJobDispatch(gpkt);
	}
}

void Scheduler::handleJobDispatch(gPacket * gpkt){
#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d handleJobDispatch: %ld.\n", myID, gpkt->getID());
		fflush(stdout);
	}
#endif
	queue->pushWaitQ(gpkt);
	if(algorithm == DSFQA_ALG){
		sPacket * spkt = queue->propagateSPacket();
		if(spkt != NULL){
			propagateSPacket(spkt);
		}
	}
	dispatchJobs();
}

void Scheduler::handleFinishedJob(gPacket * gpkt){ // from the data server
	gpkt->setKind(JOB_RESP); // let the router know: to the client

#ifdef SCH_DEBUG
//	if(myID == 2){
//		printf("Scheduler #%d finish:", myID);
//		fflush(stdout);
//	}
#endif

	if(finjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	} else {
		handleJobResp(gpkt);
	}
}

void Scheduler::dispatchJobs(){
	gPacket * jobtodispatch = NULL;

#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d try to dispatch:\n", myID);
		fflush(stdout);
	}
#endif
	while(1){
		jobtodispatch = (gPacket *)queue->dispatchNext();

		if(jobtodispatch != NULL){
			if(algorithm == DSFQD_ALG){
				sPacket * spkt = queue->propagateSPacket();
				if(spkt != NULL){
					propagateSPacket(spkt);
				}
			}
			jobtodispatch->setScheduletime(SIMTIME_DBL(simTime()));
			sendSafe(jobtodispatch);
		}
		else
			break;
	}
}

void Scheduler::handleJobResp(gPacket * gpkt){
#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d handleJobResp: %ld.\n", myID, gpkt->getID());
		fflush(stdout);
	}
#endif
	gPacket * finpkt = (gPacket *)(queue->popOsQ(gpkt->getID()));
	// Note that due to split and assembling, the popped finished packet may not be equal to the response packet.

	if(algorithm == DSFQF_ALG){
		sPacket * spkt = queue->propagateSPacket();
		if(spkt != NULL){
			propagateSPacket(spkt);
		}
	}
	if(finpkt != NULL){
		finpkt->setKind(JOB_RESP); // Let the router know: to the client.
		sendSafe(finpkt); // to the client
	}

	dispatchJobs();
}

void Scheduler::handleInterSchedulerPacket(sPacket * spkt){
#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d: Receive interscheduler packet. %d %d\n", myID, spkt->getApp(), spkt->getLength());
		fflush(stdout);
	}
#endif
	if(algorithm != DSFQF_ALG && algorithm != DSFQA_ALG && algorithm != DSFQD_ALG){
		fprintf(stderr, "[ERROR] Scheduler: distributed scheduling algorithm not selected, calling handleInterSchedulerPacket is prohibited.\n");
		fflush(stderr);
		return;
	}
	queue->receiveSPacket(spkt);
	delete spkt;
}

void Scheduler::sendSafe(gPacket * gpkt){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(gpkt, "g$o");
}

void Scheduler::propagateSPacket(sPacket * spkt){
#ifdef SCH_DEBUG
	if(myID == 2){
		printf("Scheduler #%d: send interscheduler packet. %d %d\n", myID, spkt->getApp(), spkt->getLength());
		fflush(stdout);
	}
#endif
	send(spkt, "schg$o");
}

Scheduler::~Scheduler() {
}
