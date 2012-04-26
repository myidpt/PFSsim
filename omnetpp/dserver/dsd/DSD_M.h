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

#ifndef DSD_M_H_
#define DSD_M_H_
#include <iostream>
#include "General.h"
#include "scheduler/FIFO.h"
#include "dserver/dsd/IDSD.h"
#include "dserver/dsd/PVFS2DSD.h"

class DSD_M : public cSimpleModule{
protected:
	static int idInit;
	int myID;
	double parallel_job_proc_time;
	double write_data_proc_time;
	double write_metadata_proc_time; // This is exclusively for the processing of the write request(JOB_REQ) message.
	double read_metadata_proc_time; // This is exclusively for the processing of the write request(JOB_REQ) message.
	double small_io_size_threshold;
	IDSD * dsd;
public:
	DSD_M();
	void initialize();
	void handleMessage(cMessage * cmsg);

	inline void handle_JobReq(gPacket *);
	inline void send_JobResp(gPacket *);
	inline void handle_JobDisp(gPacket *);
	inline void enqueue_dispatch_VFSReqs(gPacket *);
	inline void handle_VFSResp(gPacket *);
	inline void dispatch_VFSReqs();
	inline void sendToVFS(gPacket *);
	inline void sendToEth(gPacket *);
	void finish();
	virtual ~DSD_M();
};

#endif /* DSD_M_H_ */
