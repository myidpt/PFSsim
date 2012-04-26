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

#ifndef IDSD_H_
#define IDSD_H_
#include "General.h"

class IDSD {
protected:
	int myID;
	int degree;
	int obj_size;
	long subreq_size;
	IQueue * reqQ;
public:
	IDSD(int, int, int, long);
	virtual void newReq(gPacket *) = 0;
	virtual gPacket * dispatchNext() = 0;
	virtual gPacket * finishedReq(gPacket *) = 0;
	virtual ~IDSD();
};

#endif /* IDSD_H_ */
