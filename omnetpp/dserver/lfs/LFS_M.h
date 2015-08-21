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

#ifndef LFS_M_H_
#define LFS_M_H_

#include <sys/types.h>
#include "General.h"
#include "dserver/lfs/ILFS.h"
#include "dserver/lfs/EXT3.h"
using namespace std;

class LFS_M : public cSimpleModule{
protected:
	static int idInit;
	static int nbDisk;
	ILFS * lfs;
public:
	LFS_M();
	void initialize();
	inline void handleMessage(cMessage *);
	inline void handlePageReq(PageRequest *);
	inline void handleBlkResp(BlkRequest *);
	inline void dispatchNextDiskReq();
	inline void sendToDisk(BlkRequest *);
	inline void sendToDiskCache(PageRequest *);
	inline void sendToVFS(PageRequest *);
//	int getCachedSize(gPacket *, ICache::pr_t *);
	void finish();
	virtual ~LFS_M();
};

#endif /* LFS_M_H_ */
