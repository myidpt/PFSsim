// Controller for Interposed 2-level I/O Scheduling Framework for Performance Virtualization

#include "scheduler/I2L/AVATAR.h"

AVATAR::AVATAR(int ttlapps, double delay[], double wsize) {
    appNum = ttlapps;
    windowIndex = 0;
    cout << "AVATAR:" << endl;
    for (int i = 0; i < appNum; i ++) {
        MT_E[i] = 0;
        T_O[i] = 0;
        QoSDelay[i] = delay[i];
        cout << "<" << i << " " << QoSDelay[i] << ">" << endl;
        T_E_All[i] = 0;
        T_O_Index[i] = 0;
    }
    L_E_New = 0;
    L_E_New_Deadline = 0;
    X = 0;
    L_O_max = 0;
    L_O = AVATAR_SAFE_L_O; // Initial value.
    L_O_X_Lower = 0;
    L_O_X_Upper = L_O_max;
    windowSize = wsize;
    currentWindowEnd = windowSize;
    idle_status = false;
    miss_deadline = false;
    queue = new EDF(1, -1, ttlapps, QoSDelay);
}

// Push to the EDF queue.
void AVATAR::pushWaitQ(bPacket * bpkt) {
#ifdef SCH_DEBUG
//    cout << "AVATAR::pushWaitQ:" << bpkt->getID() << endl;
#endif
    ((gPacket *)bpkt)->setInterceptiontime(SIMTIME_DBL(simTime())); // Set the interception time.
    queue->pushWaitQ(bpkt);
    L_E_New ++;
    double deadline = ((gPacket *)bpkt)->getRisetime() + QoSDelay[bpkt->getApp()];
    if (deadline <= currentWindowEnd) {
        L_E_New_Deadline ++;
    }
}

// By default any request in the DEF queue that missed its deadline is dispatched to the storage utility
// queue regardless of the queue threshold of the latter.
// In the following cases, if the queue length at the server is OK, dispatch one more request.
// Called when new requests arrive at the EDF queue;
// when requests depart from the storage utility upon completion of service;
// at the beginning of each time window.
// dispatchNext is a major idle_status change point. Because a new arrival, a popOsQ and a windowEnd are all
// followed by a dispatchNext method.
// Because the "dipatchNext" method keeps being called until a NULL is returned, we only need to set the idle_status
// when "NULL" is going to be returned.
bPacket * AVATAR::dispatchNext() {
#ifdef SCH_DEBUG
    cout << "AVATAR::dispatchNext." << endl;
#endif
    double now = SIMTIME_DBL(simTime());
    int missnum = queue->getNumReqsInWaitQWithinDeadline(now);
    bPacket * pkt = NULL;
    if (missnum != 0 || L_O_Exist() < L_O) { // Missing or degree not full.
        pkt = queue->dispatchNext();
        if (pkt != NULL) {
            T_E_Count[pkt->getApp()] ++;
            T_E_All[pkt->getApp()] += now - ((gPacket *)pkt)->getInterceptiontime();
            ((gPacket *)pkt)->setScheduletime(now);
#ifdef SCH_DEBUG
            if (missnum != 0) {
                cout << "Missed deadline (dl=" << ((gPacket *)pkt)->getRisetime() + QoSDelay[pkt->getApp()]
                        << " < now=" << now << "): " << pkt->getID() << endl;
            }
            else {
                cout << "Degree good (" << L_O_Exist() << " < " << L_O << "): " << pkt->getID() << endl;
            }
#endif
        }
        else {
#ifdef SCH_DEBUG
            cout << "queue empty" << endl;
#endif
            int LO_Exist = L_O_Exist();
            if (!idle_status) {
//                if ((LO_Exist < L_O * 0.9 && !miss_deadline) || (LO_Exist < MIN(L_O, AVATAR_SAFE_L_O))) {
                if (LO_Exist < L_O * 0.9) {
                    // Change idle status in these conditions.
                    idle_status = true;
//                    cout << "idle_status = true, LO_Exist = " << LO_Exist << ", m = " << miss_deadline << endl;
                }
            }
            if (L_O_max < LO_Exist) { // Update L_O_max: max outstanding reqs in one window at the server.
                L_O_max = LO_Exist;
            }
        }
        return pkt;
    }
#ifdef SCH_DEBUG
    cout << "No missing deadline (queuelength = " << L_E_Exist() << ") and degree (" << L_O << ") is full." << endl;
#endif
    if (idle_status) {
        idle_status = false; // Concurrency is full. Change idle status.
//        cout << "idle_status = false" << endl;
    }
    if (L_O_max < L_O_Exist()) { // Update L_O_max: max outstanding reqs in one window at the server.
        L_O_max = L_O_Exist();
    }
    return NULL;
}

// Called when a request is finished.
// idle_status is unchanged here, because dispatchNext will go after it. It's changed in dispatchNext.
bPacket * AVATAR::popOsQ(long id) {
#ifdef SCH_DEBUG
//    cout << "AVATAR::popOsQ." << endl;
#endif
    bPacket * pkt = queue->popOsQ(id);
    T_O_All[pkt->getApp()][T_O_Index[pkt->getApp()]] = SIMTIME_DBL(simTime()) - ((gPacket *)pkt)->getScheduletime();
    T_O_Index[pkt->getApp()] ++;
    X ++;

    return pkt;
}

bPacket * AVATAR::queryJob(long id) {
    return queue->queryJob(id);
}

void AVATAR::windowEnd() {
#ifdef SCH_DEBUG
//    cout << "AVATAR::windowEnd." << endl;
#endif
    cout << "T=" << currentWindowEnd << ": ";
    calculateStatistics();
    int LO_Exist = L_O_Exist();
    int LE_Exist_Deadline = L_E_Exist_Deadline(currentWindowEnd + windowSize);
    int LE_Exist = L_E_Exist();
    int X_Lower = LO_Exist + LE_Exist_Deadline + L_E_New_Deadline;
    int X_Upper = LO_Exist + LE_Exist + L_E_New;
    double E[MAX_APP];

    if (X != 0) {
        L_O_X_Lower = L_O * X_Lower / X;
        L_O_X_Upper = L_O * X_Upper / X;
    } // If X (throughput) == 0, L_O_X_Lower and L_O_X_Upper remain unchanged.

#ifdef SCH_DEBUG2
    cout << "LO_Exist=" << LO_Exist << "; LE_Exist=" << LE_Exist << "; LE_Exist_Deadline=" << LE_Exist_Deadline
            << "; L_E_New=" << L_E_New << "; L_E_New_Deadline=" << L_E_New_Deadline << endl;
    cout << "X_L = " << X_Lower << ", X_U = " << X_Upper <<
            "; L_O_X_L = " << L_O_X_Lower << ", L_O_X_U = " << L_O_X_Upper << ", X = " << X << endl;
    cout << "L_O = " << L_O << ", L_O_max = " << L_O_max << endl;
#endif

    miss_deadline = false;
    for (int i = 0; i < appNum; i ++) {
        if (T_O_Index[i] == 0) { // Requests from this app didn't come during last window.
            L_O_PerApp[i] = AVATAR_INFINITY + 1; // Set to be AVATAR_INFINITY. Need farther clarification.
            continue;
        }
        E[i] = (QoSDelay[i] - MT_E[i]) / T_O[i];
        if (E[i] <= 1) { // You missed the deadline.
            miss_deadline = true;
            idle_status = false; // You also need to change the idle status to busy, to prevent SARC dispatching more.
//            cout << "idle_status = false ---internal" << endl;
        }
#ifdef SCH_DEBUG2
        cout << "E[" << i << "] = " << E[i] << endl;
#endif
        if (L_O_PerApp[i] != AVATAR_INFINITY) { // AVATAR_INFINITY means AVATAR_INFINITY.
            // underload case
            L_O_RT[i] = E[i] * L_O;
#ifdef SCH_DEBUG2
            cout << "L_O_RT[" << i << "] = " << L_O_RT[i] << endl;
#endif
            if (L_O_RT[i] < L_O_X_Lower) {
#ifdef SCH_DEBUG2
                cout << "  case 1: ";
#endif
                L_O_PerApp[i] = AVATAR_INFINITY; // Deadline can't meet. Need overload.
            }
            else if (L_O_RT[i] > L_O_X_Upper) {
#ifdef SCH_DEBUG2
                cout << "  case 2: ";
#endif
                L_O_PerApp[i] = L_O_X_Upper; // You only need so much.
            }
            else if (L_O_RT[i] < L_O || L_O_max >= L_O) {
#ifdef SCH_DEBUG2
                cout << "  case 3: ";
#endif
                L_O_PerApp[i] = L_O_RT[i]; // The requirements for the next window are more stringent than last window.
            }
            else if (L_O_RT[i] >= L_O && L_O_max < L_O) {
#ifdef SCH_DEBUG2
                cout << "  case 4: ";
#endif
                L_O_PerApp[i] = L_O; // Balance state.
            }
#ifdef SCH_DEBUG2
            cout << "L_O_PerApp[" << i << "] = " << L_O_PerApp[i] << endl;
#endif
        }
        else { // Overload case.
            L_O_RT[i] = E[i] * L_O_max;
#ifdef SCH_DEBUG2
            cout << "L_O_RT[" << i << "] = " << L_O_RT[i] << endl;
#endif
            if (X_Lower <= ((double)X * 0.9)) { // Underload.
#ifdef SCH_DEBUG2
                cout << "  case 5: ";
#endif
                L_O_PerApp[i] = MAX(L_O_RT[i], L_O_X_Lower);
//                L_O_PerApp[i] = L_O_RT[i];
            }
            else if (X_Upper > ((double)X * 0.9)) { // Overload.
#ifdef SCH_DEBUG2
                cout << "  case 6: ";
#endif
                L_O_PerApp[i] = AVATAR_INFINITY;
            }
#ifdef SCH_DEBUG2
            cout << "L_O_PerApp[" << i << "] = " << L_O_PerApp[i] << endl;
#endif
        }
    }
    L_O = AVATAR_INFINITY;
    for (int i = 0; i < appNum; i ++) {
        if (L_O > L_O_PerApp[i]) {
            L_O = L_O_PerApp[i];
        }
    }
    windowIndex ++;
    currentWindowEnd += windowSize;

    L_E_New = 0;
    L_E_New_Deadline = 0;
    X = 0;

    // Nullify the statistical variables.
    for (int i = 0; i < appNum; i ++) {
        T_E_All[i] = 0;
        T_E_Count[i] = 0;
        T_O_Index[i] = 0;
    }
    L_O_max = 0;
//    L_O = L_O_max;
    cout << "L_O = " << L_O << endl;
}

double AVATAR::getCurrentWindowEnd() {
    return currentWindowEnd;
}

// The queue size of the the EDF queue. Calculate whenever it is called.
int AVATAR::L_E_Exist() { // The number of requests exist in the EDF queue.
    return queue->getWaitQSize();
}

// Calculate whenever it is called.
int AVATAR::L_E_Exist_Deadline(double t) { // The number of requests exist in EDF queue and deadline is in the next window.
    return queue->getNumReqsInWaitQWithinDeadline(t);
}

int AVATAR::L_O_Exist() {
    return queue->getOsQSize();
}

bool AVATAR::getIdleStatus() {
    return idle_status;
}

// Calculate T_O and MT_E.
void AVATAR::calculateStatistics() {
    // Calculate T_O.
    for (int i = 0; i < appNum; i ++) {
        T_O[i] = getValueByRank(T_O_All[i], T_O_Index[i], RESP_TIME_RANK);
    }
#ifdef SCH_DEBUG2
    cout << "-----------------------T_O [" << T_O[0] << "," << T_O[1] << "]" << endl;
#endif

    // Calculate MT_E.
    for (int i = 0; i < appNum; i ++) {
        MT_E[i] = T_E_All[i] / T_E_Count[i];
    }
#ifdef SCH_DEBUG2
    cout << "-----------------------MT_E [" << MT_E[0] << "," << MT_E[1] << "]" << endl;
#endif
}

// A small utility function to get a value from an array by its value rank (low to high).
double AVATAR::getValueByRank(double array[MAX_REQ_IN_WINDOW], int total, double rank) {
    if (total == 0) { // Nothing.
        return 0;
    }
    if (rank > 1 || rank <= 0) {
        PrintError::print("AVATAR", 0, "getValueByRank: rank is not in (0,1) range.");
        return 0;
    }
    int rankIndex = (double)total * (1-rank);
    // Note that this rank index is reverse : max -> min.
    // We only need to go through 5% of the array to find the 95th min response time.
    double max = 0;
    int maxIndex = -1;
    for (int di = 0; di <= rankIndex; di ++) {
        max = array[di];
        maxIndex = di;
        for (int ci = di + 1; ci < total; ci ++) {
            if (max < array[ci]) {
                max = array[ci];
                maxIndex = ci;
            }
        }
        if (di == rankIndex) {
            return max;
        }
        // Swap.
        array[maxIndex] = array[di];
        array[di] = max;
    }
    return 0;
}

AVATAR::~AVATAR() {

}
