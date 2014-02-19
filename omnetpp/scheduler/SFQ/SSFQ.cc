// Yonggang Liu

#include "SSFQ.h"

SSFQ::SSFQ(int id, int deg, int totalc, const char * param):SFQ(id, deg, totalc, param) {
	maxSubReqSize = MAX_SUBREQ_SIZE; // The maximum size of sub-requests.
	subReqNum = new map<long, int>();
	reqList = new map<long, bPacket *>();
}

// Split the requests large in size to smaller sub-requests.
void SSFQ::pushWaitQ(bPacket * pkt){
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
			subreq->setID(pkt->getID() * RID_OFFSET + i++);
			subreq->setLowoffset(offset % LOWOFFSET_RANGE);
			subreq->setHighoffset(offset / LOWOFFSET_RANGE);
			subreq->setRead(gpkt->getRead());
			subreq->setFileId(gpkt->getFileId());
			subreq->setSubID(gpkt->getSubID());
			subreq->setApp(gpkt->getApp());
			subreq->setClientID(gpkt->getClientID());
			subreq->setDsID(gpkt->getDsID());

			if(size > MAX_SUBREQ_SIZE){
				subreq->setSize(MAX_SUBREQ_SIZE);
				size -= MAX_SUBREQ_SIZE;
				offset += MAX_SUBREQ_SIZE;
			}else{
				subreq->setSize(size);
				size = 0;
			}
			SFQ::pushWaitQ(subreq);
		}
	}else{
		subReqNum->insert(std::pair<long, int>(pkt->getID(), -10)); // If the request is small in size, there's no sub-request.
		pkt->setID(pkt->getID() * RID_OFFSET);
		SFQ::pushWaitQ(pkt);
	}
}

bPacket * SSFQ::popOsQ(long subreqid){
	bPacket * ret = NULL;
	bPacket * subreq = SFQ::popOsQ(subreqid);
	if(subreq == NULL)
		return NULL;
	long reqid = subreqid / RID_OFFSET;
	map<long, int>::iterator it = subReqNum->find(reqid);
	if(it == subReqNum->end()){ // Not found
		PrintError::print("SSFQ - popOsQ", "Can not find the request with this ID.", reqid);
		return NULL;
	}

	int num = it->second;
	subReqNum->erase(it);

	if(num < 0){ // Itself is the request, no sub-request.
		map<long, bPacket *>::iterator it2 = reqList->find(reqid);
		reqList->erase(it2);
		ret = it2->second;
		ret->setID(ret->getID() / RID_OFFSET);
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

SSFQ::~SSFQ(){
}
