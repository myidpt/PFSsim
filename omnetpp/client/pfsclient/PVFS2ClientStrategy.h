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

#ifndef PVFS2CLIENTSTRATEGY_H_
#define PVFS2CLIENTSTRATEGY_H_

#include "client/pfsclient/PFSClientStrategy.h" // Inheritance
#include "pfsfile/PFSFiles.h"
#include "pfsfile/PVFS2File.h"
#include "trace/WindowBasedTrace.h"
#include "scheduler/FIFO.h"

class PVFS2ClientStrategy : public PFSClientStrategy {
protected:
	const int myID;

	vector<WindowBasedTrace *> traces;
	PFSFiles pfsFiles;
	FIFO * appRequestQ;

	int requestID;

	int packet_size_limit;

	inline void generateJobRequestDataPackets(int fileID, vector<cPacket *> * packetlist);
	inline void processJobResponseDataPacket(gPacket * gpkt, vector<cPacket *> * packetlist);
	inline void processJobFinishLastDataPacket(gPacket * packet, vector<cPacket *> * packetlist);
public:
	PVFS2ClientStrategy(int id);
	string getSignature();
	void setPacketLengthLimit(int limit);
	vector<cPacket *> * handleNewTrace(AppRequest * request); // Don't care about memory management here.
	vector<cPacket *> * handleMetadataPacketReply(qPacket * packet);
	vector<cPacket *> * handleDataPacketReply(gPacket * packet);
	~PVFS2ClientStrategy();
};

#endif /* PVFS2CLIENTSTRATEGY_H_ */
