// Auther: Yonggang Liu. All rights reserved.

#include "Proxy.h"

Define_Module(Proxy);

int Proxy::proxyID = 0;
int Proxy::numProxies=0;

Proxy::Proxy() {
}

void Proxy::initialize(){
    //In order to rebuild Network with the correct ID.
    if(proxyID==0){//Parse only one time cause it's a static on the first call
        numProxies=par("numProxies").longValue();
    }
    myID = proxyID;
    proxyID ++;
    if(proxyID>=numProxies){
        proxyID=0; //The last call reset the proxyID
    }

	algorithm = par("algorithm").stringValue();
	const char * alg_param =par("alg_param").stringValue();
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	int numapps = par("numApps").longValue();
	alg_timer = NULL;

	cout << "Proxy #" << myID << " algorithm: ";
	queue = SchedulerFactory::createScheduler(algorithm, myID, degree, numapps, alg_param);

	if (queue == NULL) {
	    PrintError::print("Proxy", "Undefined algorithm.");
	    deleteModule();
	}

	// These algorithms need to set the timers.
    if (!strcmp(algorithm, SchedulerFactory::DSFQATB_ALG) || !strcmp(algorithm, SchedulerFactory::I2L_ALG)) {
        alg_timer = new gPacket("ALG_TIMER");
        alg_timer->setKind(ALG_TIMER);
        scheduleAt(queue->notify(), alg_timer);
    }
/*
    cMessage * info_timer = new cMessage("INFO", SELF_EVENT);
    scheduleAt(INFO_INTERVAL, info_timer);
*/
    osReqNum = 0;
    totalOSReqNum = 0;
}

/*
 * We map the packet kinds to the SELF event family for internal use.
 */

void Proxy::handleMessage(cMessage * cmsg) {
#ifdef MSG_PROXY
	cout << "Proxy[" << myID << "] " << MessageKind::getMessageKindString(cmsg->getKind()) <<
			" ID=" << ((bPacket *)cmsg)->getID() << endl;
#endif
	switch(cmsg->getKind()){
	case PFS_R_REQ: // To be enqueued.
	case PFS_W_REQ:
		handleJobReq((gPacket *)cmsg);
		break;

	case SELF_PFS_R_REQ: // Enqueue here.
	case SELF_PFS_W_REQ:
		handleJobReq2((gPacket *)cmsg);
		break;

	case PFS_W_FIN: // To be dequeued.
	case PFS_R_DATA_LAST:
		handleReadLastWriteFin((gPacket *)cmsg);
		break;

	case SELF_PFS_W_FIN: // Dequeue here.
	case SELF_PFS_R_DATA_LAST:
		handleReadLastWriteFin2((gPacket *)cmsg);
		break;

	case PFS_W_RESP:
	case PFS_W_DATA: // No need to put in queue.
	case PFS_W_DATA_LAST: // No need to put in queue or pop queue. The queue will be poped when PFS_W_FIN comes.
	case PFS_R_DATA: // No need to go through the queue.
		handleMinorReadWrite((gPacket *)cmsg);
		break;

	case SELF_PFS_W_RESP:
	case SELF_PFS_W_DATA:
	case SELF_PFS_W_DATA_LAST:
	case SELF_PFS_R_DATA:
		sendSafe((gPacket *)cmsg);
		break;

	case PROP_SCH: // inter-scheduler message, currently unused.
		handleInterSchedulerPacket((sPacket *)cmsg);
		break;

	case ALG_TIMER: // Timer for the propagation interval.
		handleAlgorithmTimer();
		break;
/*
	case SELF_EVENT: // Self information print.
	    scheduleAt(SIMTIME_DBL(simTime()) + INFO_INTERVAL, cmsg);
        totalOSReqNum += osReqNum;
	    if (((int)(SIMTIME_DBL(simTime())*10 + 1) % 10) == 0) {
	        cout << "Time: " << SIMTIME_DBL(simTime()) << " " << (totalOSReqNum / 10) << endl;
	        totalOSReqNum = 0;
	    }
	    break;
*/
	default:
		char sentence[50];
		sprintf(sentence, "Unknown message type %d.", cmsg->getKind());
		PrintError::print("Proxy", myID, sentence);
		break;
	}
}

void Proxy::handleJobReq(gPacket * gpkt){
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobReq ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));
	if (gpkt->getKind() == PFS_R_REQ) {
		gpkt->setKind(SELF_PFS_R_REQ);
	} else if (gpkt->getKind() == PFS_W_REQ) {
		gpkt->setKind(SELF_PFS_W_REQ);
	}

	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		handleJobReq2(gpkt);
	}
}

void Proxy::handleJobReq2(gPacket * gpkt) {
#ifdef PROXY_DEBUG
    cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobReq2 ID=" << gpkt->getID() << endl;
    fflush(stdout);
#endif

    queue->pushWaitQ(gpkt); // Push in the request.
    if(!strcmp(algorithm, SchedulerFactory::DSFQA_ALG)
            || !strcmp(algorithm, SchedulerFactory::DSFQALB_ALG)) {
        // DSFQA packet propagation is triggered.
		propagateSPackets();
    }
	scheduleJobs();
}

void Proxy::handleReadLastWriteFin(gPacket * gpkt) { // from the data server
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobRespFinComp ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif

	if (gpkt->getKind() == PFS_W_FIN) {
		gpkt->setKind(SELF_PFS_W_FIN);
	} else { // PFS_R_DATA_LAST
		gpkt->setKind(SELF_PFS_R_DATA_LAST);
	}

	if(finjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + finjob_proc_time), gpkt);
	} else {
		handleReadLastWriteFin2(gpkt);
	}
}

void Proxy::handleReadLastWriteFin2(gPacket * gpkt){
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobRespFinComp2 ID=" << gpkt->getID() << endl;
	fflush(stdout);
#endif

	int recordPacketID = gpkt->getID() / RID_OFFSET * RID_OFFSET;
	// The recorded one is the request, which has the lowest ID among the serial of packets from one request.
	gPacket * queuedpacket;
    queuedpacket = (gPacket *)queue->popOsQ(recordPacketID);
	delete queuedpacket; // Delete the record.
	osReqNum --;

	if(!strcmp(algorithm, SchedulerFactory::DSFQF_ALG)) { // DSFQA packet propagation is triggered.
		propagateSPackets();
	}

	sendSafe(gpkt); // to the client
	scheduleJobs();
}

// PFS_W_RESP, PFS_W_DATA or PFS_W_DATA_LAST, PFS_R_DATA:
// The packet does not go through the queue.
void Proxy::handleMinorReadWrite(gPacket * gpkt) { // from the client
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleJobDisp ID=" << gpkt->getID() << endl;
#endif
	gpkt->setInterceptiontime(SIMTIME_DBL(simTime()));

	if(gpkt->getKind() == PFS_W_RESP) {
		gpkt->setKind(SELF_PFS_W_RESP);
	} else if(gpkt->getKind() == PFS_W_DATA) {
		gpkt->setKind(SELF_PFS_W_DATA);
	} else if(gpkt->getKind() == PFS_W_DATA_LAST) {
		gpkt->setKind(SELF_PFS_W_DATA_LAST);
	} else {
		gpkt->setKind(SELF_PFS_R_DATA);
	}

	if(newjob_proc_time != 0){
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + newjob_proc_time), gpkt);
	} else {
		sendSafe(gpkt);
	}
}

void Proxy::handleAlgorithmTimer(){
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": handleAlgorithmTimer" << endl;
#endif
	if (!strcmp(algorithm, SchedulerFactory::DSFQATB_ALG)) {
		propagateSPackets();
        scheduleAt(queue->notify(), alg_timer);
	}
	else if (!strcmp(algorithm, SchedulerFactory::I2L_ALG)) {
        scheduleAt(queue->notify(), alg_timer);
        scheduleJobs(); // Due to the update of the information, new requests may be available to be dispatched.
	}
	else {
        PrintError::print("Proxy", "Current algorithm does not support .");
	}
}

void Proxy::scheduleJobs(){
#ifdef PROXY_DEBUG
    cout << "ScheduleJobs." << endl;
#endif
	gPacket * jobtodispatch = NULL;
	gPacket * packetcopy = NULL;
	while(1){
	    jobtodispatch = (gPacket *)queue->dispatchNext();

		if(jobtodispatch != NULL){
			if(!strcmp(algorithm, SchedulerFactory::DSFQD_ALG)) { // DSFQA packet propagation is triggered.
				propagateSPackets();
			}
			packetcopy = new gPacket(*jobtodispatch);
			jobtodispatch->setScheduletime(SIMTIME_DBL(simTime()));
			packetcopy->setScheduletime(SIMTIME_DBL(simTime()));
			sendSafe(packetcopy);
			osReqNum ++;
		}
		else
			break;
	}
#ifdef PROXY_DEBUG
    cout << "ScheduleJobs done." << endl;
#endif
}

void Proxy::sendSafe(gPacket * gpkt){
	switch(gpkt->getKind()){
	case SELF_PFS_W_REQ:
		gpkt->setKind(PFS_W_REQ);
		break;
	case SELF_PFS_R_REQ:
		gpkt->setKind(PFS_R_REQ);
		break;
	case SELF_PFS_W_RESP:
		gpkt->setKind(PFS_W_RESP);
		break;

	case SELF_PFS_W_DATA:
		gpkt->setKind(PFS_W_DATA);
		break;
	case SELF_PFS_W_DATA_LAST:
		gpkt->setKind(PFS_W_DATA_LAST);
		break;
	case SELF_PFS_W_FIN:
		gpkt->setKind(PFS_W_FIN);
		break;

	case SELF_PFS_R_DATA:
		gpkt->setKind(PFS_R_DATA);
		break;
	case SELF_PFS_R_DATA_LAST:
		gpkt->setKind(PFS_R_DATA_LAST);
		break;

	default:
		PrintError::print("Proxy::sendSafe", "Undefined type ", gpkt->getKind());
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
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Scheduler #" << myID << ": Receive interscheduler packet. src="
			<< spkt->getSrc() << endl;
	fflush(stdout);
#endif
	if(strcmp(algorithm, SchedulerFactory::DSFQF_ALG)
	        && strcmp(algorithm, SchedulerFactory::DSFQA_ALG)
	        && strcmp(algorithm, SchedulerFactory::DSFQD_ALG)
			&& strcmp(algorithm, SchedulerFactory::DSFQATB_ALG)
			&& strcmp(algorithm, SchedulerFactory::DSFQALB_ALG)) {
		PrintError::print("Proxy", "distributed scheduling algorithm not selected,"
		        " calling handleInterSchedulerPacket is prohibited.");
		return;
	}
	queue->receiveSPacket(spkt);
	delete spkt;
}

void Proxy::propagateSPackets(){
	sPacket * spkt;
	while((spkt = queue->propagateSPacket()) != NULL){
#ifdef PROXY_DEBUG
		cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": send interProxy packet. src="
			<< spkt->getSrc() << endl;
		fflush(stdout);
#endif
		spkt->setKind(PROP_SCH);
		send(spkt, "schg$o");
	}
}

void Proxy::finish(){
#ifdef PROXY_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] Proxy #" << myID << ": finish." << endl;
	fflush(stdout);
#endif
	if(queue != NULL)
		delete queue;
	if(alg_timer != NULL)
		cancelAndDelete(alg_timer);
}

Proxy::~Proxy() {
}
