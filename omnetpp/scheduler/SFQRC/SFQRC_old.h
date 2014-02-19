/*
 * SFQRC.h
 *
 *  Created on: Nov 12, 2012
 *      Author: Yonggang Liu
 */

#ifndef SFQRC_H_
#define SFQRC_H_

#define MAX_DEADLINE    1000000
#define MINUS_INF       -1000000

#include "scheduler/IQueue/IQueue.h"
using namespace std;

class SFQRC_old : public IQueue {
protected:
    static const int MAX_TAG_VALUE = 10000000;
    static const int INVALID = -1;
    double weight[MAX_APP]; // Weight for every application
    struct Job{
        bPacket * pkt;
        double stag; // The start tags for all applications
        double ftag; // The finish tags for all applications
        double deadline; // The deadline for the job.
    };
    list<Job*> * waitQ[MAX_APP];
    list<Job*> * osQ;
    double maxftags[MAX_APP]; // store the finish tags for each job.
    double vtime; // virtual time
    int startpos; // The application first considered to be dispatched in the algorithm.
    double earliestOSDeadline; // The nearest deadline among all jobs in the OsQ.
    double earliestVirtualOSDeadline; // Including the virtual requests.

    double buffer[MAX_APP]; // The buffer.
    double bufferIncRate[MAX_APP]; // The increase rate for the flow buffers.
    double bufferMaxSize[MAX_APP]; // The max size of flow buffer.
    double lastArrivalTime[MAX_APP]; // The last time for the flow.
    double QoSDelay[MAX_APP]; // The delay requirement for the service rate.
    double reqSize[MAX_APP];
    double serviceRate; // The service rate at the server.
    double lastServiceTimeUpdateTime;
    long loadLeft; // The load left for the server.
    long virtualLoadLeft;
    double latestFinishTime; // This calculates the service time left of all the real requests on the server.
    double latestVirtualFinishTime; // This calculation includes the virtual requests.

    static FILE * schfp;

    inline void processParameters(const char * param);
    inline void updateFinishTime(double now);
    inline void updateFinishTime(Job * job, double now);
    inline void resetEarliestOSDeadline();
    inline bool checkNewDeadlineGuarantees(Job * job, double now);
    inline void updateVirtualFinishTimeAndEarliestVirtualOSDeadline(double now);

    inline void printNJ(Job *);
    inline void printDP(Job *);
    inline void printFIN(Job *);
public:
    SFQRC_old(int, int, int, const char *);
    virtual void pushWaitQ(bPacket *, double now);
    virtual bPacket * dispatchNext(double now);
    virtual void pushOsQ(Job *, double now);
    virtual bPacket * popOsQ(long id, double now);
    virtual bPacket * popOsQ(long id, long subid, double now);
    virtual bPacket * queryJob(long id);
    virtual bPacket * queryJob(long id, long subid);
    virtual ~SFQRC_old();
};

#endif /* SFQRC_H_ */
