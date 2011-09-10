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

Scheduler::Scheduler() {
}

void Scheduler::initialize(){
	algorithm = par("algorithm").longValue();
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	numClients = par("numClients").longValue();

	switch(algorithm){
	case FIFO_ALG:
		queue = new FIFO(degree);
		break;
	case SFQ_ALG:
		queue = new SFQ(degree, numClients);
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
	case JOB_RESP:
		dispatchJobs();
	}
}

void Scheduler::handleNewJob(gPacket * gpkt) { // from the client
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));
	gpkt->setKind(JOB_DISP); // let the router know: to the data server
	queue->pushWaitQ(gpkt);
	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		dispatchJobs();
	}
}

void Scheduler::handleFinishedJob(gPacket * gpkt){ // from the data server
	gpkt->setKind(JOB_RESP); // let the router know: to the client
	queue->popOsQ(gpkt->getID());
	sendSafe(gpkt); // to the client
	if(finjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	} else {
		dispatchJobs();
	}
}

void Scheduler::dispatchJobs(){
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

void Scheduler::sendSafe(gPacket * gpkt){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(gpkt, "g$o");
}

Scheduler::~Scheduler() {
}
