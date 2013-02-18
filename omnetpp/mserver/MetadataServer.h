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

#include <omnetpp.h>
#include <mserver/strategy/PVFS2MetadataServerStrategy.h>

class MetadataServer : public cSimpleModule
{
protected:
	int myID;
	static int initID;

	PFSMetadataServerStrategy * pfsMetadataServerStrategy;

	// Time.
	double metadataPacketProcessTime;
	double dataPacketProcessTime;

	void initialize();
	void handleMessage(cMessage *);

	inline void handleMetadataPacket(qPacket * packet);
	inline void handleDataPacketResponse(gPacket * packet);
	inline void processPacketList(vector<cPacket *> * list);
	inline void sendSafe(cMessage *);
	void finish();
};
