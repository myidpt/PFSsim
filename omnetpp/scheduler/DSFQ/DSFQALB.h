// Yonggang Liu

#ifndef DSFQALB_H_
#define DSFQALB_H_

#include "DSFQA.h"

class DSFQALB : public DSFQA{
protected:
	double workload_threshold;
    void processParameter(const char * alg_param);
public:
	DSFQALB(int id, int deg, int totalapp, const char * alg_param);
	void pushWaitQ(bPacket *);
	virtual ~DSFQALB();
};

#endif /* DSFQALB_H_ */
