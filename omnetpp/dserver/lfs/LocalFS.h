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
#include <map>
#include "General.h"
#include "scheduler/FIFO.h"
using namespace std;

class LocalFS : public cSimpleModule{
protected:
    int degree;
    int page_size;
    int blk_size;
	FIFO * fileReqQ;
	map<long, PageRequest *> * diskIOs;
public:
	LocalFS();
	void initialize();
	void handleMessage(cMessage *);
	void handleNewFileReq(gPacket *);
	void handleDiskReqFromCache(PageRequest *);
	void handlePageResp(PageRequest *);
	void handleBlkResp(DiskRequest *);
	void dispatchNextFileReq();
	void sendToDSD(gPacket *);
	void sendToDisk(DiskRequest *);
	void sendToDiskCache(PageRequest *);
//	int getCachedSize(gPacket *, ICache::pr_t *);
	void finish();
	virtual ~LocalFS();
};

#endif /* LOCALFS_H_ */
