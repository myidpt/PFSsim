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

#include "dserver/dsd/DSdaemon.h"

Define_Module(DSdaemon);
DSdaemon::DSdaemon() {
}

void DSdaemon::initialize(){
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	queue = new FIFO(degree);
}

void DSdaemon::handleMessage(cMessage * cmsg) {
	/*
	 * 			        JOB_DISP >				 LFILE_REQ >
	 *  router/switch <------------> DSdaemon <--------------> LocalFS
	 *                  < JOB_FIN  			 	< LFILE_RESP
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
	case JOB_FIN:
		sendToEth(gpkt);
		break;
	}
}

void DSdaemon::handleNewJob(gPacket * gpkt){
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));
	gpkt->setKind(LFILE_REQ); // turn to DATA_REQ
	if(newjob_proc_time != 0){ // Consider the delay on the data server delay.
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
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
	queue->popOsQ(gpkt->getID());
	gpkt->setName("JOB_FIN");
	gpkt->setKind(JOB_FIN);
	if(finjob_proc_time != 0){ // Consider the delay on the data server delay.
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	}
	else{
		sendToEth(gpkt);
	}
}

void DSdaemon::dispatchJobs(){
	gPacket * jobtodispatch = NULL;
	while(1){
		jobtodispatch = (gPacket *)queue->dispatchNext();
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
