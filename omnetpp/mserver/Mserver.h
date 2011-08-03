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

#ifndef __MSERVER_H__
#define __MSERVER_H__

#include <time.h>
#include <math.h>
#include <omnetpp.h>
#include "../General.h"
#include "../layout/Layout.h"

class Mserver : public cSimpleModule{
private:
	double ms_proc_time;
	Layout * layoutlist[MAX_APP];
	int numApps;
	int numDservers;
	virtual void parseLayoutDoc(const char * fname);
protected:
	void initialize();
	void handleMessage(cMessage *);
	virtual void handleLayoutReq(qPacket *);
	void handleqPacketReply(qPacket *);
	void sendSafe(cMessage *);
	virtual ~Mserver();
};

#endif
