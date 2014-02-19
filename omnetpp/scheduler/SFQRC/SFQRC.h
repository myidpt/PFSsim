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

class SFQRC : public IQueue {
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
    list<Job*> * overflowWaitQ[MAX_APP];
    list<Job*> * overflowOsQ;

    list<Job*> * basicWaitQ;
    list<Job*> * basicOsQ;

    double maxftags[MAX_APP]; // store the finish tags for each job.
    double vtime; // virtual time
    int startpos; // The application first considered to be dispatched in the algorithm.
    double earliestDeadline; // The nearest deadline among all jobs in the OsQ.
    double earliestVirtualDeadline; // Including the virtual requests.

    double buffer[MAX_APP]; // The buffer.
    double bufferIncRate[MAX_APP]; // The increase rate for the flow buffers.
    double bufferMaxSize[MAX_APP]; // The max size of flow buffer.
    double lastBuffersUpdateTime; // The last time for the flow.
    double QoSDelay[MAX_APP]; // The delay requirement for the service rate.
    double reqSize[MAX_APP];

    double readRate; // The read service rate at the server.
    double writeRate; // The write service rate at the server.
    double seekingTime; // The basic service time that is added to each request.
    long lastAccessFileID; // The last access file ID.
    bool appRead[MAX_APP]; // The mark of read for applications.
    // Note: this assumes the application has unique access pattern.

    double lastServiceTimeUpdateTime;
    double latestFinishTime; // This calculates the service time left of all the real requests on the server.
    double latestVirtualFinishTime; // This calculation includes the virtual requests.

    // In case the predicted finish time is already passed, we need to tune it to now + advanceStep.
    // A larger advanceStep means it is more conservative.
    double advanceStep;

    static FILE * schfp;

    inline void updateVirtualFinishTimeAndEarliestVirtualOSDeadline();
    inline void updateBuffers();
    inline void updateLatestFinishTime();
    inline void updateLatestFinishTimeWithNewJob(Job * job);
    inline void updateOverflowQReqTags();
    inline void updateEarliestDeadline();
    inline void insertBasicWaitQ(Job * job);
    inline bool checkGuarantees(Job * job);
    inline double getServiceTime(Job * job);

    inline void processParameters(const char * param);
    inline void printNJ(Job * job);
    inline void printDP(Job * job);
    inline void printFIN(Job * job);
public:
    SFQRC(int, int, int, const char *);
    virtual void pushWaitQ(bPacket *);
    virtual bPacket * dispatchNext();
    virtual bPacket * popOsQ(long id);
    virtual bPacket * popOsQ(long id, long subid);
    virtual bPacket * queryJob(long id);
    virtual bPacket * queryJob(long id, long subid);
    virtual ~SFQRC();
};

#endif /* SFQRC_H_ */
