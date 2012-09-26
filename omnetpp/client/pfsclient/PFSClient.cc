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

#include "PFSClient.h"

Define_Module(PFSClient);

int PFSClient::idInit = 0;

PFSClient::PFSClient() {

	dataPacketOutput = NULL;
	metadataPacketOutput = NULL;

	myID = idInit ++;
}

void PFSClient::initialize() {
	maxTransferWindowSize = par("max_transfer_window_size").longValue();
	dataPacketProcessTime = par("data_packet_process_time").doubleValue();
	metadataPacketProcessTime = par("metadata_packet_process_time").doubleValue();

	StreamersFactory streamersfactory;
	string outputMethod = par("data_packet_output_method").stdstringValue();
	if(outputMethod.compare(streamersfactory.FILE_OUTPUT_SIGNITURE) == 0) {

		int count = par("data_packet_output_file_count").longValue();
		string prefixBeforeID = par("data_packet_output_file_prefix_before_client_ID").stdstringValue();
		int clientIDDigits = par("output_file_client_index_digits").longValue();
		string prefixAfterID = par("data_packet_output_file_prefix_after_client_ID").stdstringValue();
		string postfix = par("data_packet_output_file_postfix").stdstringValue();
		int digits = par("data_packet_output_file_index_digits").longValue();

		dataPacketOutput = streamersfactory.createDataPacketOutputStreamer
				(prefixBeforeID, myID, clientIDDigits, prefixAfterID, count, digits, postfix);

	} else if(outputMethod.compare(streamersfactory.NONE_OUTPUT_SIGNITURE) == 0) {
		dataPacketOutput = NULL;
	}

	outputMethod = par("metadata_packet_output_method").stdstringValue();
	if(outputMethod.compare(streamersfactory.FILE_OUTPUT_SIGNITURE) == 0) {

		int count = par("metadata_packet_output_file_count").longValue();
		string prefixBeforeID = par("metadata_packet_output_file_prefix_before_client_ID").stdstringValue();
		int clientIDDigits = par("output_file_client_index_digits").longValue();
		string prefixAfterID = par("metadata_packet_output_file_prefix_after_client_ID").stdstringValue();
		string postfix = par("metadata_packet_output_file_postfix").stdstringValue();
		int digits = par("metadata_packet_output_file_index_digits").longValue();

		metadataPacketOutput = streamersfactory.createMetadataPacketOutputStreamer
				(prefixBeforeID, myID, clientIDDigits, prefixAfterID, count, digits, postfix);

	} else if(outputMethod.compare(streamersfactory.NONE_OUTPUT_SIGNITURE) == 0) {
		metadataPacketOutput = NULL;
	}

	int packetSizeLimit = par("packet_size_limit").longValue();

	string pfssignature = par("pfs_client_signature").stdstringValue();
	if(pfssignature.compare(PVFS2_SIGNATURE) == 0) {
		pfsClientStrategy = new PVFS2ClientStrategy(myID);
		((PVFS2ClientStrategy *)(pfsClientStrategy))->setPacketLengthLimit(packetSizeLimit);
	} else {
		pfsClientStrategy = NULL;
		string errorMessage = "Unknown pfsClient signature." + pfssignature;
		PrintError::print("PFSClient::initialize", errorMessage);
	}
}
void PFSClient::handleMessage(cMessage * message) {
#ifdef MSG_CLIENT
	cout << "Client[" << myID << "] " << MessageKind::getMessageKindString(message->getKind())
	<< " ID=" << ((bPacket *)message)->getID() << endl;
#endif
	switch(message->getKind()) {

	case TRACE_RESP: // Scheduled to send.
		sendAppResponse((AppRequest *)message);
		break;
	case TRACE_REQ: // From Application.
		handleAppRequest((AppRequest *)message);
		break;

	case LAYOUT_REQ: // Scheduled to send.
		sendMetadataPacketRequest((qPacket *)message);
		break;
	case LAYOUT_RESP: // layout information reply.
		handleMetadataPacketResponse((qPacket *)message);
		break;

	case PFS_W_REQ: // Request a write.
	case PFS_R_REQ: // Request a read.
	case PFS_W_DATA: // Writes out.
	case PFS_W_DATA_LAST:
		sendDataPacketRequest((gPacket *)message);
		break;
	case PFS_W_RESP: // A write response.
	case PFS_R_DATA: // Reads in.
	case PFS_R_DATA_LAST:
	case PFS_W_FIN:
		handleDataPacketResponse((gPacket *)message);
		break;

	default:
		PrintError::print("PFSClient::handleMessage", "Unknown message type: ", message->getKind());
		break;
	}
}


void PFSClient::handleAppRequest(AppRequest * request) {
	vector<cPacket *> * packetlist = pfsClientStrategy->handleNewTrace(request);
	processPacketList(packetlist);
}

void PFSClient::handleDataPacketResponse(gPacket * packet) {
	packet->setReturntime(SIMTIME_DBL(simTime()));
	if (dataPacketOutput != NULL) {
		dataPacketOutput->writePacket(packet);
	}
	vector<cPacket *> * packetlist = pfsClientStrategy->handleDataPacketReply(packet);
	processPacketList(packetlist);
}

void PFSClient::handleMetadataPacketResponse(qPacket * packet) {
	packet->setReturntime(SIMTIME_DBL(simTime()));
	if (metadataPacketOutput != NULL) {
		metadataPacketOutput->writePacket(packet);
	}
	vector<cPacket *> * packetlist = pfsClientStrategy->handleMetadataPacketReply(packet);
	processPacketList(packetlist);
}

void PFSClient::sendDataPacketRequest(gPacket * packet) {
	sendToEth(packet);
}

void PFSClient::sendMetadataPacketRequest(qPacket * packet) {
	sendToEth(packet);
}

void PFSClient::sendAppResponse(AppRequest * request) {
	sendToApp(request);
}

void PFSClient::sendToApp(AppRequest * request){
	send(request, "app$o");
}

void PFSClient::sendToEth(cPacket * packet){
	cChannel * cch = gate("eth$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(packet, cch->getTransmissionFinishTime() - simTime(), "eth$o");
	else
		send(packet, "eth$o");
}

void PFSClient::processPacketList(vector<cPacket *> * list) {
	vector<cPacket *>::iterator it;
	double dataPacketRiseTime = SIMTIME_DBL(simTime());
	double metadataPacketRiseTime = SIMTIME_DBL(simTime());
	for (it = list->begin(); it != list->end(); it ++) {
#ifdef PFSCLIENT_DEBUG
	cout << "PFSClient-" << myID << "::processPacketList packetID=" << ((bPacket *)(*it))->getID() << " ";
#endif
		switch ((*it)->getKind()) {
		case TRACE_RESP:
#ifdef PFSCLIENT_DEBUG
	cout << "TRACE_RESP" << endl;
#endif
			((AppRequest *)(*it))->setFinishtime(SIMTIME_DBL(simTime()));
			sendAppResponse((AppRequest *)(*it));
			break;

		case PFS_R_REQ: // Scheduled to send.
		case PFS_W_REQ:
		case PFS_W_DATA: // Write data.
		case PFS_W_DATA_LAST:
#ifdef PFSCLIENT_DEBUG
			if ((*it)->getKind() == PFS_R_REQ) {
				cout << "PFS_R_REQ" << endl;
			} else if ((*it)->getKind() == PFS_W_REQ) {
				cout << "PFS_W_REQ" << endl;
			} else if ((*it)->getKind() == PFS_W_DATA) {
				cout << "PFS_W_DATA" << endl;
			} else if ((*it)->getKind() == PFS_W_DATA_LAST) {
				cout << "PFS_W_DATA_LAST" << endl;
			}
#endif
			if (dataPacketProcessTime != 0) {
				dataPacketRiseTime += dataPacketProcessTime;
				((gPacket *)(*it))->setRisetime(dataPacketRiseTime);
				scheduleAt((simtime_t)dataPacketRiseTime, *it);
			}
			else {
				((gPacket *)(*it))->setRisetime(dataPacketRiseTime);
				sendDataPacketRequest((gPacket *)(*it));
			}
			break;

		case LAYOUT_REQ: // Scheduled to send.
#ifdef PFSCLIENT_DEBUG
	cout << "LAYOUT_REQ" << endl;
#endif
			if (metadataPacketProcessTime != 0) {
				metadataPacketRiseTime += metadataPacketProcessTime;
				((qPacket *)(*it))->setRisetime(metadataPacketRiseTime);
				scheduleAt((simtime_t)metadataPacketRiseTime, *it);
			}
			else {
				((qPacket *)(*it))->setRisetime(metadataPacketRiseTime);
				sendMetadataPacketRequest((qPacket *)(*it));
			}
			break;

		default:
			PrintError::print("PFSClient::processPacketList", "Unknown message type: ", (*it)->getKind());
		}
	}
	delete list;
}

void PFSClient::finish() {
	if(dataPacketOutput != NULL) {
		delete dataPacketOutput;
	}
	if(metadataPacketOutput != NULL){
		delete metadataPacketOutput;
	}
}

PFSClient::~PFSClient() {
}
