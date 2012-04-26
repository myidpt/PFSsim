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
#include "trace/Trace.h"
#include "scheduler/IQueue.h"
#include "scheduler/FIFO.h"

#define MAX_C_OSREQS 32

class Client : public cSimpleModule{
private:
    double trc_proc_time;
    double pkt_proc_time;

	int ms;
	long pktId; // This ID increments, it is the ID of the sent packet.
	int myID; // The ID of this client

	FILE * tfp[MAX_APP_PER_C]; // trace file pointer array
	FILE * rfp[MAX_APP_PER_C]; // result file pointer array
	bool traceEnd[MAX_APP_PER_C];
	Trace * trace[MAX_APP_PER_C]; // Trace is one entry in the trace file; it may be divided into smaller jobs. This is an array for traces of all apps on this client.
	gPacket * traceSync[MAX_APP_PER_C];
	int trcId[MAX_APP_PER_C];
	long pkt_size_limit;
	int small_io_size_threshold; // The max size for small IO.
	int trcs_per_c; // Number of traces to read from.

//	IQueue * reqQ;

	Layout * layoutlist[MAX_FILE]; // The layout information should be temporarily locally cached.

	static int idInit;

protected:
	void initialize();
	void handleMessage(cMessage *msg);
	inline void handle_NewTrace(int appid);
	void send_JobPacket(gPacket *);
	int send_LayoutReq(int appid);
	inline void handle_LayoutResp(qPacket *);
	void schedule_NextPackets(int appid);
	int read_NextTrace(int appid);
	inline void handle_FinishedPacket(gPacket *);
	void pktStatistic(gPacket *);
	void trcStatistic(Trace *);
	inline void sendSafe(cMessage *);
	void finish();
};

#endif
