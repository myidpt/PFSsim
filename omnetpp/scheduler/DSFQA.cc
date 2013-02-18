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

#include "DSFQA.h"

DSFQA::DSFQA(int id, int deg, int totalapp, const char * alg_param) : DSFQ(id, deg, totalapp, alg_param) {
}

void DSFQA::receiveSPacket(sPacket * spkt){
	DSFQ::receiveSPacket_InsertBack(spkt);
}

void DSFQA::pushWaitQ(bPacket * pkt){
	DSFQ::pushWaitQ(pkt);
	pktToPropagate->setLengths(pkt->getApp(), pktToPropagate->getLengths(pkt->getApp()) + pkt->getSize());
	pktToPropagate->setSrc(myID); // Set activate
}

DSFQA::~DSFQA() {
}
