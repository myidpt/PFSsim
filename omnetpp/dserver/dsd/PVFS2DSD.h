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
#include <map>
#include "IDSD.h"
#include "scheduler/SchedulerFactory.h"

#define MAX_DSD_OSREQS 512 // The max number of outstanding requests on DSD layer.

class PVFS2DSD : public IDSD{
protected:
	struct info_t{
		int small_req;
		int size;
		long loff;
        long hoff;
		long lub; // upper bound for the original request, only the lower part.
		int fid;
		int subID;
		int app;
		int read;
		int decision; // Different meanings for reads and writes.
		int cid;
	};
	// Keeps the size and offset information of the "new" request, based on the object size.
	map<int, info_t *> * infoMap;
    // Keeps track of the number of outstanding subrequests belonging to one application.
	bool oslist[MAX_APP];
	int packet_size_limit;
	IQueue * subreqQ;
public:
	PVFS2DSD(int, int, int);
	// Put a new gPacket into reqQ.
    // Get a gPacket from reqQ:
    // 1. Check if the packet is a write. If so, put COMP information to info->decision. Then dispatch.
    // 2. Resize the request according to the object size. Temporarily store the information into info[].
    // 3. Push the first sub-request to subreqQ->waitQ.
	inline void newReq(gPacket *);
	inline gPacket * dispatchNext();
	// When receive a finished sub-request:
	// 1. Pop it from subreqQ.
	// 2. Find the information for this original request from infoMap.
	// 3. See if this is a write packet.
	// If so, check the original packet to decide if it will be returned.
	// 4. If this is a read packet.
	// Generate the next one and put it into subreqQ (if the next one exists)
	// and send it back.
	// TODO: For read, this is not right. You can't finish one and return one.
	// Because the original request offset may not be aligned,
	// but the one that you are going to return has the offset aligned.
	// In the long term, you need to have a cache. Now I just have a temp list
	// for the subrequests.
	// It will return the request with the original offset and size.
	inline gPacket * finishedReq(gPacket *);
	virtual ~PVFS2DSD();
};

#endif /* PVFS2DSD_H_ */
