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

#ifndef SFQ_H_
#define SFQ_H_

#include "IQueue.h"
using namespace std;

class SFQ : public IQueue{
protected:
    static const int MAX_TAG_VALUE = 10000000;
    double weight[MAX_APP]; // Weight for every application
	struct Job{
		bPacket * pkt;
		double stag; // The start tags for all applications
		double ftag; // The finish tags for all applications
	};
	list<Job*> * waitQ[MAX_APP];
	list<Job*> * osQ;
	double maxftags[MAX_APP]; // store the finish tags for each job.
	double vtime; // virtual time
	int startpos; // The application first considered to be dispatched in the algorithm.

	static FILE * schfp;

    virtual void setWeights(const char *);

    void printNJ(Job *);
    void printDP(Job *);
    void printFIN(Job *);
public:
	SFQ(int, int, int, const char *);
	virtual void pushWaitQ(bPacket *);
	virtual bPacket * dispatchNext();
	virtual void pushOsQ(Job *);
	virtual bPacket * popOsQ(long id);
    virtual bPacket * popOsQ(long id, long subid);
	virtual bPacket * queryJob(long id);
	virtual bPacket * queryJob(long id, long subid);
	virtual sPacket * propagateSPacket();
	virtual void receiveSPacket(sPacket *);
	virtual ~SFQ();
};

#endif /* SFQ_H_ */
