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
	int totalClients;
	struct Job{
		bPacket * pkt;
		double stag; // The start tags for all applications
		double ftag; // The finish tags for all applications
	};
	list<Job*> waitQ[MAX_APP];
	list<Job*> osQ;
	double maxftags[MAX_APP]; // store the finish tags for each job.
	double vtime; // virtual time

	static FILE * schfp;

public:
	SFQ(int, int, int);
	virtual bPacket * dispatchNext();
	virtual void pushWaitQ(bPacket *);
	virtual void pushOsQ(Job *);
	virtual bPacket * popOsQ(long id);
	virtual bPacket * queryJob(long id);
	virtual sPacket * propagateSPacket();
	virtual void receiveSPacket(sPacket *);

	void printNJ(Job *);
	void printDP(Job *);
	void printFIN(Job *);
};

#endif /* SFQ_H_ */
