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

#include "MetadataServer.h"

Define_Module(MetadataServer);

int MetadataServer::initID = 0;

void MetadataServer::initialize()
{
	myID = initID;
	initID ++;

	metadataPacketProcessTime = par("metadata_proc_time").doubleValue();
	dataPacketProcessTime = par("data_proc_time").doubleValue();

	string pfsSignature = par("pfs_metadataserver_signature").stdstringValue();

	int pfsInputFileNum = par("pfs_input_file_num").longValue();
	int pfsInputFileIDDigits = par("pfs_input_file_ID_digits").longValue();
	string pfsIpnutFilePrefix = par("pfs_input_file_prefix").stdstringValue();
	string pfsInputFilePostfix = par("pfs_input_file_postfix").stdstringValue();


	if(pfsSignature.compare(PVFS2_SIGNATURE) == 0) {
		pfsMetadataServerStrategy = new PVFS2MetadataServerStrategy(myID);
		((PVFS2MetadataServerStrategy *)(pfsMetadataServerStrategy))->
				readPFSFileInformationFromFile(pfsInputFileNum, pfsInputFileIDDigits, pfsIpnutFilePrefix,
				pfsInputFilePostfix);
	} else {
		pfsMetadataServerStrategy = NULL;
		string errorMessage = "Unknown pfsClient signature." + pfsSignature;
		PrintError::print("MetadataServer::initialize", errorMessage);
	}
}

void MetadataServer::handleMessage(cMessage * cmsg)
{
	switch(cmsg->getKind()){
	case LAYOUT_REQ:
		handleMetadataPacket((qPacket*)cmsg);
		break;
	case JOB_REQ: // For data request.
	case LAYOUT_RESP: // For metadata reply.
		sendSafe(cmsg);
		break;
		sendSafe(cmsg);
	case JOB_RESP: // Receive the data packet response.
	case JOB_FIN:
	case JOB_FIN_LAST:
		handleDataPacketResponse((gPacket *)cmsg);
	default:
		PrintError::print("MetadataServer::handleMessage", "Unknown message type.", cmsg->getKind());
	}
}

void MetadataServer::handleMetadataPacket(qPacket * packet) {
	vector<cPacket *> * packetlist = pfsMetadataServerStrategy->handleMetadataPacket(packet);
	processPacketList(packetlist);
}

void MetadataServer::handleDataPacketResponse(gPacket * packet) {
	vector<cPacket *> * packetlist = pfsMetadataServerStrategy->handleDataPacketReply(packet);
	processPacketList(packetlist);
}

void MetadataServer::processPacketList(vector<cPacket *> * list) {
	vector<cPacket *>::iterator it;
	double dataPacketRiseTime = SIMTIME_DBL(simTime());
	double metadataPacketRiseTime = SIMTIME_DBL(simTime());
	for (it = list->begin(); it != list->end(); it ++) {
#ifdef MS_DEBUG
	cout << "MetadataServer-" << myID << "::processPacketList packetID=" << ((bPacket *)(*it))->getID() << " ";
#endif
		switch ((*it)->getKind()) {
		case JOB_REQ: // Scheduled to send.
#ifdef MS_DEBUG
			cout << "JOB_REQ" << endl;
#endif
			if (dataPacketProcessTime != 0) {
				dataPacketRiseTime += dataPacketProcessTime;
				((gPacket *)(*it))->setRisetime(dataPacketRiseTime);
				scheduleAt((simtime_t)dataPacketRiseTime, *it);
			}
			else {
				((gPacket *)(*it))->setRisetime(dataPacketRiseTime);
				sendSafe((gPacket *)(*it));
			}
			break;

		case LAYOUT_RESP: // Scheduled to send.
#ifdef MS_DEBUG
	cout << "LAYOUT_RESP" << endl;
#endif
			if (metadataPacketProcessTime != 0) {
				metadataPacketRiseTime += metadataPacketProcessTime;
				((qPacket *)(*it))->setRisetime(metadataPacketRiseTime);
				scheduleAt((simtime_t)metadataPacketRiseTime, *it);
			}
			else {
				((qPacket *)(*it))->setRisetime(metadataPacketRiseTime);
				sendSafe((qPacket *)(*it));
			}
			break;

		default:
			PrintError::print("MetadataServer::processPacketList", "Unknown message type: ", (*it)->getKind());
		}
	}
	delete list;
}

void MetadataServer::sendSafe(cMessage * cmsg){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(cmsg, cch->getTransmissionFinishTime() - simTime(), "g$o");
	else
		send(cmsg, "g$o");
}

void MetadataServer::finish() {
	delete pfsMetadataServerStrategy;
}

