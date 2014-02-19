// Algorithm of "An interposed 2-level I/O scheduling framework for performance virtualization"
// Yonggang Liu

#ifndef I2L_H_
#define I2L_H_

#include "scheduler/IQueue/IQueue.h"
#include "scheduler/FIFO/FIFO.h"
#include "scheduler/I2L/SARC.h"
#include "scheduler/I2L/AVATAR.h"
#define MINIMAL_DBL 0.000001

using namespace std;

class I2L : public IQueue {
protected:
    int myID;
    int degree;
    int appNum;

    double QoSThroughput[MAX_APP];
    double QoSDelay[MAX_APP];

    double SARC_replenish_interval; // Interval for required SARC replenishment.
    double AVATAR_window_size; // AVATAR time window.

    SARC * sarc;
    AVATAR * avatar;

    bool idle_status;

    inline void processParameters(const char *);
public:
    I2L(int, int, int, const char *);
    void pushWaitQ(bPacket *);
    bPacket * dispatchNext();
    bPacket * popOsQ(long id);
    bPacket * queryJob(long id);
    // Called by outside timer whenever a time window ends.
    // Return next interval to the SARC replenishment or window end.
    double notify();
    virtual ~I2L();
};

#endif /* I2L_H_ */
