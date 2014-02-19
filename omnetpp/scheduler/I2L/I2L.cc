// Algorithm of "An interposed 2-level I/O scheduling framework for performance virtualization"
// It includes SARC and AVATAR.

#include "scheduler/I2L/I2L.h"

I2L::I2L(int id, int deg, int ttlapps, const char * alg_param) : IQueue(id, deg) {
    appNum = ttlapps;
    processParameters(alg_param);

    // Init SARC.
    sarc = new SARC(ttlapps, QoSThroughput, SARC_replenish_interval);

    // Init AVATAR.
    avatar = new AVATAR(ttlapps, QoSDelay, AVATAR_window_size);

    idle_status = true;
}

// Check the credits for the request's application.
// If the SARC queue does not have enough credits, it stays in the SARC queue.
// Call tryToReplenishAndDispatch:
//   <Then check if a replenishment is necessary, if so, replenish and dispatch queued requests.>
// If the SARC queue has enough credits, reduce one credit and dispatch the request to AVATAR.
void I2L::pushWaitQ(bPacket * pkt) {
    // Check the credits for the request's application.
    if (sarc->newArrival(pkt) != NULL) { // Good with SARC. Dispatch to AVATAR. Otherwise, it's queued at sarc.
        avatar->pushWaitQ(pkt);
    }
    else { // Not enough tokens.
        if (idle_status) { // server is idle.
            // Upon a new arrival at a FIFO queue with no available credits,
            // while the spareness status indicates that the storage utility has spare bandwidth available.
            sarc->replenishTokens();
            while((pkt = sarc->tryToDispatch()) != NULL) {
                avatar->pushWaitQ(pkt);
            }
        }
    }
}

// Dispatch next request: check the AVATAR queue.
bPacket * I2L::dispatchNext() {
#ifdef SCH_DEBUG
//    cout << "I2L::dispatchNext." << endl;
#endif
    bPacket * pkt = avatar->dispatchNext();
    if (pkt == NULL) {
        if ((!idle_status) && avatar->getIdleStatus()) { // busy -> idle.
            // Upon AVATAR changing the spareness status and indicating that spare bandwidth has become available.
            sarc->replenishTokens();
            while((pkt = sarc->tryToDispatch()) != NULL) {
                avatar->pushWaitQ(pkt);
            }
            pkt = avatar->dispatchNext();
        }
        idle_status = avatar->getIdleStatus();
    }
    return pkt;
}

bPacket * I2L::popOsQ(long id) {
#ifdef SCH_DEBUG
//    cout << "I2L::popOsQ." << endl;
#endif
    return avatar->popOsQ(id);
}

bPacket * I2L::queryJob(long id) {
    return avatar->queryJob(id);
}

// Called by outside timer whenever a replenishment is due or a time window ends.
// Return next interval to the SARC replenishment or window end.
double I2L::notify() {
#ifdef SCH_DEBUG
//    cout << "I2L::notification." << endl;
#endif
    double now = SIMTIME_DBL(simTime());
    if (now + MINIMAL_DBL >= sarc->getNextPeriodicReplenishmentTime()) {
        sarc->replenishTokens();
        // Try to dispatch the requests buffered in the buckets after the replenishment.
        bPacket * pkt = NULL;
        while((pkt = sarc->tryToDispatch()) != NULL) {
            avatar->pushWaitQ(pkt);
        }
        // Dispatch operation for AVATAR is controlled by I2L::dispatchNext.
    }
    else if (now + MINIMAL_DBL >= avatar->getCurrentWindowEnd()) {
        avatar->windowEnd();
    }
    if (sarc->getNextPeriodicReplenishmentTime() > avatar->getCurrentWindowEnd()) {
        return avatar->getCurrentWindowEnd();
    }
    else {
        return sarc->getNextPeriodicReplenishmentTime();
    }
}

// <0 220 2> <1 200 0.5> {0.5 1}
void I2L::processParameters(const char * alg_param) {
    int i;
    int throughput;
    double delay;

    // To get the max buffer and buffer increase rate.
    const char * tmp = alg_param;
    cout << endl << "QoSThroughput and QoSDelay:" << endl;
    while(1){
        tmp = strchr(tmp, '<');
        if(tmp == NULL){
            break;
        }
        sscanf(tmp, "<%d %d %lf>", &i, &throughput, &delay);
        QoSThroughput[i] = throughput;
        QoSDelay[i] = delay;
        cout << "<" << i << " " << throughput << " " << delay << ">" << endl;
        tmp ++;
    }

    // To get the window size and replenish time interval.
    tmp = alg_param;
    tmp = strchr(tmp, '{');
    if(tmp == NULL) {
        PrintError::print("SFQRC::processParameters", 0, "cannot get initial service rate.");
        return;
    }
    sscanf(tmp, "{%lf %lf}", &SARC_replenish_interval, &AVATAR_window_size);
    cout << "SARC_replenish_interval=" << SARC_replenish_interval
            << "s, AVATAR_time_window=" << AVATAR_window_size << "s." << endl;
    if (SARC_replenish_interval <= 0 || AVATAR_window_size <= 0) {
        PrintError::print("I2L", myID,
                "SARC_replenish_interval or AVATAR_time_window is equal to or smaller than 0.");
    }
}

I2L::~I2L() {
    delete sarc;
    delete avatar;
}
