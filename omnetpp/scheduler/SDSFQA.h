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

#ifndef SDSFQA_H_
#define SDSFQA_H_

#include "DSFQA.h"
using namespace std;

class SDSFQA : public DSFQA{
protected:
	map<long, int> * subReqNum; // Keeps the record of the number of sub-requests each request still needs to receive.
	map<long, bPacket *> * reqList; // Stores the requests, which can be referred by the ID.

	long maxSubReqSize;

public:
	SDSFQA(int, int, int);
	virtual void pushWaitQ(bPacket *);
	virtual bPacket * popOsQ(long id);
};

#endif /* SDSFQA_H_ */
