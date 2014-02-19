// Yonggang Liu

#include "DSFQALB.h"

// Alg_param format: [id weight] [id weight] [id weight] ... | Load_threshold
DSFQALB::DSFQALB(int id, int deg, int totalapp, const char * alg_param)
    : DSFQA(id, deg, totalapp, alg_param) {
    processParameter(alg_param);
}

void DSFQALB::pushWaitQ(bPacket * pkt) {
	DSFQ::pushWaitQ(pkt);
	pktToPropagate->setLengths(pkt->getApp(), pktToPropagate->getLengths(pkt->getApp()) + pkt->getSize());
	if(pktToPropagate->getLengths(pkt->getApp()) >= workload_threshold)
		pktToPropagate->setSrc(myID); // Set activate
}

void DSFQALB::processParameter(const char * alg_param) {
    const char * wtstr = strchr(alg_param, '|');
    if(wtstr == NULL){
        PrintError::print("DSFQALB", myID, "Can't find the workload threshold from the input parameter.");
        return;
    }
    wtstr ++;
    sscanf(wtstr, "%lf", &workload_threshold);
}

DSFQALB::~DSFQALB() {
}
