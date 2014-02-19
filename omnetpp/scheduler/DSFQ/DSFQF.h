// Yonggang Liu

#ifndef DSFQF_H_
#define DSFQF_H_

#include "DSFQ.h"

class DSFQF : public DSFQ{
public:
	DSFQF(int id, int deg, int totalc, const char * alg_param);
	virtual void receiveSPacket(sPacket * spkt);
	virtual bPacket * popOsQ(long id);
	virtual ~DSFQF();
};

#endif /* DSFQF_H_ */
