// Yonggang Liu

#ifndef DSFQD_H_
#define DSFQD_H_

#include "DSFQ.h"

class DSFQD : public DSFQ{
public:
	DSFQD(int id, int deg, int totalc, const char * alg_param);
	virtual void receiveSPacket(sPacket * spkt);
	virtual bPacket * dispatchNext();
	virtual ~DSFQD();
};

#endif /* DSFQF_A_ */
