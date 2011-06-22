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
#include "../General.h"
#include "../request/Request.h"

class Client : public cSimpleModule{
private:
	int ms;
	FILE * tfp; // trace file pointer
	FILE * rfp; // result file pointer
	long pktId; // This ID increments, it is the ID of the sent packet.
	int myId; // The ID of this client
	bool traceEnd;
	Request * request; // Request is one entry in the trace file; it may be divided into smaller jobs.
	gPacket * requestSync;
	int reqId;

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void sendJobPacket(gPacket *);
	virtual int sendLayoutQuery();
	virtual void handleLayoutResponse(qPacket *);
	virtual int scheduleNextPackets();
	virtual int readNextReq();
	virtual void handleFinishedPacket(gPacket *);
	virtual void pktStatistic(gPacket *);
	virtual void reqStatistic(Request *);
	virtual void sendSafe(cMessage *);
	virtual void finish();
};

#endif
