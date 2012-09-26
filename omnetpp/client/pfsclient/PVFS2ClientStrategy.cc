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

#include "PVFS2ClientStrategy.h"

PVFS2ClientStrategy::PVFS2ClientStrategy(int id) : myID(id){
	requestID = CID_OFFSET * myID + RID_OFFSET * 1;
	packet_size_limit = 10000000; // No limit by default.
	appRequestQ = new FIFO(123, 100);
}

string PVFS2ClientStrategy::getSignature() {
	return "PVFS2";
}

void PVFS2ClientStrategy::setPacketLengthLimit(int limit) {
	packet_size_limit = limit;
}

vector<cPacket *> * PVFS2ClientStrategy::handleNewTrace(AppRequest * request) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleNewTrace TraceID=" << request->getID()
			<< " TraceFileID=" << request->getTraceFileID() << " PFSFileID=" << request->getFileID()
			<< " APP=" << request->getApp() << " HO=" << request->getHighoffset()
			<< " LO=" << request->getLowoffset() << endl;
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
	if (pfsfile == NULL) { // This means the file is never processed.
		PVFS2File * file = new PVFS2File(fileID);
		pfsFiles.addPFSFile(file);
		qPacket * packet = file->createPFSFileMetadataPacket(requestID, myID, 0); // Create the layout query packet.
		packet->setKind(LAYOUT_REQ);
		packetlist->push_back(packet);
	}
	else if (pfsfile->informationIsSet()) { // The information is ready.
		trace->setLayout(pfsfile->getLayout()); // Set the new trace with the PFSFile layout information.
		generateJobRequestDataPackets(fileID, packetlist);
	}
	// Otherwise the query for information is sent, waiting for reply.

	return packetlist;
}

vector<cPacket *> * PVFS2ClientStrategy::handleMetadataPacketReply(qPacket * packet) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleMetadataPacketReply packetID=" << packet->getID()
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

	// Update the traces with the layout.
	vector<WindowBasedTrace *>::iterator it;
	WindowBasedTrace * trace = NULL;
	for (it = traces.begin(); it != traces.end(); it ++) {
		trace = *it;
		if (trace->getFileID() == fileID) {
			trace->setLayout(layout); // Be careful: this layout is created in PFSFile, and this is just a pointer.
		}
	}

	vector<cPacket *> * packetlist = new vector<cPacket *>();
	generateJobRequestDataPackets(fileID, packetlist);

	return packetlist;
}

vector<cPacket *> * PVFS2ClientStrategy::handleDataPacketReply(gPacket * packet) {
#ifdef PFSCLIENT_DEBUG
	cout << "PVFS2ClientStrategy-" << myID << "::handleDataPacketReply packetID=" << packet->getID()
			<< " FileID=" << packet->getFileId() << " " << MessageKind::getMessageKindString(gpkt->getKind()) << endl;
#endif
	vector<cPacket *> * packetlist = new vector<cPacket *>();
	if (packet->getKind() == PFS_R_DATA) {
		delete packet;
		return packetlist;
	}
	else if(packet->getKind() == PFS_W_RESP) { // This is the finish of the first round for a write.
		processJobResponseDataPacket(packet, packetlist);
	}
	else if(packet->getKind() == PFS_R_DATA_LAST || packet->getKind() == PFS_W_FIN) { // This is the last packet of a read/write.
		processJobFinishLastDataPacket(packet, packetlist);
	}
	return packetlist;
}


/// <summary>
/// Generate a JOB_REQ data packet to request for the data access. This is the first packet to send for a data access.
/// Note that there may be multiple traces waiting to be started. So do a loop to check all.
/// </summary>
void PVFS2ClientStrategy::generateJobRequestDataPackets(int fileID, vector<cPacket *> * packetlist) {
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

/// <summary>
/// Process a PFS_W_RESP data packet. This is the finish of the first round for a write.
/// Note that this is in the middle of a WRITE. Have got the PFS_W_RESP. Start to transfer the real data.
/// </summary>
void PVFS2ClientStrategy::processJobResponseDataPacket(gPacket * gpkt, vector<cPacket *> * packetlist) {
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

void PVFS2ClientStrategy::processJobFinishLastDataPacket(gPacket * packet, vector<cPacket *> * packetlist) {
	vector<WindowBasedTrace *>::iterator traceIt;
	WindowBasedTrace * trace = NULL;
	for (traceIt = traces.begin(); traceIt != traces.end(); traceIt ++) {
		if ((*traceIt)->getID() == packet->getSubID()) {
			trace = *traceIt;
			break;
		}
	}

	if (trace == NULL) {
		PrintError::print("PVFS2ClientStrategy::processJobFinishLastDataPacket",
				"Trace is not found for the finished data packet with subID ", packet->getSubID());
	}

	WindowBasedTrace::status ret = trace->finishedgPacket(packet);

	if(ret == trace->ALL_DONE){ // All done for this trace.
		// Delete the trace.
		traces.erase(traceIt);

		// Retrieve the AppRequest from the queue.
		AppRequest * retRequest = (AppRequest *)(appRequestQ->popOsQ(packet->getSubID()));

		if (retRequest == NULL) {
			PrintError::print("PVFS2ClientStrategy::processJobFinishLastDataPacket",
					"Cannot find the original AppRequest from the queue.");
		}

		retRequest->setKind(TRACE_RESP);
		packetlist->push_back(retRequest);
	} else if(ret == trace->MORE_TO_SEND){ // You have done the current window, schedule next packets.
		generateJobRequestDataPackets(packet->getFileId(), packetlist); // set up future event
	}
	delete packet;
}

PVFS2ClientStrategy::~PVFS2ClientStrategy() {
	if (appRequestQ != NULL){
		delete appRequestQ;
	}
}
