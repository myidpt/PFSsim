#ifndef DTA_H_
#define DTA_H_

#include <map>
#include <set>
#include "IQueue.h"
#include "DTAFIFO.h"
#include "DTAEDF.h"
using namespace std;

class DTA : public IQueue {
protected:
    class Request{
    public:
        int id;
        double deadline;
        int waiting_ctr;
        int total_ctr;
        int serving_ctr;
        int served_ctr;
        long waiting_subreqs[MAX_DS];
        Request(int id, double dl);
    };
    static int initID;
    int dsNum; // The number of servers.
    double p_lag; // The percentage of postponed packets.
    double qos_delays[MAX_APP]; // Record the delay requirements for each application.
    // Set by the constructor parameters.

    int degrees[MAX_DS];
    map<long, Request *> * reqList; // A map to record all the requests.
    map<long, bPacket *> * waitingSubreqs; // A map to record the waiting sub-requests.
    map<long, bPacket *> * dispatchedSubreqs; // A map to record the dispatched sub-requests.
    DTA_EDF * edfQ[MAX_DS]; // The EDF queue for sub-requests according to deadlines.
    DTA_FIFO * lrQ[MAX_DS]; // The FIFO queue for lagged behind sub-requests.

    // For service time predictions.
    double last_fin_time[MAX_DS]; // This is used to calculate fin_int[].
    double fin_int[MAX_DS]; // Note: this only calculates the expected waiting time in EDF queue.
    double int_weight; // Set in constructor.
    double service_time[MAX_DS];
    double svc_weight; // Set in constructor.

    FILE * schfp;

    // Update the request information upon dispatching the packet.
    void dispatchUpdate(gPacket * pkt);
    void processParameters(const char * alg_param);
public:
    DTA(int id, int deg, int numapps, const char * alg_param);
    void pushWaitQ(bPacket * job);
    bPacket * tryToDispatch(int sid);
    bPacket * dispatchNext();
    bPacket * popOsQ(long id);
    bPacket * popOsQ(long id, long subid);
    bPacket * queryJob(long id);
    bPacket * queryJob(long id, long subid);
    virtual ~DTA();
};

#endif /* DTA_H_ */
