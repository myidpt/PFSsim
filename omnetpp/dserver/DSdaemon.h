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

#ifndef DSDAEMON_H_
#define DSDAEMON_H_
#include "packet/GPacket_m.h"
#include "../General.h"
#include "../scheduler/FIFO.h"

class DSdaemon : public cSimpleModule{
protected:
	FIFO * queue;
public:
	DSdaemon();
	void initialize();
	void handleMessage(cMessage * cmsg);
	void handleNewJob(gPacket *);
	void handleDataReq(gPacket *);
	void handleDataResp(gPacket *);
	void dispatchJobs();
	void sendToLFS(gPacket *);
	void sendToEth(gPacket *);
	virtual ~DSdaemon();
};

#endif /* DSDAEMON_H_ */
