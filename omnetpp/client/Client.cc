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
#include "client/Client.h"

Define_Module(Client);

void Client::initialize()
{
	myId = getId() - C_ID_BASE;
	pktId = CLIENT_INDEX * (getId() - C_ID_BASE) + 1; // ID is valid from 1.
	traceEnd = false;

	char tfname[16] = {'t','r','a','c','e','s','/',
			't','r','a','c','e','0','0','0','\0'};
	tfname[12] = myId/100 + '0';
	tfname[13] = myId%100/10 + '0';
	tfname[14] = myId%10 + '0';
	if( (tfp = fopen(tfname, "r")) == NULL){
		fprintf(stderr, "Trace file open failure: %s\n", tfname);
		deleteModule();
	}

	char rfname[18] = {'r','e','s','u','l','t','s','/',
			'r','e','s','u','l','t','0','0','0','\0'};
	rfname[14] = myId/100 + '0';
	rfname[15] = myId%100/10 + '0';
	rfname[16] = myId%10 + '0';
	if( (rfp = fopen(rfname, "w+")) == NULL){
		fprintf(stderr, "Result file open failure: %s\n", rfname);
		deleteModule();
	}
	requestSync = new gPacket();
	requestSync->setKind(REQ_SYN);

	request = NULL;
	reqId = 0;

	readNextReq(); // Start Simulation.
}

void Client::handleMessage(cMessage *cmsg)
{
	switch(cmsg->getKind()){
	case REQ_SYN: // Time for creating a new request.
		sendLayoutQuery();
		break;
	case SELF_EVENT: // If not driven by the completion of previous job, self-driven.
		sendJobPacket((gPacket *)cmsg);
		break;
	case LAYOUT_RESP: // layout information
		handleLayoutResponse((qPacket *)cmsg);
		break;
	case JOB_RESP: // Job finished. Result from data server
		handleFinishedPacket((gPacket *)cmsg);
		break;
	default:
		fprintf(stderr, "ERROR Client #%d: Unknown message type %d", myId, cmsg->getKind());
	}
}

// Read next request from the file.
int Client::readNextReq(){
	if(traceEnd)
		return -1;
	if(requestSync->isScheduled()){
//		printf("Scheduled - at %lf\n",requestSync->getSendingTime());
//		fflush(stdout);
		return 0; // Already done this.
	}

	if(SIMTIME_DBL(simTime()) > MAX_TIME){
		fprintf(stdout, "You reached the simulation max time limit: %d.\n", MAX_TIME);
		fclose(tfp);
		traceEnd = true;
		return -1;
	}

	double time;
	long long offset;
	int size;
	int read;
	int appid;
	int sync;
	char line[201];

	// A request may be driven by the finish of the previous request (when the sync mark is set), or be driven by a time stamp.
	// If the sync mark is set, it will immediately process the current request when the previous one is finished.
	// Otherwise, the current request will be processed according to the "time" argument.
	// Note that currently we do not support processing multiple requests concurrently on a single client,
	// so if the request is driven by a time stamp, the request will be processed at the latter one of the finish time
	// of the previous request and the time stamp of this request in the trace file.

	while(1){
		if(fgets(line, 200, tfp) == NULL){
			fprintf(stdout, "CLIENT #%d: Reach the end of the trace file.\n", myId);
			fclose(tfp);
			traceEnd = true;
			return -1;
		}
		if(line[0] != '\n' && line[0] != '\0')
			break;
	}

	sscanf(line, "%lf %lld %d %d %d %d",
			&time, &offset, &size, &read, &appid, &sync);

	if(size > REQ_MAXSIZE){
		fprintf(stderr, "ERROR Client #%d: Size %d is bigger than REQ_MAXSIZE %d, set it to be REQ_MAXSIZE.\n",
				myId, size, REQ_MAXSIZE);
		size = REQ_MAXSIZE;
	}

	if(sync == 1 || (time < SIMTIME_DBL(simTime()) + C_REQ_PROC_TIME))
		time = SIMTIME_DBL(simTime()) + C_REQ_PROC_TIME;

	request = new Request(reqId++, time, offset, size, read, appid, sync);
	scheduleAt(time, requestSync);

	return 1;
}

// Return -1 if all the requests are done.
// Return 0 if can't schedule more packets at this moment.
// Return 1 if a new packet is scheduled.
// Return 2 if the request needs to query the layout.
int Client::scheduleNextPackets(){
	if(request == NULL) { // At the start, or just finished a request.
		if(readNextReq() == -1)
			return -1; // All requests are done.
		return 2; // Wait for requestSync to query the layout.
	}
	gPacket * gpkt = request->nextgPacket();
	if(gpkt == NULL)
		return 0; // Can't schedule more at this moment.

	gpkt->setId(pktId);
	gpkt->setRisetime(SIMTIME_DBL(simTime()) + C_PKT_PROC_TIME);
	gpkt->setKind(SELF_EVENT);

	pktId ++;
	scheduleAt((simtime_t)(gpkt->getRisetime()), gpkt);
	return 1;
}

int Client::sendLayoutQuery(){
	qPacket * qpkt = new qPacket("qpacket");
	qpkt->setId(pktId);// Important, other wise the response packet won't know where to be sent.
	qpkt->setApp(request->getApp());
	qpkt->setKind(LAYOUT_REQ);
	qpkt->setByteLength(100); // schedule query: assume Length 100.
	sendSafe(qpkt);
	return 1;
}

void Client::handleLayoutResponse(qPacket * qpkt){
	request->setLayout(qpkt);
	delete qpkt;
	scheduleNextPackets();
}


void Client::sendJobPacket(gPacket * gpkt){
	if(gpkt->getDecision() == UNSCHEDULED){
		fprintf(stderr, "ERROR: the packet is UNSCHEDULED.");
		return;
	}
	gpkt->setKind(JOB_REQ);
	if(gpkt->getRead() == 0) // Write operation, the packet size is (assumed) 100 + data size
		gpkt->setByteLength(100 + gpkt->getSize());
	gpkt->setSubmittime(SIMTIME_DBL(simTime()));
	sendSafe(gpkt);
	scheduleNextPackets();
}

void Client::handleFinishedPacket(gPacket * gpkt){
	gpkt->setReturntime(SIMTIME_DBL(simTime()));
	pktStatistic(gpkt);
	int ret = request->finishedgPacket(gpkt);
	delete gpkt;
	if(ret == 2){ // All done for this request.
		reqStatistic(request);
		delete request;
		request = NULL; // Mark that the current request is done.
		readNextReq(); // set up future event: read next request.
	}else if(ret == 1){ // You have done the current window, schedule next packets.
		scheduleNextPackets(); // set up future event
	}
}

void Client::sendSafe(cMessage * cmsg){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(cmsg, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(cmsg, "g$o");
}

// General information
void Client::reqStatistic(Request * request){
	fprintf(rfp, "Request #%d: %d %lld %ld {%lf %lf}\n",
			request->getId(),
			request->getApp(),
			request->getOffset(),
			request->getSize(),
			request->getStarttime(),
			request->getFinishtime());
}

// General information
void Client::pktStatistic(gPacket * gpkt){
	fprintf(rfp, "   Packet #%ld: %lld %d %d %d %d\n"
			"\t\t%lf %lf %lf %lf %lf\n",
			gpkt->getId(),
			(long long)(gpkt->getLowoffset() + gpkt->getHighoffset() * LOWOFFSET_RANGE),
			gpkt->getSize(),
			gpkt->getRead(),
			gpkt->getApp(), // End of basic info
			gpkt->getDecision(),

			gpkt->getRisetime(), // Time info
			gpkt->getArrivaltime(),
			gpkt->getDispatchtime(),
			gpkt->getFinishtime(),
			gpkt->getReturntime());
}

/*
void Client::statistic(gPacket * gpkt){
	double proc = gpkt->getFinishtime() - gpkt->getDispatchtime();
	stat_time += proc;
	fprintf(rfp, "%ld\t%lf\t%lf\n", gpkt->getId(), proc, stat_time);
}
*/
//Throughput and Delay information
/*
void Client::statistic(gPacket * gpkt){
	fprintf(rfp, "%lf %lf %lf %lf %lf\n",
		gpkt->getRisetime(),
		gpkt->getArrivaltime() - gpkt->getSubmittime(), // Arrivaltime : time is in lower case. (not ArrivalTime!)
		gpkt->getDispatchtime() - gpkt->getArrivaltime(),
		gpkt->getFinishtime() - gpkt->getDispatchtime(),
		gpkt->getReturntime() - gpkt->getRisetime());
}
*/

void Client::finish(){ // close result files
	fclose(rfp); // close files
	cancelAndDelete(requestSync);
}
