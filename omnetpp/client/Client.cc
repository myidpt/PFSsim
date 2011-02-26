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
	curId = CLIENT_INDEX * (getId() - C_ID_BASE) + 1; // ID is valid from 1.
	traceEnd = false;

	char fname[16] = {'t','r','a','c','e','s','/',
			't','r','a','c','e','0','0','0','\0'};
	fname[12] = myId/100 + '0';
	fname[13] = myId%100/10 + '0';
	fname[14] = myId%10 + '0';
	if( (fp = fopen(fname, "r")) == NULL){
		fprintf(stderr, "Trace file open failure: %s\n", fname);
		deleteModule();
	}

	char sfname[18] = {'r','e','s','u','l','t','s','/',
			'r','e','s','u','l','t','0','0','0','\0'};
	sfname[14] = myId/100 + '0';
	sfname[15] = myId%100/10 + '0';
	sfname[16] = myId%10 + '0';
	if( (sfp = fopen(sfname, "w+")) == NULL){
		fprintf(stderr, "Result file open failure: %s\n", fname);
		deleteModule();
	}
	requestSync = new cMessage("reqArrivalTime");

	process_time = C_PROC_TIME;
	request = NULL;

	readNextReq(); // Start Simulation.
}

void Client::handleMessage(cMessage *cmsg)
{
	if(cmsg == requestSync){ // Time for a new request.
		sendLayoutQuery(request);
		return;
	}
	switch(cmsg->getKind()){
	case SELF_EVENT: // If not driven by the completion of previous job, self-driven.
		sendJobPacket((gPacket *)cmsg);
		scheduleNextPackets();
		break;
	case LAYOUT_RESP: // layout information
		handleLayoutResponse((qPacket *)cmsg);
		scheduleNextPackets();
		break;
	case JOB_RESP: // Job finished. Result from data server
		handleFinishedPacket((gPacket *)cmsg);
		break;
	}
}

// Read next request from the file.
int Client::readNextReq(){
	if(traceEnd)
		return -1;
	if(requestSync->isScheduled()){
		return 0; // Already done this.
	}

	char line[201];
	if(SIMTIME_DBL(simTime()) > MAX_TIME){
		fprintf(stdout, "You reached the max time.\n");
		fclose(fp);
		traceEnd = true;
		return -1;
	}

	double time;
	long long offset;
	int size;
	int read;
	int appid;
	int sync;
	// If sync, you can only issue the current request when you have finished the previous one.
	// Otherwise, you issue the current request according to the "time" argument.
	while(1){
		if(fgets(line, 200, fp) == NULL){
			fprintf(stdout, "You reached the end of the trace file.\n");
			fclose(fp);
			traceEnd = true;
			return -1;
		}
		if(line[0] != '\n' && line[0] != '\0')
			break;
	}

	sscanf(line, "%lf %lld %d %d %d %d",
			&time, &offset, &size, &read, &appid, &sync);

	if(size > REQ_MAXSIZE){
		fprintf(stderr, "%lf %lld %d %d\n", time, offset, size, read);
		fprintf(stderr, "Size %d is bigger than REQ_MAXSIZE %d.\n", size, REQ_MAXSIZE);
		size = REQ_MAXSIZE;
	}

	if(sync == 1)
		time = SIMTIME_DBL(simTime()) + process_time;
	request = new Request(time, offset, size, read, appid, sync);
	scheduleAt(time, requestSync);

	return 1;
}

// Return -1 if all the requests are done.
// Return 0 if there are still packets to receive.
// Return 1 if the packet is scheduled.
// Return 2 if the request needs to query the layout.
int Client::scheduleNextPackets(){
	if(request == NULL) { // At the beginning, or just finished a request.
		if(readNextReq() == -1)
			return -1; // All requests are done.
		return 2;
	}
	gPacket * gpkt = request->nextgPacket();
	if(gpkt == NULL)
		return 0; // No more packets in the window.

	gpkt->setId(curId);
	gpkt->setRisetime(SIMTIME_DBL(simTime()) + process_time);
	gpkt->setKind(SELF_EVENT);
	curId ++;
	scheduleAt((simtime_t)(gpkt->getRisetime()), gpkt);
	return 1;
}

int Client::sendLayoutQuery(Request * request){
	qPacket * qpkt = new qPacket("qpacket");
	qpkt->setId(curId);
	qpkt->setApp(request->getApp());
	qpkt->setKind(LAYOUT_REQ);
	qpkt->setByteLength(100); // schedule query: assume Length 100.
	sendSafe(qpkt);
	return 1;
}

void Client::handleLayoutResponse(qPacket * qpkt){
	request->setLayout(qpkt);
	delete qpkt;
}

int Client::sendJobPacket(gPacket * gpkt){
	printf("%d: sendJobPacket-%ld\n", myId, gpkt->getId());
	fflush(stdout);
	gpkt->setKind(JOB_REQ);
	gpkt->setByteLength(100 + gpkt->getSize()); // Assume job length 100 + size. (write)
	if(gpkt->getDecision() == UNSCHEDULED)
		return -1;
	gpkt->setSubmittime(SIMTIME_DBL(simTime()));
	sendSafe(gpkt);
	return 1;
}

void Client::handleFinishedPacket(gPacket * gpkt){
	gpkt->setReturntime(SIMTIME_DBL(simTime()));
	pktStatistic(gpkt);
	int ret = request->finishedgPacket(gpkt);
	// gpkt is deleted here.
	if(ret == -1){
		reqStatistic(request);
		delete request;
		request = NULL;
		readNextReq(); // set up future event
	}else if(ret == 1){ // You have done the current window, call nextPacket.
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
	fprintf(sfp, "%lf %lf %lld %ld %d\n",
		request->getStarttime(),
		request->getFinishtime(),
		request->getOffset(),
		request->getSize(),
		request->getApp());
}

// General information
void Client::pktStatistic(gPacket * gpkt){
	fprintf(sfp, "\t%ld %lld %d %d %d %d\n"
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
	fprintf(sfp, "%ld\t%lf\t%lf\n", gpkt->getId(), proc, stat_time);
}
*/
//Throughput and Delay information
/*
void Client::statistic(gPacket * gpkt){
	fprintf(sfp, "%lf %lf %lf %lf %lf\n",
		gpkt->getRisetime(),
		gpkt->getArrivaltime() - gpkt->getSubmittime(), // Arrivaltime : time is in lower case. (not ArrivalTime!)
		gpkt->getDispatchtime() - gpkt->getArrivaltime(),
		gpkt->getFinishtime() - gpkt->getDispatchtime(),
		gpkt->getReturntime() - gpkt->getRisetime());
}
*/

void Client::finish(){ // close result files
	fclose(sfp); // close files
	cancelAndDelete(requestSync);
}
