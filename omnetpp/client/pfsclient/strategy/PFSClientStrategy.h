/*
 * Author: Yonggang Liu
 * This abstract class serves as the interface for the strategies on the PFSClient module.
 */

#ifndef PFSCLIENTSTRATEGY_H_
#define PFSCLIENTSTRATEGY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omnetpp.h>

#include "General.h"
#include "scheduler/IQueue.h"
#include "scheduler/FIFO.h"

class PFSClientStrategy {
public:
	virtual vector<cPacket *> * handleNewTrace(AppRequest * request) = 0; // Don't care about memory management here.
	virtual vector<cPacket *> * handleMetadataPacketResponse(qPacket * packet) = 0;
	virtual vector<cPacket *> * handleDataPacketResponse(gPacket * packet) = 0;
	virtual string getSignature() = 0;
	virtual ~PFSClientStrategy();
};

#endif /* PFSCLIENTSTRATEGY_H_ */
