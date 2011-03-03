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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <list>

#include <omnetpp.h>
#include "General.h"

#define NOW SIMTIME_DBL(simTime())
using namespace std;

class IQueue {
protected:
	int degree;
	struct Job{
		gPacket * gpkt;
		double stag; // The start tags for all applications
		double ftag; // The finish tags for all applications
	};
public:
	double weight[MAX_APP]; // Weight for every application

	IQueue(int deg);
	~IQueue();
	virtual void pushWaitQ(gPacket *) = 0;
	virtual gPacket * dispatchNext() = 0;
	virtual gPacket * popOsQ(long id) = 0;
	virtual gPacket * queryJob(long id) = 0;
};

#endif /* IQUEUE_H_ */
