/*
 * Author: Yonggang Liu
 * The PFSClient class simulates the PFS client module.
 */

#include "PFSClient.h"

Define_Module(PFSClient);

int PFSClient::idInit = 0;
int PFSClient::numClients=0;
/*
 * Initialize the ID of this module.
 */
PFSClient::PFSClient() {
	dataPacketOutput = NULL;
	metadataPacketOutput = NULL;
	myID = idInit ++;
}

/*
 * Take the parameters from the ini file to:
 * Create the output streamers, including the data packet records and the metadata packet records.
 * Initialize the PFSClientStrategy.
 */
void PFSClient::initialize() {
	maxTransferWindowSize = par("max_transfer_window_size").longValue();
	dataPacketProcessTime = par("data_packet_process_time").doubleValue();
	metadataPacketProcessTime = par("metadata_packet_process_time").doubleValue();
	if(numClients == 0){//Parse only one time cause it's a static
	    numClients = par("numClients").longValue();
	}
	//In order to rebuild simulation and parse the corrects files
	if(myID >= numClients){
	    idInit=0;
	    myID=myID%numClients;
	}

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

/*
 *                TRACE_REQ                LAYOUT_REQ
 * Application <-------------> PFSClient <-------------> MetadataServer
 *                TRACE_RESP               LAYOUT_RESP
 *                                           PFS_XXX
 *                                       <-------------> DataServer
 *                                           PFS_XXX
 */
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

/*
 * Pass the AppRequest to the PFSClientStrategy.
 * Pass the result packets to the processPacketList to schedule/send them.
 */
void PFSClient::handleAppRequest(AppRequest * request) {
	vector<cPacket *> * packetlist = pfsClientStrategy->handleNewTrace(request);
	processPacketList(packetlist);
}

/*
 * Pass the DataPacketResponse to the PFSClientStrategy.
 * Pass the result packets to the processPacketList to schedule/send them.
 */
void PFSClient::handleDataPacketResponse(gPacket * gpkt) {
    gpkt->setReturntime(SIMTIME_DBL(simTime()));
	if (dataPacketOutput != NULL) {
		dataPacketOutput->writePacket(gpkt);
	}
	vector<cPacket *> * packetlist = pfsClientStrategy->handleDataPacketResponse(gpkt);
	processPacketList(packetlist);
}

/*
 * Pass the MetadataPacketResponse to the PFSClientStrategy.
 * Pass the result packets to the processPacketList to schedule/send them.
 */
void PFSClient::handleMetadataPacketResponse(qPacket * packet) {
	packet->setReturntime(SIMTIME_DBL(simTime()));
	if (metadataPacketOutput != NULL) {
		metadataPacketOutput->writePacket(packet);
	}
	vector<cPacket *> * packetlist = pfsClientStrategy->handleMetadataPacketResponse(packet);
	processPacketList(packetlist);
}

/*
 * Send the DataPacketRequest to Ethernet.
 */
void PFSClient::sendDataPacketRequest(gPacket * packet) {
	sendToEth(packet);
}

/*
 * Send the MetadataPacketRequest to Ethernet.
 */
void PFSClient::sendMetadataPacketRequest(qPacket * packet) {
	sendToEth(packet);
}

/*
 * Send the AppResponse to Application.
 */
void PFSClient::sendAppResponse(AppRequest * request) {
	sendToApp(request);
}

/*
 * Send the AppResponse to Application, because the connection between PFSClient and Application is ideal,
 * there's no need to wait for connection free.
 */
void PFSClient::sendToApp(AppRequest * request){
	send(request, "app$o");
}

/*
 * Send a packet to Ethernet.
 */
void PFSClient::sendToEth(cPacket * packet){
	cChannel * cch = gate("eth$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(packet, cch->getTransmissionFinishTime() - simTime(), "eth$o");
	else
		send(packet, "eth$o");
}

/*
 * This method iterates all the cPackets in the vector parameter, and send them according to the right channel.
 */
void PFSClient::processPacketList(vector<cPacket *> * list) {
	vector<cPacket *>::iterator it;
	double now = SIMTIME_DBL(simTime());
	double dataPacketRiseTime = now;
	double metadataPacketRiseTime = now;
	for (it = list->begin(); it != list->end(); it ++) {
#ifdef PFSCLIENT_DEBUG
	cout << "PFSClient-" << myID << "::processPacketList packetID=" << ((bPacket *)(*it))->getID()
	        << ", size=" << ((bPacket *)(*it))->getSize() << " ";
#endif
		switch ((*it)->getKind()) {
		case TRACE_RESP:
#ifdef PFSCLIENT_DEBUG
	cout << "TRACE_RESP" << endl;
#endif
			((AppRequest *)(*it))->setFinishtime(now);
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
			break;
		}
	}
	delete list;
}

/*
 * Clean the streamers' memory allocation.
 */
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
