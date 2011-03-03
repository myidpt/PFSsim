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

#include "DSscheduler.h"

Define_Module(DSscheduler);

DSscheduler::DSscheduler() {
}

void DSscheduler::initialize(){
	char type[5] = {'F','I','F','O','\0'};
	if(!strcmp(type, "FIFO"))
		queue = new FIFO(SCH_DEGREE);
	else if(!strcmp(type,"SFQ"))
		queue = new SFQ(SCH_DEGREE);
}

void DSscheduler::handleMessage(cMessage * cmsg) {
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

void DSscheduler::handleNewJob(gPacket * gpkt) { // from the client
	gpkt->setKind(JOB_DISP); // let the router know: to the data server
	queue->pushWaitQ(gpkt);
	if(SCH_NJ_DELAY != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + SCH_NJ_DELAY), gpkt);
	} else {
		dispatchJobs();
	}
}

void DSscheduler::handleFinishedJob(gPacket * gpkt){ // from the data server
	gpkt->setKind(JOB_RESP); // let the router know: to the client
	queue->popOsQ(gpkt->getId());
	sendSafe(gpkt); // to the client
	if(SCH_FJ_DELAY != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + SCH_FJ_DELAY), gpkt);
	} else {
		dispatchJobs();
	}
}

void DSscheduler::dispatchJobs(){
	gPacket * jobtodispatch = NULL;
	while(1){
		jobtodispatch = queue->dispatchNext();
		if(jobtodispatch != NULL)
			sendSafe(jobtodispatch);
		else
			break;
	}
}

void DSscheduler::sendSafe(gPacket * gpkt){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(gpkt, "g$o");
}

DSscheduler::~DSscheduler() {
}
