// Yonggang Liu

#ifndef DSFQATB_H_
#define DSFQATB_H_

#include "DSFQA.h"

// Time bounded Distributed SFQ algorithm.
class DSFQATB : public DSFQA{
protected:
    double time_threshold;
    void processParameter(const char * alg_param);
public:
	DSFQATB(int id, int deg, int totalapp, const char * alg_param);
	double notify();
	virtual ~DSFQATB();
};

#endif /* DSFQATB_H_ */
