// Yonggang Liu

#ifndef IQUEUE_H_
#define IQUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <list>

#include <omnetpp.h>
#include "General.h"

#define NOW SIMTIME_DBL(simTime())
using namespace std;

class IQueue {
protected:
	int myID;
	int degree; // Note that the degree can be negative, which indicates that the degree is not controlled.
	int appNum;
public:
	IQueue(int id, int deg);
	virtual void pushWaitQ(bPacket *) = 0;
	virtual bPacket * dispatchNext() = 0;
	virtual bPacket * popOsQ(long id);
	virtual bPacket * popOsQ(long id, long subid);
    virtual bPacket * queryJob(long id) = 0;
	virtual bPacket * queryJob(long id, long subid);
	virtual sPacket * propagateSPacket();
	virtual void receiveSPacket(sPacket *);
	virtual bool isEmpty();
	virtual double notify(); // Can be called in a future time. The return is next calling event.
	virtual ~IQueue();
};

#endif /* IQUEUE_H_ */
