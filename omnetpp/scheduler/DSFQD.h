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

#ifndef DSFQD_H_
#define DSFQD_H_

#include "SFQ.h"

class DSFQD : public SFQ{
protected:
	bPacket * pktToPropagate;
public:
	DSFQD(int id, int deg, int totalc);
	void receiveSPacket(sPacket * spkt);
	bPacket * dispatchNext();
	sPacket * propagateSPacket();
	virtual ~DSFQD();
};

#endif /* DSFQF_A_ */
