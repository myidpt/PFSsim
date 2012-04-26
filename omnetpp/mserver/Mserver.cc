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

#include "mserver/Mserver.h"

Define_Module(Mserver);

void Mserver::initialize(){ // Change here to change data layout.
	numDservers = (int)par("numDservers").longValue();
	ms_proc_time = par("ms_proc_time").doubleValue();

	if(readLayoutFile(par("layout_input_path").stringValue())){
		printf("Metadata Server: Data layout file read successful.\n");
		fflush(stdout);
	}else{
		PrintError::print("Mserver", "Data layout file read error.");
		deleteModule();
	}
}

void Mserver::handleMessage(cMessage *cmsg){
	switch(cmsg->getKind()){
	case LAYOUT_REQ:
		handleLayoutReq((qPacket*)cmsg);
		break;
	case LAYOUT_RESP:
		sendSafe(cmsg);
		break;
	default:
		PrintError::print("Mserver", "Unknown message type.", cmsg->getKind());
	}
}

void Mserver::handleLayoutReq(qPacket *qpkt){
	int fileid = qpkt->getFileId();
	int i;
	for(i = 0; i < numFiles; i ++){
		if(layoutlist[i]->getFileID() == fileid){
			layoutlist[i]->setqPacket(qpkt);
			break;
		}
	}
	if(i == numFiles){
		PrintError::print("Mserver", "Layout of this file ID is not defined in the layout file.", fileid);
		deleteModule();
	}
	qpkt->setByteLength(METADATA_LENGTH); // Assume query reply length is METADATA_LENGTH.
	qpkt->setKind(LAYOUT_RESP);
	if(ms_proc_time != 0)
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + ms_proc_time), qpkt);
	else
		sendSafe(qpkt);
}

void Mserver::sendSafe(cMessage * cmsg){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(cmsg, cch->getTransmissionFinishTime() - simTime(), "g$o");
	else
		send(cmsg, "g$o");
}

int Mserver::readLayoutFile(const char * fname){
	FILE * fp = NULL;
	if((fp = fopen(fname, "r")) == NULL){
		PrintError::print("Mserver - parseLayoutFile", string("Can not find layout document")+fname);
		return -1;
	}
	// Each line of the document is in this format:
	// FILE_ID [server_ID data_size] [server_ID data_szie] ...
	char line[500];
	int file_index = 0;
	while(fgets(line,500,fp) != NULL){
		Layout * layout = NULL;
		int i = 0;
		int index = 0;
		long val = -1;
		bool end = false; // True if it meets the end of line.
		bool sep = false; // True if a separation mark is found.
		enum position_type{file_id, server_id, server_stripe_size};
		int position = file_id;
		while(!end){
			if(line[i] == '\n' || line[i] == '\0'){
				end = true;
				sep = true;
			}
			if(line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '[' || line[i] == ']'){
				sep = true;
			}
			if(line[i] >= '0' && line[i] <= '9'){
				if(val  == -1)
					val = 0;
				val = val * 10 + (line[i] - '0');
				sep = false;
			}
			if(line[i] == 'k' || line[i] == 'K'){
				sep = true;
				if(val == -1) // Useless label if there's no numeral part right before k.
					continue;
				val = val * 1024;
			}
			if(line[i] == 'm' || line[i] == 'M'){
				sep = true;
				if(val == -1) // Useless label if there's no numeral part right before m.
					continue;
				val = val * 1024 * 1024;
			}

			if(val != -1 && sep == true){
				if(position == file_id){
					if(val >= MAX_FILE){
						PrintError::print("Mserver - parseLayoutFile", "File ID in layout file out of range.", (int)val);
						fclose(fp);
						return -1;
					}
					layout = new Layout((int)val);
					position = server_id;
				}else if(position == server_id){
					if(val >= numDservers){
						PrintError::print("Mserver - parseLayoutFile", "Data Server ID in layout file out of range.", (int)val);
						fclose(fp);
						return -1;
					}
					layout->setServerID(index, (int)val);
					position = server_stripe_size;
				}else{
					layout->setServerStripeSize(index, val);
					position = server_id;
					index ++;
				}
				val = -1;
			}
			i ++;
		}
		if(layout != NULL){
			layout->setServerNum(index);
			layout->calculateWindowSize();
			layoutlist[file_index] = layout;
			file_index ++;
		}
	}
	numFiles = file_index;
	if(numFiles == 0){
		PrintError::print("Mserver - parseLayoutFile", "No application data layout is given.");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 1;
}

void Mserver::finish(){
#ifdef MS_DEBUG
	cout << "Mserver - finish." << endl;
	fflush(stdout);
#endif
	for(int i = 0; i < numFiles; i ++)
		delete layoutlist[i];
}

Mserver::~Mserver(){
}
