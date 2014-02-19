/*
 * SFQRC.cpp
 *
 *  Created on: Nov 12, 2012
 *      Author: Yonggang Liu
 */

#include "SFQRC_old.h"

FILE * SFQRC_old::schfp = NULL;

SFQRC_old::SFQRC_old(int id, int deg, int tlapps, const char * alg_param) : IQueue (id, deg) {
    // Initialization.
    vtime = 0;
    appNum = tlapps;
    startpos = 0;
    earliestOSDeadline = MAX_DEADLINE + 1;
    earliestVirtualOSDeadline = MAX_DEADLINE + 1;

    serviceRate = 0; // The service rate at the server.
    lastServiceTimeUpdateTime = 0;
    loadLeft = 0; // The load left for the server.
    virtualLoadLeft = 0;
    latestFinishTime = INVALID; // This calculates the service time left of all the real requests on the server.
    latestVirtualFinishTime = INVALID; // This calculation includes the virtual requests.
    osQ = new list<Job*>();

    processParameters(alg_param);

    for(int i = 0; i < MAX_APP; i ++) { // Buffer is full at the start.
        maxftags[i] = - bufferMaxSize[i];
        lastArrivalTime[i] = MINUS_INF;
    }

#ifdef SCH_PRINT
    if(id == 0){
        char schfn[30] = "schedprint/queue";
        schfp = fopen(schfn, "w+");
        if(schfp == NULL){
            PrintError::print("SFQ", "Can't open schedprint/queue file.");
        }
    }
#endif
}

void SFQRC_old::pushWaitQ(bPacket * pkt, double now) {
    // You should update the maxftags.
    updateVirtualFinishTimeAndEarliestVirtualOSDeadline(now);

    struct Job * job = new Job();
    int app = pkt->getApp();
    job->pkt = pkt;
    if(waitQ[app] == NULL){ // Not initialized yet
        waitQ[app] = new list<Job*>();
    }

    if(waitQ[app]->empty()) { // No back-logged jobs.
        int myvtime = vtime - (now - lastArrivalTime[app]) * bufferIncRate[app];
        job->stag = (maxftags[app] > myvtime) ? maxftags[app] : myvtime;
    }
    else { // queue not empty
        job->stag = maxftags[app];
    }

    job->ftag = job->stag + job->pkt->getSize() / weight[app];
    job->deadline = now + QoSDelay[app];

    waitQ[app]->push_back(job); // Push to the waitQ.
    lastArrivalTime[app] = now; // Update the last arrival time.
    maxftags[app] = job->ftag;
#ifdef SCH_PRINT
    printNJ(job);
#endif
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl << "pushWaitQ: "<< " Time = " << now
            << ", ID = " << pkt->getID() << ", size = " << pkt->getSize() << endl;
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
            << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif
}

bPacket * SFQRC_old::dispatchNext(double now){
    // If outstanding queue is bigger than the degree, stop dispatching more jobs.
    if((signed int)(osQ->size()) >= IQueue::degree) {
        return NULL;
    }

    updateFinishTime(now); // Update the load and service time left on the server.

    // Get the job with the lowest start tag.
    double mintag = MAX_TAG_VALUE;
    int minindex = -1;

    // Random, for fairness.
    bool firstround = true;
    int i = startpos;
    while(true){
        if(i >= appNum)
            i = 0;
        if(i == startpos){
            if(firstround == true)
                firstround = false;
            else
                break;
        }
        if(waitQ[i] == NULL || waitQ[i]->empty()){ // No job in queue
            i++;
            continue;
        }
        // Only compare to the front (The earliest job).
        if(minindex == -1 || mintag > waitQ[i]->front()->stag){ // Select the smallest stag.
            // You still need to see if it is OK to dispatch for not violating the deadlines.
            if (checkNewDeadlineGuarantees(waitQ[i]->front(), now)) {
                minindex = i;
                mintag = waitQ[i]->front()->stag;
            }
        }
        i ++;
    }
    if(minindex == -1){ // No job to schedule.
#ifdef SCH_DEBUG
        cout << "---------------------------------" << endl
                << "dispatchNext: NO JOB to schedule. Time = " << now << endl;
        cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
            << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif
        return NULL;
    }

    Job * job = waitQ[minindex]->front();
    waitQ[minindex]->pop_front();
    pushOsQ(job, now);
    startpos = minindex; // Update the start position.
    startpos ++;
    if(startpos >= appNum)
        startpos = 0;

    // Update vtime
    vtime = job->stag;

    // Set the tags for the next one in wait queue.
    if(waitQ[minindex]->empty()){
        maxftags[minindex] = job->ftag;
    }else{ // Set the tags of the next one in the wait queue.
        // This is necessary because in DSFQ-D and DSFQ-F the stags and ftags of requests at front of queue may be changed.
        Job * next = waitQ[minindex]->front();
        next->ftag += job->ftag - next->stag;
        next->stag = job->ftag; // vtime should be equal to job->stag, which is smaller than job->ftag.
//      next->ftag = next->stag + next->pkt->getSize() / weight[minindex];
    }
    bPacket * pkt = job->pkt;

#ifdef SCH_PRINT
    printDP(job);
#endif

#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "dispatchNext: ID = " << pkt->getID() << ", Time = " << now << endl;
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif

    return pkt;
}

void SFQRC_old::pushOsQ(Job * job, double now) {
    osQ->push_back(job);

    // Update the earliestOSDeadline.
    if (earliestOSDeadline > job->deadline) {
        resetEarliestOSDeadline();
    }
    // Update service finish time.
    updateFinishTime(job, now);

#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "pushOsQ: ID = " << job->pkt->getID() << ", Time = " << now << endl;
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif
}

bPacket * SFQRC_old::popOsQ(long id, double now) {
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "popOsQ ID = " << id << ", Time = " << now << endl;
#endif
    bPacket * pkt = NULL;
    if(osQ->empty()){
        PrintError::print("SFQ", "Didn't find the job in OsQ, OsQ is empty.", id);
        return NULL;
    }

    list<Job*>::iterator iter;
    for(iter = osQ->begin(); iter != osQ->end(); iter ++) {
        if ( (*iter)->pkt->getID() == id ){
            pkt = (*iter)->pkt;
            osQ->erase(iter);
#ifdef SCH_PRINT
    printFIN(*iter);
#endif
            delete *iter;
            break;
        }
    }

    if(pkt == NULL){
        PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
        return NULL;
    }

    // loadLeft is reduced.
    loadLeft -= pkt->getSize();
    // Reselect the earliest OS deadline.
    resetEarliestOSDeadline();
    updateFinishTime(now);

#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif
    return pkt;
}

bPacket * SFQRC_old::popOsQ(long id, long subid, double now) {
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "popOsQ ID = " << id << ", Time = " << now << endl;
#endif
    bPacket * pkt = NULL;
    if(osQ->empty()){
        PrintError::print("SFQ", "Didn't find the job in OsQ, OsQ is empty.", id);
        return NULL;
    }

    list<Job*>::iterator iter;
    for(iter = osQ->begin(); iter != osQ->end(); iter ++) {
        if ((*iter)->pkt->getID() == id && (*iter)->pkt->getSubID() == subid){
            pkt = (*iter)->pkt;
            osQ->erase(iter);
#ifdef SCH_PRINT
    printFIN(*iter);
#endif
            delete *iter;
            break;
        }
    }

    if(pkt == NULL){
        PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
        return NULL;
    }

    // loadLeft is reduced.
    loadLeft -= pkt->getSize();
    // Reselect the earliest OS deadline.
    resetEarliestOSDeadline();
    updateFinishTime(now);

#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestOSDeadline << ", EVD = " << earliestVirtualOSDeadline << endl;
#endif
    return pkt;
}

bPacket * SFQRC_old::queryJob(long id){
    bPacket * pkt = NULL;
    if(osQ->empty())
        return NULL;

    list<Job*>::iterator iter;
    for(iter = osQ->begin(); iter != osQ->end(); iter ++){
        if( (*iter)->pkt->getID() == id ){
            pkt = (*iter)->pkt;
            break;
        }
    }
    if(pkt == NULL){
        PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
        return NULL;
    }
    return pkt;
}

bPacket * SFQRC_old::queryJob(long id, long subid){
    bPacket * pkt = NULL;
    if(osQ->empty())
        return NULL;

    list<Job*>::iterator iter;
    for(iter = osQ->begin(); iter != osQ->end(); iter ++){
        if( (*iter)->pkt->getID() == id && (*iter)->pkt->getSubID() == subid){
            pkt = (*iter)->pkt;
            break;
        }
    }
    if(pkt == NULL){
        PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
        return NULL;
    }
    return pkt;
}

SFQRC_old::~SFQRC_old() {
    if(osQ != NULL){
        osQ->clear();
        delete osQ;
    }
    for(int i = 0; i < MAX_APP; i ++){
        if(waitQ[i] != NULL){
            waitQ[i]->clear();
            delete waitQ[i];
        }
    }
}

// This update is with a new job.
void SFQRC_old::updateFinishTime(Job * job, double now) {
#ifdef SCH_DEBUG
//    cout << endl << "-updateServiceLeft: time=" << now << endl;
#endif
    if (latestFinishTime < 0) { // No OS job before.
        latestFinishTime = job->pkt->getSize() / serviceRate + now;
        loadLeft = 0; // reset loadLeft first.
    }
    else {
        if (latestFinishTime >= 0 && latestFinishTime <= now) {
            cout << "latestFinishTime=" << latestFinishTime << ", now=" << now << endl;
            fflush(stdout);
            PrintError::print("SFQRC", myID, "latestFinishTime <= now.");
            latestFinishTime = now;
        }
        latestFinishTime = job->pkt->getSize() / serviceRate + latestFinishTime;
    }
#ifdef SCH_DEBUG
//    cout << "latestFinishTime = " << latestFinishTime << ", now = " << now << ". Set loadLeft = " << loadLeft << endl;
#endif
    loadLeft += job->pkt->getSize();

    updateVirtualFinishTimeAndEarliestVirtualOSDeadline(now);

#ifdef SCH_DEBUG
//    cout << "-updateServiceLeft end." << endl;
#endif
}

void SFQRC_old::updateFinishTime(double now) {
#ifdef SCH_DEBUG
//    cout << endl << "-updateServiceLeft: time=" << now << endl;
#endif
    // Update lastestFinishTime.
    if (osQ->empty()) {
        loadLeft = 0;
        latestFinishTime = INVALID; // Set latest finish time to be invalid.
#ifdef SCH_DEBUG
//    cout << "osQ is empty. latestFinishTime = " << latestFinishTime << endl;
#endif
    } else {
        if (latestFinishTime >= 0 && latestFinishTime <= now) {
            cout << "latestFinishTime=" << latestFinishTime << ", now=" << now << endl;
            fflush(stdout);
            PrintError::print("SFQRC", myID, "latestFinishTime <= now.");
            latestFinishTime = now;
        }
//        loadLeft = (latestFinishTime - now) * serviceRate;
#ifdef SCH_DEBUG
//    cout << "latestFinishTime = " << latestFinishTime << ", now = " << now << ". Set loadLeft = " << loadLeft << endl;
#endif
    }

    updateVirtualFinishTimeAndEarliestVirtualOSDeadline(now);

#ifdef SCH_DEBUG
//    cout << "-updateServiceLeft end." << endl;
#endif
}

void SFQRC_old::updateVirtualFinishTimeAndEarliestVirtualOSDeadline(double now) {
    // Update latestVirtualFinishTime.
#ifdef SCH_DEBUG
//    cout << endl << "-updateVirtualFinishTimeAndEarliestVirtualOSDeadline. Time = " << now << endl;
#endif
    double bufferSize = 0;
    int reqCount = 0;
    virtualLoadLeft = 0;
    double earliestDeadlineInVirtual = MAX_DEADLINE + 1; // Only consider virtual requests.
    for (int i = 0; i < appNum; i ++) {
        // Every one may have virtual requests.
        bufferSize = (now - lastArrivalTime[i]) * bufferIncRate[i];
        if (bufferSize > bufferMaxSize[i]) { // bufferSize cannot be bigger than the max size.
            bufferSize = bufferMaxSize[i];
        }
        if (maxftags[i] <= vtime - bufferSize) { // Update max finish tag.
            maxftags[i] = vtime - bufferSize;
        } else {
            bufferSize = vtime - maxftags[i];
        }
        // Measure how many requests can fit this buffer.
        reqCount = (int) (bufferSize * weight[i] / reqSize[i]);
        if (reqCount > 0) {
            virtualLoadLeft += reqCount * reqSize[i];
            if (earliestDeadlineInVirtual > (QoSDelay[i] + now)) {
                earliestDeadlineInVirtual = QoSDelay[i] + now;
            }
        }
#ifdef SCH_DEBUG
//    cout << "i = " << i << ", reqCount = " << reqCount << "." << endl;
#endif
    }
    // Update latestVirtualFinishTime and earliestVirtualOSDeadline.
    if (latestFinishTime < 0) { // osQ is empty.
        if (virtualLoadLeft == 0) { // Didn't get any virtual load.
            latestVirtualFinishTime = INVALID;
        } else {
            latestVirtualFinishTime = now + virtualLoadLeft / serviceRate;
        }
        earliestVirtualOSDeadline = earliestDeadlineInVirtual;
    } else {
        latestVirtualFinishTime = latestFinishTime + virtualLoadLeft / serviceRate;
        if (earliestDeadlineInVirtual > earliestOSDeadline) {
            earliestVirtualOSDeadline = earliestOSDeadline;
        } else {
            earliestVirtualOSDeadline = earliestDeadlineInVirtual;
        }
    }
#ifdef SCH_DEBUG
//    cout << "latestVirtualFinishTime = " << latestVirtualFinishTime <<
//            ", virtualLoadLeft = " << virtualLoadLeft << "." << endl;
//    cout << "earliestVirtualOSDeadline = " << earliestVirtualOSDeadline <<
//            ", earliestOSDeadline = " << earliestOSDeadline << endl;
#endif
#ifdef SCH_DEBUG
//    cout << "-updateVirtualFinishTimeAndEarliestVirtualOSDeadline end." << endl;
#endif
}

// Update earliestOSDeadline.
void SFQRC_old::resetEarliestOSDeadline() {
#ifdef SCH_DEBUG
//  cout << endl << "resetEarliestOSDeadline." << endl;
#endif
    earliestOSDeadline = MAX_DEADLINE + 1;
    list<Job*>::iterator iter;
    for(iter = osQ->begin(); iter != osQ->end(); iter ++) {
        if ((*iter)->deadline < earliestOSDeadline) {
            earliestOSDeadline = (*iter)->deadline;
        }
    }
    if (earliestVirtualOSDeadline > earliestOSDeadline) { // earliestVirtualOSDeadline <= earliestOSDeadline.
        earliestVirtualOSDeadline = earliestOSDeadline;
    }
#ifdef SCH_DEBUG
//    cout << "earliestVirtualOSDeadline = " << earliestVirtualOSDeadline <<
//            ", earliestOSDeadline = " << earliestOSDeadline << endl;
//    cout << "resetEarliestOSDeadline end." << endl;
#endif
}

void SFQRC_old::processParameters(const char * alg_param) {
    int i;
    double maxsize;
    double incrate;
    double delay;
    double reqsize;
    double w;

    // To get the weights.
    const char * tmp = alg_param;
    cout << " weights:";
    while(1){
        tmp = strchr(tmp, '[');
        if(tmp == NULL)
            break;
        sscanf(tmp, "[%d %lf]", &i, &w);
        weight[i] = w;
        cout << " [" << i << "] $" << weight[i];
        tmp ++;
    }
    cout << "." << endl;

    // To get the max buffer and buffer increase rate.
    tmp = alg_param;
    cout << endl << "max buffer, buffer increase rate and QoSDelay:";
    while(1){
        tmp = strchr(tmp, '<');
        if(tmp == NULL){
            break;
        }
        sscanf(tmp, "<%d %lf %lf %lf %lf>", &i, &maxsize, &incrate, &delay, &reqsize);
        bufferMaxSize[i] = maxsize;
        bufferIncRate[i] = incrate;
        QoSDelay[i] = delay;
        reqSize[i] = reqsize;
        cout << " <" << i << " " << bufferMaxSize[i] << " " << bufferIncRate[i] << " " << QoSDelay[i] << ">";
        tmp ++;
    }

    // To get the initial service rate.
    tmp = alg_param;
    cout << endl << "Initial service rate: ";
    tmp = strchr(tmp, '{');
    if(tmp == NULL) {
        PrintError::print("SFQRC_old::processParameters", 0, "cannot get initial service rate.");
        return;
    }
    sscanf(tmp, "{%lf}", &serviceRate);
    cout << serviceRate << "." << endl;
}

// Check if the job is dispatched, are there violations on deadlines of any outstanding requests?
bool SFQRC_old::checkNewDeadlineGuarantees(Job * job, double now) {
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "checkNewDeadlineGuarantees." << endl;
#endif
    double newLVFT;
    if (latestVirtualFinishTime < 0) {
        newLVFT = job->pkt->getSize() / serviceRate + now;
    } else {
        newLVFT = job->pkt->getSize() / serviceRate + latestVirtualFinishTime;
    }

#ifdef SCH_DEBUG
    cout << "newVFT = " << newLVFT << ", VFT = " << latestVirtualFinishTime
            << ", EVD = " << earliestVirtualOSDeadline << ", job->deadline = " << job->deadline << endl;
#endif
    if (newLVFT > earliestVirtualOSDeadline || newLVFT > job->deadline) { // If dispatched, some deadlines may be violated.
#ifdef SCH_DEBUG
    cout << "-Violated." << endl;
#endif
        return false;
    }
#ifdef SCH_DEBUG
    cout << "-Guaranteed." << endl;
#endif
    return true;
}

void SFQRC_old::printNJ(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tNJ #%d {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}

void SFQRC_old::printDP(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tDP #%d [%.2lf %.2lf] {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), job->stag, job->ftag, waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}
void SFQRC_old::printFIN(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tFIN #%d {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}
