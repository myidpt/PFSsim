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
	parseLayoutDoc("config/layout");
	if(appNum == 0)
		fprintf(stderr, "ERROR Mserver: No application data layout is given.\n");
	printf("Mserver created.\n");
	fflush(stdout);
}

void Mserver::handleMessage(cMessage *cmsg){
	switch(cmsg->getKind()){
	case LAYOUT_REQ:
		handleLayoutReq((qPacket*)cmsg);
		break;
	default:
		fprintf(stderr, "ERROR Mserver: Unknown message type %d.\n", cmsg->getKind());
	}
}

void Mserver::handleLayoutReq(qPacket *qpkt){
	int app = qpkt->getApp();
	for(int i = 0; i < appNum; i ++){
		if(layoutlist[i]->getAppID() == app){
			layoutlist[i]->setqPacket(qpkt);
		}
	}
	qpkt->setKind(LAYOUT_RESP);
	qpkt->setByteLength(300); // Assume schedule reply length is 300.
	sendSafe(qpkt);
}

void Mserver::sendSafe(cMessage * cmsg){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy())
		sendDelayed(cmsg, cch->getTransmissionFinishTime() - simTime(), "g$o");
	else
		send(cmsg, "g$o");
}

void Mserver::parseLayoutDoc(const char * fname){
	FILE * fp = NULL;
	if((fp = fopen(fname, "r")) == NULL){
		fprintf(stderr, "ERROR Mserver: Can not find layout document %s.", fname);
		return;
	}
	// Each line of the document is in this format:
	// APP_ID [server_ID data_size] [server_ID data_szie] ...
	char line[500];
	int app_index = 0;
	while(fgets(line,500,fp) != NULL){
		Layout * layout = NULL;
		int i = 0;
		int index = 0;
		long val = -1;
		bool end = false; // True if it meets the end of line.
		bool sep = false; // True if a separation mark is found.
		enum position_type{app_id, server_id, server_share};
		int position = app_id;
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
				if(position == app_id){
					printf("\nAPP: %ld   ",val);
					layout = new Layout((int)val);
					position = server_id;
				}else if(position == server_id){
					printf("[%ld ",val);
					layout->setServerID(index, (int)val);
					position = server_share;
				}else{
					printf("%ld]  ",val);
					layout->setServerShare(index, val);
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
			layoutlist[app_index] = layout;
			app_index ++;
		}
	}
	printf("\n");
	fflush(stdout);
	appNum = app_index;
	fclose(fp);
}
Mserver::~Mserver(){
	for(int i = 0; i < appNum; i ++){
		delete layoutlist[i];
	}
}
