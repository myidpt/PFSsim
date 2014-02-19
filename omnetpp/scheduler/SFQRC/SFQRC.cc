/*
 * This algorithm is designed by Yonggang Liu. For the new paper.
 *
 *  Created on: Nov 12, 2012
 *      Author: Yonggang Liu
 */

#include "SFQRC.h"

FILE * SFQRC::schfp = NULL;

SFQRC::SFQRC(int id, int deg, int tlapps, const char * alg_param) : IQueue (id, deg) {
    // Initialization.
    vtime = 0;
    appNum = tlapps;
    startpos = 0;
    earliestDeadline = MAX_DEADLINE + 1;
    earliestVirtualDeadline = MAX_DEADLINE + 1;

    readRate = 0; // The read service rate at the server.
    writeRate = 0; // The write service rate at the server.
    lastAccessFileID = -1; // Initialize.
    lastServiceTimeUpdateTime = 0;
    latestFinishTime = INVALID; // This calculates the service time left of all the real requests on the server.
    latestVirtualFinishTime = INVALID; // This calculation includes the virtual requests.

    overflowOsQ = new list<Job*>();
    basicWaitQ = new list<Job*>();
    basicOsQ = new list<Job*>();

    processParameters(alg_param);

    for(int i = 0; i < appNum; i ++) { // Buffer is full at the start.
        overflowWaitQ[i] = new list<Job*>();
        buffer[i] = bufferMaxSize[i];
        appRead[i] = true; // This is slower, conservative.
    }
    lastBuffersUpdateTime = 0;

#ifdef SCH_PRINT
    if(id == 0){
        char schfn[30] = "schedprint/queue";
        schfp = fopen(schfn, "w+");
        if(schfp == NULL){
            PrintError::print("SFQRC", "Can't open schedprint/queue file.");
        }
    }
#endif
}

void SFQRC::pushWaitQ(bPacket * pkt) {
    double now = SIMTIME_DBL(simTime());
#ifdef SCH_DEBUG
    cout << "--------------------------------------------------------------------------------" << endl;
#endif
    appRead[pkt->getApp()] = ((gPacket *)pkt)->getRead(); // Update the read/write assumption.

    // You should update the maxftags.
    updateBuffers();

    // Put the packet into a Job.
    struct Job * job = new Job();
    int app = pkt->getApp();
    job->pkt = pkt;
    job->deadline = now + QoSDelay[app];

    int stag = 0;
    if (overflowWaitQ[app]->empty() && buffer[app] >= pkt->getSize()) { // No back-logged jobs in overflow queue.
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl << "pushbasicWaitQ: "<< " Time = " << now
            << ", ID = " << pkt->getID() << ", BS = " << buffer[app] << endl;
#endif
        insertBasicWaitQ(job);
        buffer[app] -= pkt->getSize();
        // Update the finish time.
 //       updateLatestFinishTimeWithNewJob(job, now);
    } else {
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl << "pushoverflowWaitQ: "<< " Time = " << now
            << ", ID = " << pkt->getID() << ", BS = " << buffer[app];
#endif
        if (!overflowWaitQ[app]->empty()) {
            stag = overflowWaitQ[app]->back()->ftag;
        } else {
            stag = maxftags[app];
        }
        job->stag = stag > vtime ? stag : vtime;
        job->ftag = job->stag + job->pkt->getSize() / weight[app];
        overflowWaitQ[app]->push_back(job); // Push to the overflowWaitQ.
#ifdef SCH_DEBUG
    cout << " stag = " << job->stag << ", ftag = " << job->ftag << endl;
#endif
        // You don't need to update the tags according to buffer,
        // because they are updated before used: dispatchNext.
    }

#ifdef SCH_PRINT
    printNJ(job);
#endif
#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
            << ", ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif
}

bPacket * SFQRC::dispatchNext(){
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl << "dispatchNext" << endl;
#endif
    // If outstanding queue is bigger than the degree, stop dispatching more jobs.
    if ((signed int)(overflowOsQ->size()) >= IQueue::degree) {
        return NULL;
    }

    // First schedule from basicWaitQ.
    if (!basicWaitQ->empty()) {
#ifdef SCH_DEBUG
        cout << "basicWaitQ is not empty." << endl;
#endif
        Job * job = basicWaitQ->front(); // Front has the soonest deadline.
        basicWaitQ->pop_front();
        basicOsQ->push_back(job); // push osQ.
        updateLatestFinishTimeWithNewJob(job);
        updateEarliestDeadline();
#ifdef SCH_DEBUG
        cout << endl << "From basicWaitQ: " << "ID = " << job->pkt->getID() << " Time = " << SIMTIME_DBL(simTime())
                << ", size = " << job->pkt->getSize() << endl;
        cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
            << ", ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif
        return job->pkt;
    }

    updateBuffers();
    updateLatestFinishTime();
    updateOverflowQReqTags();
    updateEarliestDeadline();

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
        if(overflowWaitQ[i] == NULL || overflowWaitQ[i]->empty()){ // No job in queue
            i++;
            continue;
        }
        // Only compare to the front (The earliest job).
        if(minindex < 0 || mintag > overflowWaitQ[i]->front()->stag){ // Select the smallest stag.
            // You still need to see if it is OK to dispatch for not violating the deadlines.
            if (checkGuarantees(overflowWaitQ[i]->front())) {
                minindex = i;
                mintag = overflowWaitQ[i]->front()->stag;
            }
        }
        i ++;
    }

    if(minindex < 0){ // No job to schedule.
#ifdef SCH_DEBUG
        cout << "---------------------------------" << endl
                << "dispatchNext: NO JOB to schedule. Time = " << SIMTIME_DBL(simTime()) << endl;
        cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
            << ", ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif
        return NULL;
    }

    Job * job = overflowWaitQ[minindex]->front();
    overflowWaitQ[minindex]->pop_front();
    overflowOsQ->push_back(job); // Push osQ.
    updateLatestFinishTimeWithNewJob(job);

    startpos = minindex; // Update the start position.
    startpos ++;
    if(startpos >= appNum)
        startpos = 0;

    // Update vtime
    vtime = job->stag;

    // Set the tags for the next one in wait queue.
    if (overflowWaitQ[minindex]->empty()) {
        maxftags[minindex] = job->ftag;
    } else { // Set the tags of the next one in the wait queue.
        // This is necessary because in DSFQ-D and DSFQ-F the stags and ftags of requests at front of queue may be changed.
        Job * next = overflowWaitQ[minindex]->front();
        next->stag = job->ftag; // vtime should be equal to job->stag, which is smaller than job->ftag.
        next->ftag = next->stag + next->pkt->getSize() / weight[minindex];
    }
    bPacket * pkt = job->pkt;

#ifdef SCH_PRINT
    printDP(job);
#endif

#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "dispatchNext from overflowWaitQ: ID = " << pkt->getID() << ", Time = " << SIMTIME_DBL(simTime()) << endl;
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif

    return pkt;
}

bPacket * SFQRC::popOsQ(long id) {
    return popOsQ(id, 0);
}

bPacket * SFQRC::popOsQ(long id, long subid) {
#ifdef SCH_DEBUG
    cout << "---------------------------------" << endl
            << "popOsQ ID = " << id << ", Time = " << SIMTIME_DBL(simTime()) << endl;
#endif
    bPacket * pkt = NULL;

    list<Job*>::iterator iter;
    for (iter = basicOsQ->begin(); iter != basicOsQ->end(); iter ++) {
        if ((*iter)->pkt->getID() == id) {
            if (subid == 0 || (*iter)->pkt->getSubID() == subid) {
                pkt = (*iter)->pkt;
                basicOsQ->erase(iter);
#ifdef SCH_PRINT
                printFIN(*iter);
#endif
                delete * iter;
                break;
            }
        }
    }

    if (pkt == NULL) {
        for (iter = overflowOsQ->begin(); iter != overflowOsQ->end(); iter ++) {
            if ((*iter)->pkt->getID() == id) {
                if (subid == 0 || (*iter)->pkt->getSubID() == subid) {
                    pkt = (*iter)->pkt;
                    overflowOsQ->erase(iter);
#ifdef SCH_PRINT
                    printFIN(*iter);
#endif
                    delete *iter;
                    break;
                }
            }
        }
    }

    if(pkt == NULL){
        PrintError::print("SFQRC", "Didn't find the job in OsQs!", id);
        return NULL;
    }

    updateLatestFinishTime();

#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime
        << ", ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif
    return pkt;
}

bPacket * SFQRC::queryJob(long id){
    return queryJob(id, 0);
}

bPacket * SFQRC::queryJob(long id, long subid){
    bPacket * pkt = NULL;
    if(overflowOsQ->empty())
        return NULL;

    list<Job*>::iterator iter;
    for (iter = overflowOsQ->begin(); iter != overflowOsQ->end(); iter ++) {
        if ((*iter)->pkt->getID() == id) {
            if (subid == 0 || (*iter)->pkt->getSubID() == subid) {
                pkt = (*iter)->pkt;
                break;
            }
        }
    }

    if (pkt == NULL) {
        for (iter = overflowOsQ->begin(); iter != overflowOsQ->end(); iter ++) {
            if ((*iter)->pkt->getID() == id) {
                if (subid == 0 || (*iter)->pkt->getSubID() == subid) {
                    pkt = (*iter)->pkt;
                    break;
                }
            }
        }
    }
    if(pkt == NULL){
        PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
        return NULL;
    }
    return pkt;
}

SFQRC::~SFQRC() {
    if(overflowOsQ != NULL){
        overflowOsQ->clear();
        delete overflowOsQ;
    }
    for(int i = 0; i < MAX_APP; i ++){
        if(overflowWaitQ[i] != NULL){
            overflowWaitQ[i]->clear();
            delete overflowWaitQ[i];
        }
    }
}

void SFQRC::updateBuffers() {
    double now = SIMTIME_DBL(simTime());
#ifdef SCH_DEBUG
    cout << endl << "-updateBuffers. Time = " << now << endl;
#endif
    for (int i = 0; i < appNum; i ++) {
        // Increase the buffer with the buffer rate times the elapsed time.
        buffer[i] += (now - lastBuffersUpdateTime) * bufferIncRate[i];
        if (buffer[i] > bufferMaxSize[i] && (overflowWaitQ[i]->empty())) { // bufferSize cannot be bigger than the max size.
            buffer[i] = bufferMaxSize[i];
        }
#ifdef SCH_DEBUG
    cout << "buffer[" << i << "] = " << buffer[i] << endl;
#endif
    }
    lastBuffersUpdateTime = now;
}

void SFQRC::updateLatestFinishTime() {
    double now = SIMTIME_DBL(simTime());
#ifdef SCH_DEBUG
    cout << "-updateLatestFinishTime. Time = " << now << endl;
#endif
    // Update latestFinishTime.
    if (basicOsQ->size() + overflowOsQ->size() == 0) { // Server is idle
        latestFinishTime = INVALID;
    } else {
        if (latestFinishTime >= 0 && latestFinishTime <= now) {
#ifdef SCH_DEBUG
            cout << "latestFinishTime=" << latestFinishTime << ", now=" << now << endl;
            cout << "degree of basicOsQ: " << basicOsQ->size() << ", degree of ofOsQ: " << overflowOsQ->size() <<endl;
            fflush(stdout);
#endif
//            PrintError::print("SFQRC", myID, "latestFinishTime <= now.");
            latestFinishTime = now + advanceStep;
        }
    }

    double virtualLoadServiceTime = 0;
    for (int i = 0; i < appNum; i ++) { // buffer
        if (overflowWaitQ[i]->empty()) {
            if (appRead[i] == true) {
                virtualLoadServiceTime += buffer[i] / readRate;
                virtualLoadServiceTime += (buffer[i] / reqSize[i]) * seekingTime;
            } else {
                virtualLoadServiceTime += buffer[i] / writeRate;
            }
        }
    }
    list<Job*>::iterator it;
    for (it = basicWaitQ->begin(); it != basicWaitQ->end(); it ++) { // basicWaitQ
        if (((gPacket*)((*it)->pkt))->getRead()) {
            // Worst case: a seeking for each access.
            virtualLoadServiceTime += (*it)->pkt->getSize() / readRate + seekingTime;
        } else {
            virtualLoadServiceTime += (*it)->pkt->getSize() / writeRate;
        }
    }

    if (latestFinishTime < 0) { // No jobs.
        latestVirtualFinishTime = now + virtualLoadServiceTime;
    } else {
        latestVirtualFinishTime = latestFinishTime + virtualLoadServiceTime;
    }
#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime << ", now = " << now << endl;
#endif
}

void SFQRC::updateLatestFinishTimeWithNewJob(Job * job) {
    double now = SIMTIME_DBL(simTime());
#ifdef SCH_DEBUG
    cout << endl << "-updateLatestFinishTimeWithNewJob. ID = " << job->pkt->getID()
            << ", Time = " << now << endl;
#endif
    updateLatestFinishTime();
    double increment = getServiceTime(job);
    if (latestFinishTime < 0) { // No jobs previously.
        latestFinishTime = now + increment;
    } else {
        latestFinishTime = latestFinishTime + increment;
    }
    latestVirtualFinishTime += increment;
#ifdef SCH_DEBUG
    cout << "FT = " << latestFinishTime << ", VFT = " << latestVirtualFinishTime << ", now = " << now << endl;
#endif
}

// This is a greedy algorithm.
void SFQRC::updateOverflowQReqTags() {
#ifdef SCH_DEBUG
    cout << endl << "-updateOverflowQReqTags, Time = " << SIMTIME_DBL(simTime()) << endl;
#endif
    list<Job *>::iterator it;
    double offset = 0;
    for (int i = 0; i < appNum; i ++) {
        if (overflowWaitQ[i]->empty()) {
#ifdef SCH_DEBUG
    cout << "Queue " << i << " is empty" << endl;
#endif
            continue;
        }
        offset = buffer[i] / weight[i];
        overflowWaitQ[i]->front()->stag -= offset;
        overflowWaitQ[i]->front()->ftag -= offset;
        buffer[i] = 0;
#ifdef SCH_DEBUG
    cout << "Queue " << i << " updated: stag = " << overflowWaitQ[i]->front()->stag
            << ", ftag = " << overflowWaitQ[i]->front()->ftag << ", offset = " << offset << endl;
#endif
    }
}

void SFQRC::updateEarliestDeadline() {
    double now = SIMTIME_DBL(simTime());
#ifdef SCH_DEBUG
    cout << endl << "-updateEarliestDeadline, Time = " << now << endl;
#endif
    // Update earliestDeadline and earliestVirtualDeadline.
    earliestDeadline = MAX_DEADLINE + 1;
    list<Job*>::iterator it;
    for (it = basicOsQ->begin(); it != basicOsQ->end(); it ++) {
        if ((*it)->deadline < earliestDeadline) {
            earliestDeadline = (*it)->deadline;
        }
    }

    earliestVirtualDeadline = earliestDeadline;
    for (int i = 0; i < appNum; i ++) {
        if (overflowWaitQ[i]->empty() && buffer[i] >= reqSize[i]) {
            if (earliestVirtualDeadline > QoSDelay[i] + now) {
                earliestVirtualDeadline = QoSDelay[i] + now;
            }
        }
    }
#ifdef SCH_DEBUG
    cout << "ED = " << earliestDeadline << ", EVD = " << earliestVirtualDeadline << endl;
#endif
}

// Because the waitQ is EDF, we always keep it ordered.
void SFQRC::insertBasicWaitQ(Job * job) {
    list<Job *>::iterator it;
    // begin (front): deadline is the nearest.
    for (it = basicWaitQ->begin(); it != basicWaitQ->end(); it ++) {
        if (((Job*)(*it))->deadline > job->deadline) {
            basicWaitQ->insert(it, job);
            return;
        }
    }
    basicWaitQ->insert(it, job);
}

bool SFQRC::checkGuarantees(Job * job) {
    double serviceTimeIncrement = getServiceTime(job);
#ifdef SCH_DEBUG
    cout << "CheckGuarantees: latestVirtualFinishTime=" << latestVirtualFinishTime
            << ", servicetimeInc=" << serviceTimeIncrement << ", earliestVirtualDeadline="
            << earliestVirtualDeadline << endl;
#endif
    return (latestVirtualFinishTime + serviceTimeIncrement) < earliestVirtualDeadline;
}

double SFQRC::getServiceTime(Job * job) {
    double increment = 0;
    if (((gPacket *)(job->pkt))->getRead()) {
        increment = job->pkt->getSize() / readRate;
        if (lastAccessFileID != ((gPacket *)(job->pkt))->getFileId()) {
            increment += seekingTime;
            lastAccessFileID = ((gPacket *)(job->pkt))->getFileId();
        }
    }else {
        increment = job->pkt->getSize() / writeRate;
    }
    return increment;
}

// [0 1000] [1 1000] <0 4194304 2097152 1 4194304> <1 1572864 3145728 0.5 65536> {32000000 100000000 0.014 0.002}
void SFQRC::processParameters(const char * alg_param) {
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
        PrintError::print("SFQRC::processParameters", 0, "cannot get initial service rate.");
        return;
    }
    sscanf(tmp, "{%lf %lf %lf %lf}", &readRate, &writeRate, &seekingTime, &advanceStep);
    cout << "Read service rate=" << readRate << ", write service rate="
            << writeRate << ", seekting time=" << seekingTime << "." << endl;
}

void SFQRC::printNJ(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tNJ #%d {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), overflowWaitQ[job->pkt->getApp()]->size(), overflowOsQ->size());
#endif
}

void SFQRC::printDP(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tDP #%d [%.2lf %.2lf] {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), job->stag, job->ftag, overflowWaitQ[job->pkt->getApp()]->size(), overflowOsQ->size());
#endif
}
void SFQRC::printFIN(Job * job){
#ifdef SCH_PRINT
    fprintf(schfp, "%.5lf\t[%d]\tFIN #%d {%d #%d}\n",
            SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), overflowWaitQ[job->pkt->getApp()]->size(), overflowOsQ->size());
#endif
}
