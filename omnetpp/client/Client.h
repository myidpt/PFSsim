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

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omnetpp.h>
#include "General.h"
#include "request/Request.h"

class Client : public cSimpleModule
{
private:
	int ms;
	FILE * fp;
	FILE * sfp;
	long curId;
	int myId;
	bool traceEnd;
	// Stands for the synchronization prob., which is the prob. that one job is only submitted
	// upon finishing of the previous job.
	// The processing time between receiving of previous job and submission of this job.
	// Only used in synchronized mode.
	double process_time;
	Request * request;
	cMessage * requestSync;

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	int sendJobPacket(gPacket *);
	int sendLayoutQuery(Request *);
	void handleLayoutResponse(qPacket *);
	int scheduleNextPackets();
	int readNextReq();
	void handleFinishedPacket(gPacket *);
	void pktStatistic(gPacket *);
	void reqStatistic(Request *);
	void sendSafe(cMessage *);
	virtual void finish();
};

#endif
