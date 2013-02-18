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
	int degree;
	int appNum;
public:
	IQueue(int id, int deg);
	virtual void pushWaitQ(bPacket *);
	virtual void pushWaitQ(bPacket * packet, double time);
	virtual bPacket * dispatchNext();
	virtual bPacket * dispatchNext(double time);
	virtual bPacket * popOsQ(long id);
    virtual bPacket * popOsQ(long id, double time);
	virtual bPacket * popOsQ(long id, long subid);
    virtual bPacket * popOsQ(long id, long subid, double time);
    virtual bPacket * queryJob(long id) = 0;
	virtual bPacket * queryJob(long id, long subid) = 0;
	virtual sPacket * propagateSPacket();
	virtual void receiveSPacket(sPacket *);
	virtual bool isEmpty();
	virtual ~IQueue();
};

#endif /* IQUEUE_H_ */
