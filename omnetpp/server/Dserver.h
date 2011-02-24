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

#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <list>

#include <omnetpp.h>
#include "General.h"

#include "FIFO.h"
#include "SFQ.h"

class Dserver : public cSimpleModule
{
private:
	int sockfd;
	int outstanding;
	int degree;
	int myid;
	int wBuffer;
	int rCache;

	struct syncjob_type{
		long id;
		double time;
		long off;
		int len;
		int read; // Read 1 / write 0
	} * syncNojob, *syncEnd, *syncJob;

	struct syncjobreply_type{
		double time; // the time of next event / the finish time for job with fid in disksim.
		long fid; // If one job is finished, the ID of the finished job. Otherwise, -1;
	} * syncReply;

	// For the algorithm you use.
	IQueue * queue;
protected:
	virtual void initialize();
	int sock_init(int portno);
	void intTagInfo_init();
	bool serverIdle;

	virtual void handleMessage(cMessage *);
	void handleDisksimSync(gPacket *);
	void handleJobReq(gPacket *);
	void handleDSFQJobPropgation(gPacket *);
	void handleSchJob(gPacket *);
	void handleDoneJob(long);

	int submit(gPacket *);
	int enqueue(gPacket * pkt, int kind);
	gPacket * dequeue(int kind);
	gPacket * dequeue(long id, int kind);
	int disksim_sync();
	int dispatchJobs();
	int sendResult(gPacket *);
	void sendSafe(gPacket *);
	virtual void finish();
};

#endif
