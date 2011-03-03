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
 * Currently the DSdaemon just adds up some processing delay to the incoming (from eth) packets.
 */

#include "DSdaemon.h"

Define_Module(DSdaemon);
DSdaemon::DSdaemon() {
}

void DSdaemon::initialize(){
	queue = new FIFO(DS_DAEMON_DEGREE);
}

void DSdaemon::handleMessage(cMessage * cmsg) {
	/*
	 * 			 JOB_DISP >				  LFILE_REQ >
	 *  router <------------> DSdaemon <--------------> LocalFS
	 *           < JOB_FIN  			 < LFILE_RESP
	 */
	gPacket * gpkt = (gPacket *)cmsg;
	switch(gpkt->getKind()){
	case JOB_DISP:
		handleNewJob(gpkt);
		break;
	case LFILE_REQ:
		handleDataReq(gpkt);
		break;
	case LFILE_RESP:
		handleDataResp(gpkt);
		break;
	}
}

void DSdaemon::handleNewJob(gPacket * gpkt){
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));
	gpkt->setKind(LFILE_REQ); // turn to DATA_REQ
	if(DS_DAEMON_DELAY != 0){ // Consider the delay on the data server delay.
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + DS_DAEMON_DELAY), gpkt);
	}
	else{
		handleDataReq(gpkt);
	}
}

void DSdaemon::handleDataReq(gPacket * gpkt){
	queue->pushWaitQ(gpkt);
	dispatchJobs();
}

void DSdaemon::handleDataResp(gPacket * gpkt){
	queue->popOsQ(gpkt->getId());
	gpkt->setKind(JOB_FIN);
	sendToEth(gpkt);
}

void DSdaemon::dispatchJobs(){
	gPacket * jobtodispatch = NULL;
	while(1){
		jobtodispatch = queue->dispatchNext();
		if(jobtodispatch != NULL)
			sendToLFS(jobtodispatch);
		else
			break;
	}
}

void DSdaemon::sendToLFS(gPacket * gpkt){
	send(gpkt, "lfs$o");
}

void DSdaemon::sendToEth(gPacket * gpkt){
	cChannel * cch = gate("eth$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "eth$o");
	else
		send(gpkt, "eth$o");
}

DSdaemon::~DSdaemon() {
}
