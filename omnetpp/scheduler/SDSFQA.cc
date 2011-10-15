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

#include "scheduler/SDSFQA.h"

SDSFQA::SDSFQA(int id, int deg, int totalc):DSFQA(id, deg, totalc) {
	maxSubReqSize = MAX_SUBREQ_SIZE; // The maximum size of sub-requests.
	subReqNum = new map<long, int>();
	reqList = new map<long, bPacket *>();
}

// Split the requests large in size to smaller sub-requests.
void SDSFQA::pushWaitQ(bPacket * pkt){
	reqList->insert(std::pair<long, bPacket *>(pkt->getID(), pkt)); // Stores the requests.
	if(pkt->getSize() > MAX_SUBREQ_SIZE){
		gPacket * gpkt = (gPacket *)pkt;
		int size = pkt->getSize();
		long long offset = gpkt->getLowoffset() + gpkt->getHighoffset() * LOWOFFSET_RANGE;
		int num = size / MAX_SUBREQ_SIZE;
		if(size % MAX_SUBREQ_SIZE != 0) // Consider the remainder.
			num ++;
		subReqNum->insert(std::pair<long, int>(pkt->getID(), num));
		int i = 0;
		while(size > 0){
			gPacket * subreq = new gPacket("SUBREQ", pkt->getKind());
			subreq->setID(pkt->getID() * PID_OFFSET_IN_SUBPID + i++);
			subreq->setLowoffset(offset % LOWOFFSET_RANGE);
			subreq->setHighoffset(offset / LOWOFFSET_RANGE);
			subreq->setRead(gpkt->getRead());
			subreq->setFileId(gpkt->getFileId());
			subreq->setApp(gpkt->getApp());
			subreq->setClientID(gpkt->getClientID());
			subreq->setDecision(gpkt->getDecision());

			if(size > MAX_SUBREQ_SIZE){
				subreq->setSize(MAX_SUBREQ_SIZE);
				size -= MAX_SUBREQ_SIZE;
				offset += MAX_SUBREQ_SIZE;
			}else{
				subreq->setSize(size);
				size = 0;
			}
			DSFQA::pushWaitQ(subreq);
		}
	}else{
		subReqNum->insert(std::pair<long, int>(pkt->getID(), -10)); // If the request is small in size, there's no sub-request.
		pkt->setID(pkt->getID() * PID_OFFSET_IN_SUBPID);
		DSFQA::pushWaitQ(pkt);
	}
}

bPacket * SDSFQA::popOsQ(long subreqid){
	bPacket * ret = NULL;
	bPacket * subreq = DSFQA::popOsQ(subreqid);
	if(subreq == NULL)
		return NULL;
	long reqid = subreqid / PID_OFFSET_IN_SUBPID;
	map<long, int>::iterator it = subReqNum->find(reqid);
	if(it == subReqNum->end()){ // Not found
		fprintf(stderr, "[ERROR] SDSFQA: popOsQ - can not find the request with ID = %ld.\n", reqid);
		return NULL;
	}

	int num = it->second;
	subReqNum->erase(it);

	if(num < 0){ // Itself is the request, no sub-request.
		map<long, bPacket *>::iterator it2 = reqList->find(reqid);
		reqList->erase(it2);
		ret = it2->second;
		ret->setID(ret->getID() / PID_OFFSET_IN_SUBPID);
	}else{
		delete subreq; // Delete the sub-request.
		num --;
		if(num == 0){ // All of the sub-requests are received.
			map<long, bPacket *>::iterator it2 = reqList->find(reqid);
			reqList->erase(it2);
			ret = it2->second;
		}else{
			subReqNum->insert(std::pair<long, int>(reqid, num));
		}
	}

	return ret;
}
