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

#include "DSFQ.h"

DSFQ::DSFQ(int id, int deg, int totalapp, const char * alg_param) : SFQ(id, deg, totalapp, alg_param) {
	pktToPropagate = new sPacket("PROP_SCH");
	pktToPropagate->setKind(PROP_SCH);
	pktToPropagate->setSrc(-1); // Set inactive.
	pktToPropagate->setAppNum(totalapp);
}

void DSFQ::receiveSPacket_InsertBack(sPacket * spkt){
	for(int i = 0; i < spkt->getAppNum(); i ++){
		if(spkt->getLengths(i) == 0)
			continue;
		if(waitQ[i] == NULL){ // Not initialized yet
			waitQ[i] = new list<Job*>();
		}
		if(waitQ[i]->empty()){
			maxftags[i] += spkt->getLengths(i) / weight[i]; // maxftags only available when the queue is empty
#ifdef SCH_PRINT
			fprintf(schfp, "\t[%d]\tUD (%d) [%.2lf].\n", myID, i, maxftags[i]);
#endif
		}else{
			waitQ[i]->back()->ftag += spkt->getLengths(i) / weight[i];
#ifdef SCH_PRINT
			fprintf(schfp, "\t[%d]\tUD %ld (%d) [%.2lf %.2lf].\n",
				myID, waitQ[i]->back()->pkt->getID(), waitQ[i]->back()->pkt->getApp(),
				waitQ[i]->back()->stag, waitQ[i]->back()->ftag);
#endif
		}
	}
}

void DSFQ::receiveSPacket_InsertFront(sPacket * spkt){
	for(int i = 0; i < spkt->getAppNum(); i ++){
		if(spkt->getLengths(i) == 0)
			continue;
		if(waitQ[i] == NULL){ // Not initialized yet
			waitQ[i] = new list<Job*>();
		}
		if(waitQ[i]->empty()){
			maxftags[i] += spkt->getLengths(i) / weight[i];
#ifdef SCH_PRINT
			fprintf(schfp, "\t[%d]\tUD (%d) [%.2lf].\n", myID, i, maxftags[i]);
#endif
		}else{
			waitQ[i]->front()->stag += spkt->getLengths(i) / weight[i];
			waitQ[i]->front()->ftag += spkt->getLengths(i) / weight[i];
#ifdef SCH_PRINT
			fprintf(schfp, "\t[%d]\tUD %ld (%d) [%.2lf %.2lf].\n",
				myID, waitQ[i]->front()->pkt->getID(), waitQ[i]->front()->pkt->getApp(),
				waitQ[i]->front()->stag, waitQ[i]->front()->ftag);
#endif
		}
	}
}

sPacket * DSFQ::propagateSPacket(){
	if(pktToPropagate->getSrc() < 0){ // Inactive?
		return NULL;
	}
	sPacket * spkt = new sPacket(*pktToPropagate);
	// Reset the length.
	spkt->setDst(-1);
	for(int i = 0; i < pktToPropagate->getAppNum(); i ++){
		pktToPropagate->setLengths(i, 0);
	}
	pktToPropagate->setSrc(-1);
//	cout << "DSFQ propagateSPacket: " << spkt->getLengths(0) << " " << spkt->getLengths(1) << endl;
	return spkt;
}

DSFQ::~DSFQ() {
	if(pktToPropagate != NULL)
		delete pktToPropagate;
}
