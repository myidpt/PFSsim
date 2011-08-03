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

#ifndef DISK_H_
#define DISK_H_
#include "../General.h"
#include "../scheduler/FIFO.h"

class Disk : public cSimpleModule{
protected:
    int degree;
    int base_port;

	int sockfd;
	int myId;
	int portno;
	int outstanding;
	FIFO * queue;
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

	static int idInit;

	void dispatchJobsAndSync();

public:
	Disk();
	void initialize();
	int sock_init(int portno);
	void handleMessage(cMessage *);
	void handleDataReq(gPacket *);
	void handleDisksimSync();
	int dispatchJobs();
	void sendSafe(gPacket *);
	void finish();
	virtual ~Disk();
};

#endif /* DISK_H_ */
