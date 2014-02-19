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
#include <map>
#include "General.h"
#include "dserver/dsd/IDSD.h"
#include "dserver/dsd/PVFS2DSD.h"

class DSD_M : public cSimpleModule{
protected:
	static int idInit;
	int myID;
	int packet_size_limit;
	double parallel_job_proc_time;
	double write_data_proc_time;
    // This is exclusively for the processing of the write request(JOB_REQ) message.
	double write_metadata_proc_time;
	// This is exclusively for the processing of the read request(JOB_REQ) message.
	double read_metadata_proc_time;
	double small_io_size_threshold;
	IDSD * dsd;
	int O_DIRECT;
	// Record the incremental IDs for returned read packets.
	map<long, long> readPktIDMap;
public:
	DSD_M();
	void initialize();
	void handleMessage(cMessage * cmsg);

	inline void handleReadWriteReq(gPacket * gpkt);
	inline void handleSelfWriteReq(gPacket * gpkt);
	inline void handleWriteDataPacket(gPacket * gpkt);
	inline void handleReadDataResp(gPacket * gpkt);

	inline void sendWriteResp(gPacket * gpkt);
	inline void enqueueDispatchVFSReqs(gPacket * gpkt);
	inline void handleVFSResp(gPacket * gpkt);
	inline void dispatchVFSReqs();
	inline void sendToVFS(gPacket * gpkt);
	inline void sendToEth(gPacket * gpkt);
	void finish();
	virtual ~DSD_M();
};

#endif /* DSD_M_H_ */
