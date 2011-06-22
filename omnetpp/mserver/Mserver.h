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
	Layout * layoutlist[MAX_APP];
	int appNum;
	virtual void parseLayoutDoc(const char * fname);
  protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void handleLayoutReq(qPacket *qpkt);
	virtual void sendSafe(cMessage *);
	virtual ~Mserver();
};

#endif
