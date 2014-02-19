#include "DTA.h"

int DTA::initID = 0;

DTA::Request::Request(int id, double dl) {
    this->id = id;
    deadline = dl;
    waiting_ctr = 0;
    total_ctr = 0;
    serving_ctr = 0;
    served_ctr = 0;
    for(int i = 0; i < MAX_DS; ++ i) {
        waiting_subreqs[i] = -1;
     }
}

void DTA::processParameters(const char * alg_param) {
    const char * tmp = alg_param;
    sscanf(tmp, "%lf %d", &p_lag, &dsNum);
    cout << p_lag << " " << dsNum << " " << appNum << " [";
    tmp = strchr(tmp, '[');
    tmp ++;
    for (int i = 0; i < appNum; i ++) {
        sscanf(tmp, "%lf", &(qos_delays[i]));
        cout << " " << qos_delays[i];
        tmp = strchr(tmp, ' ');
        if (tmp == NULL) {
            break;
        }
        tmp ++;
    }
    cout << "]." << endl;
}

DTA::DTA(int id, int deg, int numapps, const char * alg_param) : IQueue(id, deg) {
    appNum = numapps;
    reqList = new map<long, Request *>();
    waitingSubreqs = new map<long, bPacket *>();
    dispatchedSubreqs = new map<long, bPacket *>();
    processParameters(alg_param);
    // Initialize edfQ, lrQ, service_time, disp_int and degrees.
    for (int i = 0; i < dsNum; i ++) {
        edfQ[i] = new DTA_EDF(i, -1, numapps, qos_delays);
        lrQ[i] = new DTA_FIFO(i, -1);
        service_time[i] = 0.001; // Initial value.
        fin_int[i] = 0.001; // Initial value.
        last_fin_time[i] = -1;
        degrees[i] = 0;
    }
    svc_weight = 0.2;
    int_weight = 0.6;
    myID = initID;
    initID ++;
#ifdef SCH_PRINT
    if(id == 0){
        string schprt = "schedprint/sch" + myID;
        schfp = fopen(schprt.c_str(), "w+");
        if(schfp == NULL){
            PrintError::print("SFQ", "Can't open schedprint/sch file.");
        }
    }
#endif
}

void DTA::pushWaitQ(bPacket * pkt) {
#ifdef SCH_DEBUG
    cout << "pushWaitQ:: ";
#endif
    long rid = pkt->getID() / CID_OFFSET * CID_OFFSET + pkt->getSubID(); // Construct the request ID.
    int sid = ((gPacket *)pkt)->getDsID();
#ifdef SCH_DEBUG2
    cout << "pushWaitQ, rid = " << rid << ", server = " << sid
         << ", total_subreqs = " << ((gPacket *)pkt)->getTotalsubreqs() << endl;
#endif
    ((gPacket *)pkt)->setInterceptiontime(SIMTIME_DBL(simTime()));
    map<long, Request *>::iterator it = reqList->find(rid);
    Request * req = NULL;
    if (it == reqList->end()) { // Can't find the request.
        req = new Request(rid, SIMTIME_DBL(simTime()) + qos_delays[pkt->getApp()]);
        req->total_ctr = ((gPacket *)pkt)->getTotalsubreqs();
        reqList->insert(pair<long, Request *>(rid, req));
#ifdef SCH_DEBUG
        cout << "Insert new request: " << rid << "; ";
#endif
    }
    else { // Found the request.
        req = it->second;
#ifdef SCH_DEBUG
        cout << "Use old request: " << rid << "; ";
#endif
    }

    // Clean the EDF queue.
    edfQ[sid]->refineWaitQ(waitingSubreqs);
    edfQ[sid]->pushWaitQ(pkt);

    req->waiting_ctr ++;
    req->waiting_subreqs[sid] = pkt->getID(); // Update the server's slot.
    waitingSubreqs->insert(pair<long, bPacket *>(pkt->getID(), pkt));

    int threshold = (double)(req->total_ctr) * p_lag;
    if ((req->total_ctr - req->serving_ctr - req->served_ctr) <= threshold) { // Considered as lagged.
#ifdef SCH_DEBUG
    cout << "Insert new sub-request: " << pkt->getID() << " to lrQ[" << sid << "]" << endl;
#endif
        lrQ[sid]->pushWaitQ(pkt);
    }
#ifdef SCH_DEBUG
    cout << "Insert new sub-request: " << pkt->getID() << " to edfQ[" << sid << "]" << endl;
#endif
}

void DTA::dispatchUpdate(gPacket * pkt) {
#ifdef SCH_DEBUG
    cout << "dispatchUpdate:: " << endl;
    fflush(stdout);
#endif
    int sid = pkt->getDsID();
    degrees[sid] ++;
    waitingSubreqs->erase(pkt->getID());
    dispatchedSubreqs->insert(pair<long, bPacket *>(pkt->getID(), pkt));

    int rid = pkt->getID() / CID_OFFSET * CID_OFFSET + pkt->getSubID();
    Request * req = reqList->find(rid)->second;
    req->waiting_ctr --;
    req->serving_ctr ++;
    req->waiting_subreqs[sid] = -1;
    int threshold = (double)(req->total_ctr) * p_lag;
    if ((req->total_ctr - req->serving_ctr - req->served_ctr) <= threshold) {
        // Considered as lagged.
        for (int i = 0; i < dsNum; i ++) {
            long pid = req->waiting_subreqs[i];
            if (pid != -1) {
                gPacket * pkt = (gPacket *)(waitingSubreqs->find(pid)->second);
#ifdef SCH_DEBUG
                cout << "Insert sub-req#" << pid << " into lrQ." << endl;
                fflush(stdout);
#endif
                // To prevent repeated insertion,
                // nullify the slot when the insertion is done.
                req->waiting_subreqs[i] = -1;
                lrQ[pkt->getDsID()]->pushWaitQ(pkt);
            }
        }
    }
}

bPacket * DTA::dispatchNext() {
#ifdef SCH_DEBUG
    cout << "dispatchNext:: ";
    fflush(stdout);
#endif
    for (int i = 0; i < dsNum; i ++) {
        if (!edfQ[i]->waitQisEmpty()
            && degrees[i] < degree && degree > 0) {
            bPacket * pkt = tryToDispatch(i);
            if (pkt != NULL) {
#ifdef SCH_DEBUG
                cout << "Dispatch: " << pkt->getID() << endl;
#endif
                return pkt;
            }
        }
    }
#ifdef SCH_DEBUG
    cout << "Nothing to dispatch." << endl;
    fflush(stdout);
#endif
    return NULL;
}

bPacket * DTA::tryToDispatch(int sid) {
#ifdef SCH_DEBUG
    cout << "tryToDispatch serverID#" << sid << ":: ";
    fflush(stdout);
#endif
    if (degrees[sid] >= degree && degree > 0) {
        return NULL;
    }
    gPacket * pkt = NULL;
    gPacket * pkt2 = NULL;
    // Find the first waiting request in EDF queue.
    while (!edfQ[sid]->waitQisEmpty()) {
        pkt = (gPacket *)edfQ[sid]->seeWaitQNext();
        if (waitingSubreqs->find(pkt->getID()) != waitingSubreqs->end()) { // It's waiting.
            break;
        }
        edfQ[sid]->dispatchNext();
        edfQ[sid]->popOsQ(pkt->getID());
    }

    // No waiting request.
    if (edfQ[sid]->waitQisEmpty()) {
#ifdef SCH_DEBUG
        cout << "EDF queue is empty." << endl;
        fflush(stdout);
#endif
        return NULL;
    }

    long rid = pkt->getID() / CID_OFFSET * CID_OFFSET
               + pkt->getSubID(); // Get the request ID.
#ifdef SCH_DEBUG
    cout << "rid = " << rid << ", server = " << sid << endl;
    cout << "EDF queue not empty, get packet #" << pkt->getID() << "; ";
    fflush(stdout);
#endif
    map<long, Request *>::iterator it = reqList->find(rid);
    if (it == reqList->end()) {
        PrintError::print(
                "DTA", "trytoDispatch: request is not found in reqList.");
        return NULL;
    }
    Request * req = it->second;
    double now = SIMTIME_DBL(simTime());
    int app = pkt->getApp();
    // Important comparison to ensure the expected finish time does not violate the deadline.
    if (req->deadline <= now + service_time[sid] ||
        (degrees[sid] == degree - 1
         && req->deadline <= now + service_time[sid] + fin_int[sid])) {
        edfQ[sid]->dispatchNext();
        edfQ[sid]->popOsQ(pkt->getID());
#ifdef SCH_PRINT
        fprintf(schfp, "%lf,%d,Deadline,,%d,,%lf,%lf,%lf,%lf,%d\n",
                SIMTIME_DBL(simTime()), sid, app, req->deadline,
                now, service_time[sid], fin_int[sid], degrees[sid]);
#endif
#ifdef SCH_DEBUG
        cout << "Due to deadline problem, despatch #" << pkt->getID()
             << "; deadline == " << req->deadline << endl;
        fflush(stdout);
#endif
        dispatchUpdate(pkt);
        return pkt;
    }
    else {
#ifdef SCH_DEBUG
        cout << "No deadline problem." << endl;
        fflush(stdout);
#endif
        // Find the first waiting request in LR queue.
        while (!lrQ[sid]->waitQisEmpty()) {
            pkt2 = (gPacket *)lrQ[sid]->seeWaitQNext();
            if (waitingSubreqs->find(pkt2->getID()) != waitingSubreqs->end()) {
                break;
            }
            lrQ[sid]->dispatchNext();
            lrQ[sid]->popOsQ(pkt2->getID());
        }
        if (lrQ[sid]->waitQisEmpty()) { // No waiting request in LR queue.
            edfQ[sid]->dispatchNext();
            edfQ[sid]->popOsQ(pkt->getID());
#ifdef SCH_PRINT
        fprintf(schfp, "%lf,%d,Non-deadline,EDF,%d\n",
                SIMTIME_DBL(simTime()), sid, app);
#endif
#ifdef SCH_DEBUG
            cout << "No request in lrQ, despatch #" << pkt->getID() << endl;
            fflush(stdout);
#endif
            dispatchUpdate(pkt);
            return pkt;
        }
        else { // Dispatch from LR queue.
            lrQ[sid]->dispatchNext();
            lrQ[sid]->popOsQ(pkt2->getID());
#ifdef SCH_PRINT
        fprintf(schfp, "%lf,%d,Non-deadline,LR,%d\n",
                SIMTIME_DBL(simTime()), sid, pkt2->getApp());
#endif
#ifdef SCH_DEBUG
            cout << "Request in lrQ exists, despatch #"
                 << pkt2->getID() << endl;
            fflush(stdout);
#endif
            dispatchUpdate(pkt2);
            return pkt2;
        }
    }
}

bPacket * DTA::popOsQ(long id) {
#ifdef SCH_DEBUG
    cout << "popOsQ:: ";
#endif
    map<long, bPacket *>::iterator it = dispatchedSubreqs->find(id);
    if (it == dispatchedSubreqs->end()) {
        PrintError::print(
            "DTA", "popOsQ: packet is not found in dispatchedSubreqs.");
        return NULL;
    }
    gPacket * pkt = (gPacket *)(it->second);
    dispatchedSubreqs->erase(it);
    degrees[pkt->getDsID()] --;
    int rid = pkt->getID() / CID_OFFSET * CID_OFFSET + pkt->getSubID(); // Get the request ID.
#ifdef SCH_DEBUG
    cout << "rid = " << rid << ", server = " << pkt->getDsID() << endl;
#endif
    map<long, Request *>::iterator rit = reqList->find(rid);
    if (rit == reqList->end()) {
        PrintError::print(
            "DTA", "popOsQ: request is not found in reqList.", rid);
        return NULL;
    }
    Request * req = rit->second;
    req->serving_ctr --;
    req->served_ctr ++;
    if (req->served_ctr == req->total_ctr) { // Done with this request, delete it.
        reqList->erase(rit);
#ifdef SCH_DEBUG2
    cout << "request " << rid << " is deleted." << endl;
#endif
        delete req;
    }

    int sid = pkt->getDsID();
    // The packet may still exists in edfQ or lrQ.
    // To prevent memory leak, refine edfQ and lrQ.
    edfQ[sid]->refineWaitQ(waitingSubreqs);
    lrQ[sid]->refineWaitQ(waitingSubreqs);

    double now = SIMTIME_DBL(simTime());
    service_time[sid] = service_time[sid] * (1 - svc_weight)
                        + (now - pkt->getScheduletime()) * svc_weight;
    if (last_fin_time[sid] != -1) {
        fin_int[sid] = fin_int[sid] * (1 - int_weight)
                       + (now - last_fin_time[sid]) * int_weight;
    }
    last_fin_time[sid] = now;
#ifdef SCH_DEBUG2
    cout << disp_int[0] << " " << disp_int[1] << " " << disp_int[2] << " " << disp_int[3] << endl;
    cout << "\t\t\t\t\t\tservice_time -> " << service_time[0] << " " << service_time[1]
         << " " << service_time[2] << " " << service_time[3] << endl;
#endif
    return pkt;
}

bPacket * DTA::popOsQ(long id, long subid) {
    return popOsQ(id);
}

bPacket * DTA::queryJob(long id) {
    map<long, bPacket *>::iterator it = dispatchedSubreqs->find(id);
    if (it == dispatchedSubreqs->end()) {
        return NULL;
    }
    return it->second;
}

bPacket * DTA::queryJob(long id, long subid) {
    return queryJob(id);
}

DTA::~DTA() {
    if(reqList != NULL){
        delete reqList;
    }
    if(waitingSubreqs != NULL){
        delete waitingSubreqs;
    }
    if(dispatchedSubreqs != NULL){
        delete dispatchedSubreqs;
    }
    for (int i = 0; i < dsNum; i ++) {
        delete edfQ[i];
        delete lrQ[i];
    }
#ifdef SCH_PRINT
    fclose(schfp);
#endif
}

