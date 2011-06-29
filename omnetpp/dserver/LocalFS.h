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

#ifndef LOCALFS_H_
#define LOCALFS_H_

#include <sys/types.h>
#include <list>
#include "../General.h"
#include "../scheduler/FIFO.h"
using namespace std;

class LocalFS : public cSimpleModule{
public:
	struct filereq_type {
		long fileReqId;
		bool readpr; // Indicates if the current pr is a read.
		int blkReqId;
		bool accessingCache; // Indicates currently the cache is being accessed.
		ICache::pr_type * diskread, * diskwrite; // For the current outstanding disk read/write serials
		ICache::pr_type * curaccess;
		filereq_type(long id);
	};
protected:
	list<filereq_type *> * fileReqList;
	ICache * cache;
	FIFO * fileReqQ;
	bool checkStep(filereq_type *); // Check the step you are at, and transit to the next step. If finished, return true.
	filereq_type * findFileReq(long id);
public:
	LocalFS();
	void initialize();
	virtual void handleMessage(cMessage *);
	virtual void handleNewFileReq(gPacket *);
	virtual void handleBlkResp(gPacket *);
	virtual void handleCacheAccessFinish(gPacket *);
	virtual void dispatchNextFileReq();
	virtual void dispatchNextDiskReq(filereq_type *);
	void sendToDSD(gPacket *);
	void sendToDisk(gPacket *);
//	int getCachedSize(gPacket *, ICache::pr_type *);
	virtual ~LocalFS();
};

#endif /* LOCALFS_H_ */
