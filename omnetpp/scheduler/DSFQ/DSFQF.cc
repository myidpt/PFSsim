// Yonggang Liu

#include "DSFQF.h"

DSFQF::DSFQF(int id, int deg, int totalc, const char * alg_param) : DSFQ(id, deg, totalc, alg_param) {
}

void DSFQF::receiveSPacket(sPacket * spkt){
	DSFQ::receiveSPacket_InsertFront(spkt);
}

bPacket * DSFQF::popOsQ(long id){
	bPacket * pkt = DSFQ::popOsQ(id);
	if(pkt == NULL)
		return NULL;
	pktToPropagate->setLengths(pkt->getApp(), pktToPropagate->getLengths(pkt->getApp()) + pkt->getSize());
	pktToPropagate->setSrc(myID); // Set activate
	return pkt;
}

DSFQF::~DSFQF() {
}
