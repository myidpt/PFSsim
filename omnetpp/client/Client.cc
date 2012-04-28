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

// Register self events:
// SELF_JOB_DISP		JOB_DISP
// SELF_JOB_DISP_LAST	JOB_DISP_LAST
// SELF_JOB_REQ		JOB_REQ

#include "client/Client.h"

Define_Module(Client);

int Client::idInit = 0;

void Client::initialize(){

    trc_proc_time = par("trc_proc_time").doubleValue();
    pkt_proc_time = par("pkt_proc_time").doubleValue();
    const char * trc_path_prefix = par("trc_path_prefix").stringValue();
    const char * rslt_path_prefix = par("rslt_path_prefix").stringValue();
    pkt_size_limit = par("pkt_size_limit").doubleValue();
    small_io_size_threshold = par("small_io_size_threshold").doubleValue();
    trcs_per_c = par("trcs_per_client").longValue();
    if(trcs_per_c > MAX_APP_PER_C){
		PrintError::print("Client", myID, "Applications per client exceeds the maximum.", trcs_per_c);
		deleteModule();
    }
//    int deg = par("degree").doubleValue();

	myID = idInit ++;
	pktId = CID_OFFSET * myID + PID_OFFSET * 1; // ID is valid from PID_OFFSET * 1.

	if(strlen(trc_path_prefix) > 196){
		PrintError::print("Client", myID, "Trace file path is too long; it should be less than 196.");
		deleteModule();
	}
	if(strlen(rslt_path_prefix) > 196){
		PrintError::print("Client", myID, "Result file path is too long; it should be less than 196.");
		deleteModule();
	}

	for(int i = 0; i < MAX_FILE; i ++)
		layoutlist[i] = NULL;

	char trcfname[200];
	strcpy(trcfname, trc_path_prefix);
	int len = strlen(trcfname);
	// Note: currently we only support less than 1000 clients.
	trcfname[len] = myID/100 + '0';
	trcfname[len+1] = myID%100/10 + '0';
	trcfname[len+2] = myID%10 + '0';
	trcfname[len+5] = '\0';

	char rsltfname[200];
	strcpy(rsltfname, rslt_path_prefix);
	int len2 = strlen(rsltfname);
	rsltfname[len2] = myID/100 + '0';
	rsltfname[len2+1] = myID%100/10 + '0';
	rsltfname[len2+2] = myID%10 + '0';
	rsltfname[len2+5] = '\0';

	// Note: currently we only support less than 100 applications per client.
	for(int i = 0; i < trcs_per_c; i ++){
		trcfname[len+3] = i/10 + '0';
		trcfname[len+4] = i%10 + '0';
		if( (tfp[i] = fopen(trcfname, "r")) == NULL){
			PrintError::print("Client", string("Trace file open failure: ")+trcfname+".");
			deleteModule();
		}

		rsltfname[len2+3] = i/10 + '0';
		rsltfname[len2+4] = i%10 + '0';
		if( (rfp[i] = fopen(rsltfname, "w+")) == NULL){
			PrintError::print("Client", string("Result file open failure: ")+rsltfname+".");
			deleteModule();
		}

		trace[i] = NULL; // init
		trcId[i] = 0;

		// Init traceSync
		traceSync[i] = new gPacket();
		traceSync[i]->setKind(TRC_SYN);
		traceSync[i]->setApp(i);

		traceEnd[i] = false;

		read_NextTrace(i); // Start Simulation.
	}
	cout << "Client #" << myID << " initialized." << endl;

//	reqQ = new FIFO(12345, deg);

}

void Client::handleMessage(cMessage *cmsg){
	switch(cmsg->getKind()){
	case TRC_SYN: // Time for creating a new trace.
		handle_NewTrace(((gPacket *)cmsg)->getApp());
		break;
	case SELF_JOB_DISP: // JOB_DISP
	case SELF_JOB_DISP_LAST: // JOB_DISP_LAST
	case SELF_JOB_REQ: // JOB_REQ
		// Can also be self-driven.
		send_JobPacket((gPacket *)cmsg);
		break;
	case LAYOUT_RESP: // layout information
		handle_LayoutResp((qPacket *)cmsg);
		break;
	case JOB_FIN: // This response can be ignored.
//		cout << simTime() << ": JOB_FIN ignored. size=" << ((gPacket *)cmsg)->getSize() << endl;
#ifdef TRACE_PKT_STAT
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": handle_FinishedPacket. ID=" << ((gPacket *)cmsg)->getID() << ", dserver=" << ((gPacket *)cmsg)->getDsID() << endl;
#endif
		((gPacket *)cmsg)->setReturntime(SIMTIME_DBL(simTime()));
		pktStatistic((gPacket *)cmsg);
#endif
		delete cmsg;
		break;
	case JOB_FIN_LAST: // Job finished.
	case JOB_RESP:
//		cout << simTime() << ": JOB_FIN." << endl;
		handle_FinishedPacket((gPacket *)cmsg);
		break;
	default:
		char sentence[50];
		sprintf(sentence, "Unknown message type %d.", cmsg->getKind());
		PrintError::print("Client", myID, sentence);
		break;
	}
}

void Client::handle_NewTrace(int appid){
	int fid = trace[appid]->getFileId();
	if(layoutlist[fid] == NULL) // File layout hasn't been created yet. Need to ask MS.
		send_LayoutReq(appid);
	else{
		trace[appid]->setLayout(layoutlist[fid]);
		schedule_NextPackets(appid);
	}
}

// Read next trace from the file.
int Client::read_NextTrace(int appid){
	if(traceEnd[appid]){
		return -1;
	}
	if(traceSync[appid]->isScheduled()){
		return 0; // Already done this.
	}

	if(SIMTIME_DBL(simTime()) > MAX_TIME){
		fprintf(stdout, "You reached the simulation max time limit: %d.\n", MAX_TIME);
		if(tfp[appid] != NULL)
			fclose(tfp[appid]);
		tfp[appid] = NULL; //Nullify it.
		traceEnd[appid] = true;
		return -1;
	}

	double time;
	long long offset;
	int size;
	int read;
	int fileid;
	int appidtmp;
	int sync;
	char line[201];

	// A trace may be driven by the finish of the previous trace (when the sync mark is set), or be driven by a time stamp.
	// If the sync mark is set, it will process the next trace when the previous one is finished.
	// Otherwise, the current trace will be processed according to the "time stamp" on the trace when it is read from the trace file.
	// Note that currently we do not support processing multiple traces in parallel on a single client,
	// so if the trace is driven by a time stamp, the trace will be processed at the latter one of the finish time
	// of the previous trace and the time stamp on this trace.

	while(1){
		if(fgets(line, 200, tfp[appid]) == NULL){
			fprintf(stdout, "Client #%d: Reach the end of the trace file #%d.\n", myID, appid);
			if(tfp[appid] != NULL)
				fclose(tfp[appid]);
			tfp[appid] = NULL;
			traceEnd[appid] = true;
			for(int i = 0; i < trcs_per_c; i ++){
				if(traceEnd[i] == false)
					return -1;
			}
			fprintf(stdout, "Client #%d: All traces are done.\n", myID);

			return -1;
		}
		if(line[0] != '\n' && line[0] != '\0')
			break;
	}

	sscanf(line, "%lf %d %lld %d %d %d %d",
			&time, &fileid, &offset, &size, &read, &appidtmp, &sync);

	if(size >= TRC_MAXSIZE){
		PrintError::print("Client", myID, "Size is bigger than TRC_MAXSIZE, set it to be TRC_MAXSIZE.", size);
		size = TRC_MAXSIZE;
	}

	if(sync == 0 && (time < SIMTIME_DBL(simTime()) + trc_proc_time)){
	    // You can't dispatch job on the provided time, because the earliest allowed dispatch time is later than that.
		time = SIMTIME_DBL(simTime()) + trc_proc_time;
	}else if(sync == 1){
	    // sync == 1, provided time is the inter-arrival time: the time span
		if(trc_proc_time > time)
			time = SIMTIME_DBL(simTime()) + trc_proc_time;
		else
			time = SIMTIME_DBL(simTime()) + time;
	}

#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": read_NextTrace. fid=" << fileid <<", offset=" << offset << ", size=" << size << endl;
#endif
	trace[appid] = new Trace(trcId[appid]++, time, fileid, offset, size, read, appid, sync);
	if(time > SIMTIME_DBL(simTime()))
	    scheduleAt(time, traceSync[appid]);
	else
	    handle_NewTrace(appid);
	return 1;
}

void Client::schedule_NextPackets(int appid){ // You should schedule all the packets in the current window at once.
	if(trace[appid] == NULL) { // At the start, or just finished a trace.
		if(read_NextTrace(appid) == -1)
			return; // All traces are done.
		return; // Wait for traceSync to query the layout.
	}

	gPacket * gpkt;
	while(1){
		gpkt = trace[appid]->nextgPacket(); // Get new requset from trace.
		if(gpkt == NULL)
			return; // Can't schedule more at this moment.

		gpkt->setID(pktId);
		gpkt->setRisetime(SIMTIME_DBL(simTime()) + pkt_proc_time);
		gpkt->setClientID(myID);

#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": scheduleNextPacket. ID=" << gpkt->getID() << ", app=" << gpkt->getApp();
	cout << ", lowoffset=" << gpkt->getLowoffset() << ", size=" << gpkt->getSize() << ", dserver=" << gpkt->getDsID() << endl;
#endif

		pktId += PID_OFFSET;
		if(pktId % CID_OFFSET >= CID_OFFSET - PID_OFFSET) // Due to the limited ID length, don't allow the trace ID to go too big.
			pktId = CID_OFFSET * myID + PID_OFFSET * 1; // Restart it;

//	reqQ->pushWaitQ(gpkt); // Push to reqQ.
//	if(reqQ->dispatchNext() == NULL){ // It is the gpkt object. Push it to the osQ.
//		return 0;
//		PrintError::print("Client - schedule_NextPackets", "req->dispatchNext Error: didn't get the object just inserted.");
//	}

		gpkt->setRisetime(SIMTIME_DBL(simTime())+pkt_proc_time);
		gpkt->setKind(SELF_JOB_REQ); // This lead to JOB_REQ to be sent.

		if(pkt_proc_time != 0)
			scheduleAt((simtime_t)(gpkt->getRisetime()), gpkt);
		else
			send_JobPacket(gpkt);
	}
}

int Client::send_LayoutReq(int appid){
	qPacket * qpkt = new qPacket("qpacket");
	qpkt->setID(pktId);// Important, other wise the response packet won't know where to be sent.
	qpkt->setFileId(trace[appid]->getFileId());
	qpkt->setKind(LAYOUT_REQ);
	qpkt->setByteLength(DATA_HEADER_LENGTH); // Set the length of packet
	qpkt->setClientID(myID);
	qpkt->setApp(appid);
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": send_LayoutReq. ID=" << qpkt->getID() << endl;
#endif
	sendSafe(qpkt);
	return 1;
}


void Client::handle_LayoutResp(qPacket * qpkt){
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": handle_LayoutResp. ID=" << qpkt->getID() << endl;
#endif
	int fid = qpkt->getFileId();
	if(layoutlist[fid] != NULL)
		PrintError::print("Client - handle_LayoutResp", "Query the layout for a layout-existing file.");
	layoutlist[fid] = new Layout(fid);
	layoutlist[fid]->setLayout(qpkt);
	trace[qpkt->getApp()]->setLayout(layoutlist[fid]);
	schedule_NextPackets(qpkt->getApp());
	delete qpkt;
}

void Client::handle_FinishedPacket(gPacket * gpkt){
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": handle_FinishedPacket. ID=" << gpkt->getID() << ", dserver=" << gpkt->getDsID() << endl;
#endif
	gpkt->setReturntime(SIMTIME_DBL(simTime()));
#ifdef TRACE_PKT_STAT
	pktStatistic(gpkt);
#endif

	int ret;
	int appid = gpkt->getApp();
	if(gpkt->getKind() == JOB_FIN_LAST){ // This is the last packet of a read/write.
		ret = trace[appid]->finishedgPacket(gpkt);
		if(ret == 2){ // All done for this trace.
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": trace " << appid << " DONE." << endl;
#endif
			trcStatistic(trace[appid]);
			delete trace[appid];
			trace[appid] = NULL; // Mark that the current trace is done.
			read_NextTrace(appid); // set up future event: read next trace.
		}else if(ret == 1 || ret == 0){ // You have done the current window, schedule next packets.
			schedule_NextPackets(gpkt->getApp()); // set up future event
		}
		delete gpkt;
		return;
	}

	if(gpkt->getKind() == JOB_FIN){
		delete gpkt;
		return;
	}

	if(gpkt->getKind() == JOB_RESP){ // This is the finish of the first round for a write.
		// This is in the middle of a write. Have got the JOB_RESP. Start to transfer the real data.
		gpkt->setName("JOB_DISP");
		long subid = gpkt->getID()+1;
		long loff = gpkt->getLowoffset();
		long ub = loff + gpkt->getSize();
		double time = SIMTIME_DBL(simTime());

		while(1){
			// Push the first sub-request into subreqQ.
			time += pkt_proc_time;
			if(loff + pkt_size_limit >= ub || gpkt->getRead()){
				// This is the last sub-request. We push the original request to the subreqQ.
				gpkt->setID(subid);
				gpkt->setKind(SELF_JOB_DISP_LAST);
				gpkt->setLowoffset(loff);
				gpkt->setSize(ub - loff);
				gpkt->setRisetime(time); // Time is important for scheduling.
				if(pkt_proc_time != 0)
					scheduleAt((simtime_t)(gpkt->getRisetime()), gpkt);
				else
					send_JobPacket(gpkt);
				return;
			}else{
				gPacket * subreq = new gPacket(*gpkt);
				subreq->setID(subid);
				subreq->setKind(SELF_JOB_DISP);
				subreq->setLowoffset(loff);
				subreq->setSize(pkt_size_limit);
				subreq->setRisetime(time); // Time is important for scheduling.
				if(pkt_proc_time != 0)
					scheduleAt((simtime_t)(subreq->getRisetime()), subreq);
				else
					send_JobPacket(subreq);
				loff += pkt_size_limit;
				subid ++;
			}
		}
	}
}

void Client::send_JobPacket(gPacket * gpkt){
	if(gpkt->getDsID() == UNSCHEDULED){
		PrintError::print("Client", "The packet is UNSCHEDULED.");
		return;
	}

#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": send_JobPacket ID = " << gpkt->getID() << ", dserver = " << gpkt->getDsID() << endl;
#endif

	if(gpkt->getKind() == SELF_JOB_DISP_LAST)
	    gpkt->setKind(JOB_DISP_LAST);
	else if(gpkt->getKind() == SELF_JOB_DISP)
	    gpkt->setKind(JOB_DISP);
	else if(gpkt->getKind() == SELF_JOB_REQ)
		gpkt->setKind(JOB_REQ);

	if(gpkt->getKind() == JOB_REQ && (gpkt->getSize() >= small_io_size_threshold || gpkt->getRead())) // Read operation or write 1st round.
		gpkt->setByteLength(DATA_HEADER_LENGTH);
	else // Write operation (including small write), the packet size is DATA_HEADER_LENGTH + data size
		gpkt->setByteLength(DATA_HEADER_LENGTH + gpkt->getSize());

//	cout << gpkt->getID() << " ==> " << gpkt->getByteLength() << endl;
	sendSafe(gpkt);
}

void Client::sendSafe(cMessage * cmsg){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	gPacket * tmp = NULL;
	if(cch->isBusy()){
		tmp = dynamic_cast<gPacket *>(cmsg);
		if(tmp)
			tmp->setSubmittime(SIMTIME_DBL(cch->getTransmissionFinishTime()));
		sendDelayed(cmsg, cch->getTransmissionFinishTime() - simTime(), "g$o");
	}
	else{
		tmp = dynamic_cast<gPacket *>(cmsg);
		if(tmp)
			tmp->setSubmittime(SIMTIME_DBL(simTime()));
		send(cmsg, "g$o");
	}
}

// General information
void Client::trcStatistic(Trace * trc){
	fprintf(rfp[trc->getApp()], "%d %d %d %lld %ld SPAN=%lf TIME=%lf\n",
			trc->getID(),
			trc->getFileId(),
			trc->getRead(),
			trc->getOffset(),
			trc->getTotalSize(),

			1000*(trc->getFinishtime() - trc->getStarttime()),
			trc->getFinishtime());
}

// General information
void Client::pktStatistic(gPacket * gpkt){
//	long long lowoff = gpkt->getLowoffset();
//	long long highoff = gpkt->getHighoffset();
	fprintf(rfp[gpkt->getApp()], "Packet ID=%ld: APP=%d FID=%d SVR=%d"
			"\tS=%lf N=%lf (TIME=%lf)\n",
			gpkt->getID(),
			gpkt->getApp(),
//			(long long)(lowoff + highoff * LOWOFFSET_RANGE),
//			gpkt->getSize(),
//			gpkt->getRead(),
			gpkt->getFileId(), // End of basic info
//			gpkt->getDsID(),
			gpkt->getDsID(),

//			1000*(gpkt->getSubmittime() - gpkt->getRisetime()),
//			1000*(gpkt->getArrivaltime() - gpkt->getSubmittime()),
//			1000*(gpkt->getDispatchtime() - gpkt->getArrivaltime()),
			1000*(gpkt->getFinishtime() - gpkt->getDispatchtime()),
			1000*(gpkt->getReturntime() - gpkt->getFinishtime()),
//			1000*(gpkt->getFinishtime() - gpkt->getRisetime()),
			SIMTIME_DBL(simTime()));
}


/*
void Client::statistic(gPacket * gpkt){
	double proc = gpkt->getFinishtime() - gpkt->getDispatchtime();
	stat_time += proc;
	fprintf(rfp, "%ld\t%lf\t%lf\n", gpkt->getID(), proc, stat_time);
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
#ifdef C_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Client#" << myID << ": finish." << endl;
	fflush(stdout);
#endif
	cout << "Client finish." << endl;
	fflush(stdout);

	for(int i = 0; i < trcs_per_c; i ++){
		if(tfp[i] != NULL)
			fclose(tfp[i]); // close files
		if(rfp[i] != NULL)
			fclose(rfp[i]); // close files
		if(traceSync[i] != NULL)
			cancelAndDelete(traceSync[i]);
		if(trace[i] != NULL){
			delete trace[i];
			trace[i] = NULL;
		}
	}
	for(int i = 0; i < MAX_FILE; i ++){
		if(layoutlist[i] != NULL)
			delete layoutlist[i];
	}
}
