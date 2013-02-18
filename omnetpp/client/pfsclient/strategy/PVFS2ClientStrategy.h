/*
 * Author: Yonggang Liu
 */

#ifndef PVFS2CLIENTSTRATEGY_H_
#define PVFS2CLIENTSTRATEGY_H_

#include "PFSClientStrategy.h" // Inheritance
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

	inline void generateDataPacketRequests(int fileID, vector<cPacket *> * packetlist);
	inline void processDataPacketResponse(gPacket * gpkt, vector<cPacket *> * packetlist);
	inline void processLastDataPacketResponse(gPacket * packet, vector<cPacket *> * packetlist);
public:
	PVFS2ClientStrategy(int id);
	vector<cPacket *> * handleNewTrace(AppRequest * request); // Don't care about memory management here.
	vector<cPacket *> * handleMetadataPacketResponse(qPacket * packet);
	vector<cPacket *> * handleDataPacketResponse(gPacket * packet);
	string getSignature();
	void setPacketLengthLimit(int limit);
	~PVFS2ClientStrategy();
};

#endif /* PVFS2CLIENTSTRATEGY_H_ */
