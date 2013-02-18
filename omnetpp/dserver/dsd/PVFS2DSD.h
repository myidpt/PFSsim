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

#ifndef PVFS2DSD_H_
#define PVFS2DSD_H_
#include "IDSD.h"
#include "scheduler/FIFO.h"

#define MAX_DSD_OSREQS 512 // The max number of outstanding requests on DSD layer.

class PVFS2DSD : public IDSD{
protected:
	struct info_t{
		long id;
		int small_req;
		int obj_size;
		int orig_size;
		long obj_loff;
		long orig_loff;
		long obj_ub; // upper bound for the object-based request
		long orig_ub; // upper bound for the original request
		long hoff;
		int fid;
		int subID;
		int app;
		int read;
		int decision;
		int cid;
		int inQueueID;
	} info[MAX_DSD_OSREQS]; // Keeps the size and offset information of the "new" request, based on the object size.
	bool oslist[MAX_APP]; // Keeps track of the number of outstanding subrequests belonging to one application.
	int packet_size_limit;
	IQueue * subreqQ;
	IQueue * cachedSubreqsBeforeReturn;
	// Request Size/offset may be at this layer. Records the size of the original request when it returns.
public:
	PVFS2DSD(int, int, int, int);
	inline void newReq(gPacket *);
	inline void dispatchPVFSReqs();
	inline gPacket * dispatchNext();
	inline gPacket * finishedReq(gPacket *); // It will return the request with the original offset and size.
	virtual ~PVFS2DSD();
};

#endif /* PVFS2DSD_H_ */
