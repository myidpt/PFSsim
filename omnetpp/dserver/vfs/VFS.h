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

#ifndef VFS_H_
#define VFS_H_

#include <sys/types.h>
#include "General.h"
#include "scheduler/FIFO.h"
using namespace std;

class VFS : public cSimpleModule{
protected:
    int degree;
    int page_size;
    int blk_size;
	FIFO * fileReqQ;
	FIFO * pageReqQ;
public:
	VFS();
	void initialize();
	inline void handleMessage(cMessage *);
	inline void handleNewFileReq(gPacket *);
	inline void handlePageResp(PageRequest *);
	inline void dispatchPageReqs();
	inline void dispatchNextFileReq();
	inline void sendToDSD(gPacket *);
	inline void sendToDiskCache(PageRequest *);
	inline void sendToLFS(PageRequest *);
//	int getCachedSize(gPacket *, ICache::pr_t *);
	void finish();
	virtual ~VFS();
};

#endif /* VFS_H_ */
