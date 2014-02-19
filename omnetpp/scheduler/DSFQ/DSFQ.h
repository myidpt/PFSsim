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

#ifndef DSFQ_H_
#define DSFQ_H_

#include "SFQ.h"

class DSFQ : public SFQ{
protected:
	sPacket * pktToPropagate;
    DSFQ(int, int, int, const char *);
public:
	void receiveSPacket_InsertBack(sPacket *);
	void receiveSPacket_InsertFront(sPacket *);
	virtual sPacket * propagateSPacket();
	virtual ~DSFQ();
};

#endif /* DSFQ_H_ */
