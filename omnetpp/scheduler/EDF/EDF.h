// Author: Yonggang Liu.

#ifndef EDF_H_
#define EDF_H_

#include "General.h"
#include "FIFO.h"

using namespace std;

class EDF : public FIFO {
private:
    double QoSDelay[MAX_APP];

    inline void processParameters(const char *);
public:
    EDF(int id, int deg, int ttlapps, const char * params);
    EDF(int id, int deg, int ttlapps, double delay[]);
    void pushWaitQ(bPacket *);
    int getNumReqsInWaitQWithinDeadline(double deadline);
    // Get the number of requests in waitQ with deadlines earlier than the deadline parameter.
};

#endif /* EDF_H_ */
