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

#ifndef DSFQF_H_
#define DSFQF_H_

#include "SFQ.h"

class DSFQF : public SFQ{
protected:
	bPacket * pktToPropagate;
public:
	DSFQF(int id, int deg, int totalc);
	void receiveSPacket(sPacket * spkt);
	bPacket * popOsQ(long id);
	sPacket * propagateSPacket();
	virtual ~DSFQF();
};

#endif /* DSFQF_H_ */
