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

// The SSFQ algorithm realizes packet splitting/assembling in addition to the original SFQ algorithm.
#ifndef SSFQ_H_
#define SSFQ_H_

#include "scheduler/SFQ.h"
using namespace std;

class SSFQ : public SFQ{
protected:
	map<long, int> * subReqNum; // Keeps the record of the number of sub-requests each request still needs to receive.
	map<long, bPacket *> * reqList; // Stores the requests, which can be referred by the ID.

	long maxSubReqSize;

public:
	SSFQ(int, int, int, const char *);
	virtual void pushWaitQ(bPacket *);
	virtual bPacket * popOsQ(long id);
	virtual ~SSFQ();
};

#endif /* SSFQ_H_ */
