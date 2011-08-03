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

#include "Routing.h"

Define_Module(Routing);

void Routing::handleMessage(cMessage *msg)
{
	switch(msg->getKind()){
	case LAYOUT_REQ:
		send(msg, "msout");
		break;
	case LAYOUT_RESP:
		send(msg, "cout", ((qPacket *)msg)->getId() / CID_OFFSET_IN_PID);
		break;
	case JOB_REQ:
		send(msg, "schout", ((gPacket *)msg)->getDecision());
		break;
	case JOB_DISP:
		send(msg, "dsout", ((gPacket *)msg)->getDecision());
		break;
	case JOB_FIN:
		send(msg, "schout", ((gPacket *)msg)->getDecision());
		break;
	case JOB_RESP:
		send(msg, "cout", ((gPacket *)msg)->getId() / CID_OFFSET_IN_PID);
		break;
	}
}

