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

int Client::idInit = 0;

void Client::initialize(){

    trc_proc_time = par("trc_proc_time").doubleValue();
    pkt_proc_time = par("pkt_proc_time").doubleValue();
    const char * trc_path_prefix = par("trc_path_prefix").stringValue();
    const char * rslt_path_prefix = par("rslt_path_prefix").stringValue();

	myId = idInit ++;
	pktId = CID_OFFSET_IN_PID * myId + 1; // ID is valid from 1.
	traceEnd = false;

	if(strlen(trc_path_prefix) > 196){
		fprintf(stderr, "ERROR Client: Trace file path is too long; it should be less than 196.\n");
		deleteModule();
	}
	if(strlen(rslt_path_prefix) > 196){
		fprintf(stderr, "ERROR Client: Result file path is too long; it should be less than 196.\n");
		deleteModule();
	}
	char trcfname[200];
	strcpy(trcfname, trc_path_prefix);
	int len = strlen(trcfname);
	// Note: currently we only support less than 10000 client.
	trcfname[len] = myId/1000 + '0';
	trcfname[len+1] = myId%1000/100 + '0';
	trcfname[len+2] = myId%100/10 + '0';
	trcfname[len+3] = myId%10 + '0';
	trcfname[len+4] = '\0';
	if( (tfp = fopen(trcfname, "r")) == NULL){
		fprintf(stderr, "ERROR Client: Trace file open failure: %s\n", trcfname);
		deleteModule();
	}
	char rsltfname[200];
	strcpy(rsltfname, rslt_path_prefix);
	len = strlen(rsltfname);
	rsltfname[len] = myId/1000 + '0';
	rsltfname[len+1] = myId%1000/100 + '0';
	rsltfname[len+2] = myId%100/10 + '0';
	rsltfname[len+3] = myId%10 + '0';
	rsltfname[len+4] = '\0';
	if( (rfp = fopen(rsltfname, "w+")) == NULL){
		fprintf(stderr, "Result file open failure: %s\n", rsltfname);
		deleteModule();
	}

	traceSync = new gPacket();
	traceSync->setKind(TRC_SYN);

	trace = NULL;
	trcId = 0;

	readNextTrace(); // Start Simulation.
}

void Client::handleMessage(cMessage *cmsg){
	switch(cmsg->getKind()){
	case TRC_SYN: // Time for creating a new trace.
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

// Read next trace from the file.
int Client::readNextTrace(){
	if(traceEnd){
		return -1;
	}
	if(traceSync->isScheduled()){
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

	// A trace may be driven by the finish of the previous trace (when the sync mark is set), or be driven by a time stamp.
	// If the sync mark is set, it will process the next trace when the previous one is finished.
	// Otherwise, the current trace will be processed according to the "time stamp" on the trace when it is read from the trace file.
	// Note that currently we do not support processing multiple traces in parallel on a single client,
	// so if the trace is driven by a time stamp, the trace will be processed at the latter one of the finish time
	// of the previous trace and the time stamp on this trace.

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

	if(size > TRC_MAXSIZE){
		fprintf(stderr, "ERROR Client #%d: Size %d is bigger than TRC_MAXSIZE %d, set it to be TRC_MAXSIZE.\n",
				myId, size, TRC_MAXSIZE);
		size = TRC_MAXSIZE;
	}

	if(sync == 1 || (time < SIMTIME_DBL(simTime()) + trc_proc_time))
		time = SIMTIME_DBL(simTime()) + trc_proc_time;

	trace = new Trace(trcId++, time, offset, size, read, appid, sync);
	scheduleAt(time, traceSync);

	return 1;
}

// Return -1 if all the traces are done.
// Return 0 if can't schedule more packets at this moment.
// Return 1 if a new packet is scheduled.
// Return 2 if the trace needs to query the layout.
int Client::scheduleNextPackets(){
	if(trace == NULL) { // At the start, or just finished a trace.
		if(readNextTrace() == -1)
			return -1; // All traces are done.
		return 2; // Wait for traceSync to query the layout.
	}
	gPacket * gpkt = trace->nextgPacket();
	if(gpkt == NULL)
		return 0; // Can't schedule more at this moment.

	gpkt->setId(pktId);
	gpkt->setRisetime(SIMTIME_DBL(simTime()) + pkt_proc_time);
	gpkt->setKind(SELF_EVENT);

	pktId ++;
	scheduleAt((simtime_t)(gpkt->getRisetime()), gpkt);
	return 1;
}

int Client::sendLayoutQuery(){
	qPacket * qpkt = new qPacket("qpacket");
	qpkt->setId(pktId);// Important, other wise the response packet won't know where to be sent.
	qpkt->setApp(trace->getApp());
	qpkt->setKind(LAYOUT_REQ);
	qpkt->setByteLength(100); // schedule query: assume Length 100.
	sendSafe(qpkt);
	return 1;
}

void Client::handleLayoutResponse(qPacket * qpkt){
	trace->setLayout(qpkt);
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
	int ret = trace->finishedgPacket(gpkt);
	delete gpkt;
	if(ret == 2){ // All done for this trace.
		trcStatistic(trace);
		delete trace;
		trace = NULL; // Mark that the current trace is done.
		readNextTrace(); // set up future event: read next trace.
	}else if(ret == 1 || ret == 0){ // You have done the current window, schedule next packets.
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
void Client::trcStatistic(Trace * trc){
	fprintf(rfp, "Trace #%d: %d %lld %ld {%lf %lf}\n",
			trc->getId(),
			trc->getApp(),
			trc->getOffset(),
			trc->getSize(),
			trc->getStarttime(),
			trc->getFinishtime());
}

// General information
void Client::pktStatistic(gPacket * gpkt){
	fprintf(rfp, "   Packet #%ld: %lld %d %d %d %d\n"
			"\t\t%lf %lf %lf %lf %lf %lf %lf\n",
			gpkt->getId(),
			(long long)(gpkt->getLowoffset() + gpkt->getHighoffset() * LOWOFFSET_RANGE),
			gpkt->getSize(),
			gpkt->getRead(),
			gpkt->getApp(), // End of basic info
			gpkt->getDecision(),

			gpkt->getRisetime(), // Time info
			gpkt->getInterceptiontime(),
			gpkt->getScheduletime(),
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
	cancelAndDelete(traceSync);
}
