// The SSFQ algorithm realizes packet splitting/assembling in addition to the original SFQ algorithm.
// Yonggang Liu

#ifndef SSFQ_H_
#define SSFQ_H_

#include "SFQ.h"
using namespace std;

class SSFQ : public SFQ{
protected:
	map<long, int> * subReqNum; // Keeps the record of the number of sub-requests each request still needs to receive.
	map<long, bPacket *> * reqList; // Stores the requests, which can be referred by the ID.

	long maxSubReqSize;

public:
	SSFQ(int, int, int, const char *);
	virtual void pushWaitQ(bPacket *);
	virtual bPacket * popOsQ(long id);
	virtual ~SSFQ();
};

#endif /* SSFQ_H_ */
