/*
 * Author: Yonggang Liu
 * This class implements PFSClientStreategy. It realizes the PVFS2 Client side mechanisms.
 */

#include "PVFS2ClientStrategy.h"

/*
 * Initialize the first request ID, the packet size limit and the queue for AppRequests.
 */
PVFS2ClientStrategy::PVFS2ClientStrategy(int id) : myID(id){
	requestID = CID_OFFSET * myID + RID_OFFSET * 1;
	packet_size_limit = 10000000; // No limit by default.
	appRequestQ = SchedulerFactory::createScheduler(SchedulerFactory::FIFO_ALG, id, 100);
}

/*
 * This is used to see what strategy you are using.
 */
string PVFS2ClientStrategy::getSignature() {
	return "PVFS2";
}

void PVFS2ClientStrategy::setPacketLengthLimit(int limit) {
	packet_size_limit = limit;
}

/*
 * When you get a new AppRequest:
 * 1. Create a Trace object, and push it into the queue.
 * 2. Check if the file this trace is accessing is with its information ready. If no, generate a LAYOUT_REQ
 * packet; if yes, generate the data packets directly.
 */
vector<cPacket *> * PVFS2ClientStrategy::handleNewTrace(AppRequest * request) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleNewTrace TraceID=" << request->getID()
			<< " TraceFileID=" << request->getTraceFileID() << " PFSFileID=" << request->getFileID()
			<< " APP=" << request->getApp() << " HO=" << request->getHighoffset()
			<< " LO=" << request->getLowoffset() << " totalsize=" << request->getTotalSize() << endl;
#endif
	appRequestQ->pushWaitQ(request); // Insert the request to the queue.
	appRequestQ->dispatchNext(); // Dispatch immediately.
	vector<cPacket *> * packetlist = new vector<cPacket *>();

	// Create a new trace for the request.
	WindowBasedTrace * trace = new WindowBasedTrace();
	trace->initialize(request);
	traces.push_back(trace);

	int fileID = request->getFileID();
	PFSFile * pfsfile = pfsFiles.findPFSFile(fileID);
	if (pfsfile == NULL) { // This means the file was never processed.
		PVFS2File * file = new PVFS2File(fileID);
		pfsFiles.addPFSFile(file);
		qPacket * packet = file->createPFSFileMetadataPacket(requestID, myID, 0); // Create the layout query packet.
		packet->setKind(LAYOUT_REQ);
		packetlist->push_back(packet);
	}
	else if (pfsfile->informationIsSet()) { // The information is ready.
		trace->setLayout(pfsfile->getLayout()); // Set the new trace with the PFSFile layout information.
		generateDataPacketRequests(fileID, packetlist);
	}
	// Otherwise the query for information is sent, waiting for Response.

	return packetlist;
}

/*
 * Handle the LAYOUT_RESPONSE packet. Record the PFS file information, update the traces waiting for the information.
 * Try to dispatch the data packet requests accessing this PFS file.
 */
vector<cPacket *> * PVFS2ClientStrategy::handleMetadataPacketResponse(qPacket * packet) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleMetadataPacketResponse packetID=" << packet->getID()
			<< " FileID=" << packet->getFileId() << endl;
#endif
	int fileID = packet->getFileId();
	PVFS2File * file = (PVFS2File *)(pfsFiles.findPFSFile(fileID));
	if (file == NULL) {
		file = new PVFS2File(fileID);
		pfsFiles.addPFSFile(file);
	}
	file->setFromPFSFileMetadataPacket(packet);
	delete packet;
	Layout * layout = file->getLayout();

	// Update the traces accessing the target PFS file.
	vector<WindowBasedTrace *>::iterator it;
	WindowBasedTrace * trace = NULL;
	for (it = traces.begin(); it != traces.end(); it ++) {
		trace = *it;
		if (trace->getFileID() == fileID) {
			trace->setLayout(layout); // Be careful: this layout is created in PFSFile, and this is just a pointer.
		}
	}

	vector<cPacket *> * packetlist = new vector<cPacket *>();
	generateDataPacketRequests(fileID, packetlist);

	return packetlist;
}

/*
 * When a data packet response comes back:
 * Distinguish if it is a READ or WRITE, and if it is the last data packet.
 */
vector<cPacket *> * PVFS2ClientStrategy::handleDataPacketResponse(gPacket * packet) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleDataPacketResponse packetID=" << packet->getID()
			<< " FileID=" << packet->getFileId() << " "
			<< MessageKind::getMessageKindString(packet->getKind()) << endl;
#endif
	vector<cPacket *> * packetlist = new vector<cPacket *>();
	if (packet->getKind() == PFS_R_DATA) {
		// The data to be read has come back, we only care about the last packet. The packets in the middle are
	    // deleted without other actions.
	    delete packet;
		return packetlist;
	}
	else if(packet->getKind() == PFS_W_RESP) {
	    // This is the finish of the first round for a write.
		processDataPacketResponse(packet, packetlist);
	}
	else if(packet->getKind() == PFS_R_DATA_LAST || packet->getKind() == PFS_W_FIN) {
	    // This is the last packet of a read/write.
		processLastDataPacketResponse(packet, packetlist);
	}
	return packetlist;
}

/*
 * Generate a PFS_R_REQ/PFS_W_REQ data packet to request for the data access.
 * This is the first packet to send for requesting a data access. This packet does not have data in it yet.
 * Note that there may be multiple traces accessing the same PFS file waiting to be started. So do a loop to check all.
 */
void PVFS2ClientStrategy::generateDataPacketRequests(int fileID, vector<cPacket *> * packetlist) {
#ifdef PFSCLIENT_DEBUG
    cout << "PVFS2ClientStrategy::generateDataPacketRequests: fileID=" << fileID << endl;
#endif
	gPacket * gpkt = NULL;
	WindowBasedTrace * trace = NULL;
	vector<WindowBasedTrace *>::iterator it;
	for (it = traces.begin(); it != traces.end(); it ++) {
		trace = *it;
		if (trace->getFileID() != fileID) {
			continue;
		}
		while(1){
			gpkt = trace->nextgPacket(); // Get new requset from trace.
			if(gpkt == NULL) {
				return; // Can't schedule more at this moment.
			}

			gpkt->setID(requestID);
			gpkt->setClientID(myID);
			if (gpkt->getRead()) {
				gpkt->setKind(PFS_R_REQ);
			} else {
				gpkt->setKind(PFS_W_REQ);
			}

			requestID += RID_OFFSET;
			if(requestID % CID_OFFSET >= CID_OFFSET - RID_OFFSET) {
				// Due to the limited ID length, don't allow the trace ID to go too big.
				requestID = CID_OFFSET * myID + RID_OFFSET * 1; // Restart it;
			}

			packetlist->push_back(gpkt);
		}
	}
}

/*
 * Process a PFS_W_RESP data packet. This is the response for a write request: PFS_W_REQ.
 * Note that this means you can start to transfer the real data.
 */
void PVFS2ClientStrategy::processDataPacketResponse(gPacket * gpkt, vector<cPacket *> * packetlist) {
	gpkt->setName("PFS_W_DATA");
	long pktid = gpkt->getID() + 1;
	long loff = gpkt->getLowoffset();
	long ub = loff + gpkt->getSize();

	while(1){
		// Push the first sub-request into subreqQ.
		if(loff + packet_size_limit >= ub || gpkt->getRead()){
			// This is the last sub-packet. We push the original request to the subreqQ.
			gpkt->setName("PFS_W_DATA_LAST");
			gpkt->setID(pktid);
			gpkt->setKind(PFS_W_DATA_LAST);
			gpkt->setLowoffset(loff);
			gpkt->setSize(ub - loff);
			packetlist->push_back(gpkt);
			return;
		}else{
			// This is not the last sub-packet.
			gPacket * subpkt = new gPacket(*gpkt);
			subpkt->setID(pktid);
			subpkt->setKind(PFS_W_DATA);
			subpkt->setLowoffset(loff);
			subpkt->setSize(packet_size_limit);
			packetlist->push_back(subpkt);
			loff += packet_size_limit;
			pktid ++;
		}
	}
}

/*
 * Process a PFS_R_DATA_LAST or PFS_W_FIN packet, which are the last packets for a access window.
 * Because the requests are submitted and finished on a window basis, we may have multiple windows of requests to
 * submit and finish for one trace. This method checks if there are remaining access windows to be done for the trace.
 */
void PVFS2ClientStrategy::processLastDataPacketResponse(gPacket * packet, vector<cPacket *> * packetlist) {
	vector<WindowBasedTrace *>::iterator traceIt;
	WindowBasedTrace * trace = NULL;
	for (traceIt = traces.begin(); traceIt != traces.end(); traceIt ++) {
		if ((*traceIt)->getID() == packet->getSubID()) {
			trace = *traceIt;
			break;
		}
	}

	if (trace == NULL) {
		PrintError::print("PVFS2ClientStrategy::processLastDataPacketResponse",
				"Trace is not found for the finished data packet with subID ", packet->getSubID());
	}

	WindowBasedTrace::status ret = trace->finishedgPacket(packet);

	// Update the earliest request finish time.
    AppRequest * request = (AppRequest *)(appRequestQ->queryJob(packet->getSubID()));
    if (request == NULL) {
        PrintError::print("PVFS2ClientStrategy::processLastDataPacketResponse",
                "Cannot find the original AppRequest from the queue.");
        return;
    }
    if (request->getEarliestsubrequestfinishtime() < 0) {
        request->setEarliestsubrequestfinishtime(packet->getFinishtime());
    }

	if(ret == trace->ALL_DONE){ // All done for this trace.
		// Delete the trace.
		traces.erase(traceIt);

		// Retrieve the AppRequest from the queue.
		AppRequest * retRequest = (AppRequest *)(appRequestQ->popOsQ(packet->getSubID()));

		if (retRequest == NULL) {
			PrintError::print("PVFS2ClientStrategy::processLastDataPacketResponse",
					"Cannot find the original AppRequest from the queue.");
		}

		retRequest->setKind(TRACE_RESP);
		packetlist->push_back(retRequest);
	} else if(ret == trace->MORE_TO_SEND){ // You have done the current window, schedule next packets.
		generateDataPacketRequests(packet->getFileId(), packetlist); // set up future event
	} else if(ret == trace->INVALID){
	    PrintError::print("PVFS2ClientStrategy::processLastDataPacketResponse",
	            "The packet is invalid for the current window.");
	}
	delete packet;
}

/*
 * Clean the memory.
 */
PVFS2ClientStrategy::~PVFS2ClientStrategy() {
	if (appRequestQ != NULL){
		delete appRequestQ;
	}
}
