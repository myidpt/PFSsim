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

#ifndef PFSMETADATASERVERSTRATEGY_H_
#define PFSMETADATASERVERSTRATEGY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omnetpp.h>

#include "General.h"
#include "scheduler/IQueue.h"
#include "scheduler/FIFO.h"

class PFSMetadataServerStrategy {
public:
	virtual string getSignature() = 0;
	virtual vector<cPacket *> * handleMetadataPacket(qPacket * packet) = 0;
	virtual vector<cPacket *> * handleDataPacketReply(gPacket * packet) = 0;
	virtual ~PFSMetadataServerStrategy();
};

#endif /* PFSMETADATASERVERSTRATEGY_H_ */
