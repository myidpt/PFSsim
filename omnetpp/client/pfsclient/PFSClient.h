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

#ifndef PFSCLIENT_M_H_
#define PFSCLIENT_M_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omnetpp.h>
#include <typeinfo>

#include "General.h"
#include "scheduler/IQueue.h"
#include "scheduler/FIFO.h"
#include "iostreamer/StreamersFactory.h"
#include "client/pfsclient/PVFS2ClientStrategy.h"

class PFSClient : public cSimpleModule {
protected:
	int myID;
	static int idInit;

	IPacketOutputStreamer * metadataPacketOutput;
	IPacketOutputStreamer * dataPacketOutput;
	PFSClientStrategy * pfsClientStrategy;

	// PFS specific.
	long maxTransferWindowSize;

	// Time overhead.
	double dataPacketProcessTime;
	double metadataPacketProcessTime;

public:
	PFSClient();
	void initialize();
	void handleMessage(cMessage * messsage);
	inline void handleAppRequest(AppRequest * request);
	inline void handleDataPacketResponse(gPacket * packet);
	inline void handleMetadataPacketResponse(qPacket * packet);
	inline void sendDataPacketRequest(gPacket * packet);
	inline void sendMetadataPacketRequest(qPacket * packet);
	inline void sendAppResponse(AppRequest * request);
	inline void sendToEth(cPacket * packet);
	inline void sendToApp(AppRequest * request);
	inline void processPacketList(vector<cPacket *> * list);
	void finish();
	virtual ~PFSClient();
};

#endif /* PFSCLIENT_H_ */
