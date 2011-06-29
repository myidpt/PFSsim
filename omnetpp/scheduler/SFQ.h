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

// Weight of applications / clients
// Weight bigger-> higher priority
#define SET_WEIGHT do {\
	for(int wi = 0; wi < 1; wi ++){\
		weight[wi] = 1000;\
	}\
	for(int wi = 1; wi < 2; wi ++){\
		weight[wi] = 2000;\
	}\
} while(0);

#include "IQueue.h"
using namespace std;

class SFQ : public IQueue{
protected:
	struct Job{
		gPacket * gpkt;
		double stag; // The start tags for all applications
		double ftag; // The finish tags for all applications
	};
	list<Job*> waitQ[MAX_APP];
	list<Job*> osQ;
	double maxftags[MAX_APP]; // store the finish tags for each job.
	double vtime; // virtual time

public:
	SFQ(int);
	gPacket * dispatchNext();
	void pushWaitQ(gPacket *);
	void pushOsQ(Job *);
	gPacket * popOsQ(long id);
	gPacket * queryJob(long id);
};

#endif /* SFQ_H_ */
