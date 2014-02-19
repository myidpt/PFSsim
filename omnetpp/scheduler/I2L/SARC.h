//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef SARC_H_
#define SARC_H_

#include "IQueue.h"

using namespace std;

class SARC {
protected:
    int appNum;
    double last_refill_time; // SARC last refill time.
    double cur_token[MAX_APP]; // SARC current token number.
    double max_token[MAX_APP]; // SARC refill token number.
    list<bPacket *> * reqQ[MAX_APP]; // SARC queue for the requests that cannot dispatched immediately.

    double replenish_interval;
    double next_periodic_replenishment;
    inline void process_req(bPacket * pkt);
public:
    SARC(int ttlapps, double QoSThroughput[], double interval);
    bPacket * newArrival(bPacket *); // Return the packet if there are enough tokens, otherwise, return NULL.
    bPacket * tryToDispatch();
    bPacket * popOsQ(long id);
    void replenishTokens();
    double getNextPeriodicReplenishmentTime();
    ~SARC();
};

#endif /* SARC_H_ */
