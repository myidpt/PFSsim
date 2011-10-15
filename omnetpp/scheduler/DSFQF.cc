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

#include "DSFQF.h"

DSFQF::DSFQF(int id, int deg, int totalc) : SFQ(id, deg, totalc) {
	pktToPropagate = NULL;
}

void DSFQF::receiveSPacket(sPacket * spkt){
	int app = spkt->getApp();
	if(waitQ[app].empty()){
		maxftags[app] += spkt->getLength() / weight[app];
#ifdef SCH_PRINT
	fprintf(schfp, "\t[%d]\tUD (%d) [%.2lf].\n", myID, app, maxftags[app]);
#endif
	}else{
		waitQ[app].front()->stag += spkt->getLength() / weight[app];
		waitQ[app].front()->ftag += spkt->getLength() / weight[app];
#ifdef SCH_PRINT
	fprintf(schfp, "\t[%d]\tUD %ld (%d) [%.2lf %.2lf].\n",
			myID, waitQ[app].front()->pkt->getID(), waitQ[app].front()->pkt->getApp(),
		waitQ[app].front()->stag, waitQ[app].front()->ftag);
#endif
	}
}

bPacket * DSFQF::popOsQ(long id){
	bPacket * bpkt = SFQ::popOsQ(id);
	pktToPropagate = bpkt;
	return bpkt;
}

sPacket * DSFQF::propagateSPacket(){
	if(pktToPropagate == NULL){
		fprintf(stderr, "[ERROR] DSFQF: propagateSPacket could not find a job to propagate.\n");
		return NULL;
	}
	sPacket * spkt = new sPacket("PROP_SCH");
	spkt->setKind(PROP_SCH);
	spkt->setApp(pktToPropagate->getApp());
	spkt->setLength(pktToPropagate->getSize());
	pktToPropagate = NULL;
	return spkt;
}

DSFQF::~DSFQF() {
}


