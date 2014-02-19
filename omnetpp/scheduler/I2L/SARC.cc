// Author: Yonggang Liu. All rights reserved.

#include "SARC.h"

SARC::SARC(int ttlapps, double QoSThroughput[], double interval) {
    appNum = ttlapps;
    replenish_interval = interval;
    next_periodic_replenishment = replenish_interval; // Init next periodic replenishment time.
    // Init the SARC queues.
    cout << "SARC:" << endl;
    for (int i = 0; i < appNum; i ++) {
        max_token[i] = double(QoSThroughput[i]) * replenish_interval;
        cout << "<" << i << " " << max_token[i] << ">" << endl;
        // Init the tokens to be full.
        cur_token[i] = max_token[i];
        // Init the request queues.
        reqQ[i] = new list<bPacket *>();
    }

}

// Only replenishment will trigger this method.
bPacket * SARC::tryToDispatch() {
#ifdef SCH_DEBUG
//    cout << "SARC::tryToDispatch." << endl;
#endif
    for (int i = 0; i < appNum; i ++) {
        if (!reqQ[i]->empty()) {
            bPacket * pkt = reqQ[i]->front();
            if (cur_token[pkt->getApp()] > 0) {
                cur_token[pkt->getApp()] --;
                reqQ[i]->pop_front();
                return pkt;
            }
        }
    }
    return NULL;
}

void SARC::replenishTokens() {
#ifdef SCH_DEBUG
    cout << "SARC::replenishTokens, time=" << SIMTIME_DBL(simTime()) << endl;
#endif
    for (int i = 0; i < appNum; i ++) {
        cur_token[i] = max_token[i];
    }
    next_periodic_replenishment = SIMTIME_DBL(simTime()) + replenish_interval;
}

// Return the packet if there are enough tokens, otherwise, return NULL.
bPacket * SARC::newArrival(bPacket * pkt) {
#ifdef SCH_DEBUG
//    cout << "SARC::newArrival: " << pkt->getID() << endl;
#endif
    int appid = pkt->getApp();
    // See if there are enough tokens.
    if (cur_token[appid] > 0) { // Enough, send to AVATAR.
        cur_token[appid] --;
#ifdef SCH_DEBUG
//    cout << "SARC::enough tokens: " << pkt->getID() << endl;
#endif
        return pkt;
    } else { // Not enough, check to see if need to replenish the SARC tokens.
        reqQ[appid]->push_back(pkt);
#ifdef SCH_DEBUG
//    cout << "SARC::not enough tokens, queued: " << pkt->getID() << endl;
#endif
        return NULL;
    }
}

double SARC::getNextPeriodicReplenishmentTime() {
    return next_periodic_replenishment;
}

SARC::~SARC() {
    for (int i = 0; i < appNum; i ++) {
        delete reqQ[i];
    }
}

