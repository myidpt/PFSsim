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

#include "Proxy.h"

Define_Module(Proxy);

int Proxy::proxyID = 0;

Proxy::Proxy() {
}

void Proxy::initialize(){
	myID = proxyID;
	proxyID ++;

	algorithm = par("algorithm").longValue();
	const char * alg_param =par("alg_param").stringValue();
	alg_prop_int = par("alg_prop_int").doubleValue(); // This is the time interval.
	double alg_prop_wl = par("alg_prop_wl").doubleValue(); // This is the workload threshold.
	degree = par("degree").longValue();
	newjob_proc_time = par("newjob_proc_time").doubleValue();
	finjob_proc_time = par("finjob_proc_time").doubleValue();
	int numapps = par("numApps").longValue();
	alg_timer = NULL;

	cout << "Proxy #" << myID << " algorithm: ";
	switch(algorithm){
	case FIFO_ALG:
		cout << "FIFO(" << degree << ")." << endl;
		queue = new FIFO(myID, degree);
		break;
	case SFQ_ALG:
		cout << "SFQ(" << degree << ")";
		queue = new SFQ(myID, degree, numapps, alg_param);
		break;
	case DSFQA_ALG:
		cout << "DSFQA(" << degree << ")";
		queue = new DSFQA(myID, degree, numapps, alg_param);
		break;
	case DSFQD_ALG:
		cout << "DSFQD(" << degree << ")";
		queue = new DSFQD(myID, degree, numapps, alg_param);
		break;
	case DSFQF_ALG:
		cout << "DSFQF(" << degree << ")";
		queue = new DSFQF(myID, degree, numapps, alg_param);
		break;
	case DSFQATB_ALG:
		cout << "DSFQA(" << degree << ")-TimeBased";
		queue = new DSFQATB(myID, degree, numapps, alg_param);
		alg_timer = new gPacket("ALG_TIMER");
		alg_timer->setKind(ALG_TIMER);
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + alg_prop_int), alg_timer);
		break;
	case DSFQALB_ALG:
		cout << "DSFQA(" << degree << ")-LoadBased";
		queue = new DSFQALB(myID, degree, numapps, alg_prop_wl, alg_param);
		break;
	case SFQRC_ALG:
        cout << "SFQRC(" << degree << ")";
        queue = new SFQRC(myID, degree, numapps, alg_param);
        break;
	default:
		PrintError::print("Proxy", "Undefined algorithm.");
		deleteModule();
		break;
	}
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
    if (algorithm == SFQRC_ALG) {
        queue->pushWaitQ(gpkt, SIMTIME_DBL(simTime())); // This algorithm requires the current time information.
    } else {
        queue->pushWaitQ(gpkt); // Push in the request.
    }
    if(algorithm == DSFQA_ALG || algorithm == DSFQALB_ALG) { // DSFQA packet propagation is triggered.
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
    if (algorithm == SFQRC_ALG) {
        queuedpacket = (gPacket *)queue->popOsQ(recordPacketID, SIMTIME_DBL(simTime()));
    } else {
        queuedpacket = (gPacket *)queue->popOsQ(recordPacketID);
    }
	delete queuedpacket; // Delete the record.

	if(algorithm == DSFQF_ALG) { // DSFQA packet propagation is triggered.
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
	if(algorithm != DSFQATB_ALG){
		PrintError::print("Proxy", "handleAlgorithmTimer called when algorithm is not DSFQATB_ALG.", algorithm);
	}
		propagateSPackets();
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + alg_prop_int), alg_timer);
}

void Proxy::scheduleJobs(){
	gPacket * jobtodispatch = NULL;
	gPacket * packetcopy = NULL;
	while(1){
	    if (algorithm == SFQRC_ALG) {
            jobtodispatch = (gPacket *)queue->dispatchNext(SIMTIME_DBL(simTime()));
	    } else {
	        jobtodispatch = (gPacket *)queue->dispatchNext();
	    }

		if(jobtodispatch != NULL){
			if(algorithm == DSFQD_ALG) // DSFQA packet propagation is triggered.
				propagateSPackets();
			packetcopy = new gPacket(*jobtodispatch);
			packetcopy->setScheduletime(SIMTIME_DBL(simTime()));
			sendSafe(packetcopy);
		}
		else
			break;
	}
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
	if(algorithm != DSFQF_ALG && algorithm != DSFQA_ALG && algorithm != DSFQD_ALG
			&& algorithm != DSFQATB_ALG && algorithm != DSFQALB_ALG){
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
