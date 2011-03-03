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
#include "General.h"

class LocalFS : public cSimpleModule{
public:
	LocalFS();
	void initialize();
	void handleMessage(cMessage *);
	void handleLFileReq(gPacket *);
	void handleBlkResp(gPacket *);
	bool checkCache(gPacket *); // This is temp, because you may have part of the range in the cache, a boolean value can't stand for it.
	void sendToDSD(gPacket *);
	void sendToDisk(gPacket *);
	virtual ~LocalFS();
};

#endif /* LOCALFS_H_ */
