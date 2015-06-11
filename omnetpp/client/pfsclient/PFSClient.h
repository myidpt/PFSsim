/*
 * Author: Yonggang Liu
 */

#ifndef PFSCLIENT_M_H_
#define PFSCLIENT_M_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omnetpp.h>
#include <typeinfo>

#include "General.h"
#include "scheduler/IQueue/IQueue.h"
#include "scheduler/FIFO/FIFO.h"
#include "iostreamer/StreamersFactory.h"
#include "client/pfsclient/strategy/PVFS2ClientStrategy.h"

class PFSClient : public cSimpleModule {
protected:
	int myID;
	static int idInit;
	static int numClients;

	IPacketOutputStreamer * metadataPacketOutput;
	IPacketOutputStreamer * dataPacketOutput;
	PFSClientStrategy * pfsClientStrategy;

	// PFS specific.
	long maxTransferWindowSize;

	// Time overhead.
	double dataPacketProcessTime;
	double metadataPacketProcessTime;

	inline void handleAppRequest(AppRequest * request);
	inline void handleDataPacketResponse(gPacket * packet);
	inline void handleMetadataPacketResponse(qPacket * packet);

	inline void sendDataPacketRequest(gPacket * packet);
	inline void sendMetadataPacketRequest(qPacket * packet);
	inline void sendAppResponse(AppRequest * request);
	inline void sendToEth(cPacket * packet);
	inline void sendToApp(AppRequest * request);

	inline void processPacketList(vector<cPacket *> * list);
public:
	PFSClient();
	void initialize();
	void handleMessage(cMessage * messsage);
	void finish();
	virtual ~PFSClient();
};

#endif /* PFSCLIENT_H_ */
