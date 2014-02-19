// Author: Yonggang Liu.

#include "EDF.h"

EDF::EDF(int id, int deg, int ttlapps, const char * alg_param) : FIFO(id, deg) {
    appNum = ttlapps;
    processParameters(alg_param);
}

EDF::EDF(int id, int deg, int ttlapps, double delay[]) : FIFO(id, deg) {
    appNum = ttlapps;
    for (int i = 0; i < ttlapps; i ++) {
        QoSDelay[i] = delay[i];
    }
}

// Push one job to the waitQ.
// -> back --- front ->
void EDF::pushWaitQ(bPacket * bpkt) {
    int appid = bpkt->getApp();
    double targetdl = ((gPacket *)bpkt)->getRisetime() + QoSDelay[appid];
//    cout << "pushWaitQ: " << ((gPacket *)bpkt)->getRisetime() << ", " << QoSDelay[appid]
//            << ", " << targetdl << endl;
    double tmpdl;
    bPacket * tmpbpkt;
    list<bPacket *>::iterator it;
    for(it = waitQ->begin(); it != waitQ->end(); it ++){
        tmpbpkt = *it;
        tmpdl = ((gPacket *)tmpbpkt)->getRisetime() + QoSDelay[tmpbpkt->getApp()];
        if (targetdl < tmpdl) {
            fflush(stdout);
            waitQ->insert(it, bpkt);
            fflush(stdout);
            return;
        }
    }
    waitQ->push_back(bpkt);
}

// Get the number of requests with deadlines earlier than the deadline parameter.
int EDF::getNumReqsInWaitQWithinDeadline(double deadline) {
    list<bPacket *>::iterator it;
    double pktdeadline;
    int count = 0;
    for(it=waitQ->begin(); it != waitQ->end(); it++){
        pktdeadline = ((gPacket *)(*it))->getRisetime() + QoSDelay[(*it)->getApp()];
        if(pktdeadline < deadline){
            count ++;
        } else { // This is EDF. Deadlines ordered.
            break;
        }
    }
    return count;
}

void EDF::processParameters(const char * alg_param) {
    int i;
    double delay;
    const char * tmp = alg_param;
    cout << endl << "QoSDelay:" << endl;
    while(1){
        tmp = strchr(tmp, '<');
        if(tmp == NULL){
            break;
        }
        sscanf(tmp, "<%d %lf>", &i, &delay);
        QoSDelay[i] = delay;
        cout << "<" << i << " " << delay << ">" << endl;
        tmp ++;
    }
}
