// Yonggang Liu

#include "DSFQD.h"

DSFQD::DSFQD(int id, int deg, int totalapp, const char * alg_param) : DSFQ(id, deg, totalapp, alg_param) {
}

void DSFQD::receiveSPacket(sPacket * spkt){
	DSFQ::receiveSPacket_InsertBack(spkt);
}

bPacket * DSFQD::dispatchNext(){
	bPacket * pkt = DSFQ::dispatchNext();
	if(pkt == NULL) // No more to dispatch.
		return NULL;
	pktToPropagate->setLengths(pkt->getApp(), pktToPropagate->getLengths(pkt->getApp()) + pkt->getSize());
	pktToPropagate->setSrc(myID); // Set activate
//	cout << "DSFQD - dispatchNext" << endl;
	return pkt;
}

DSFQD::~DSFQD() {
}
