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

void Routing::initialize(){
	numSchedulers = par("numDservers").longValue();
}

void Routing::handleMessage(cMessage *msg)
{
	switch(msg->getKind()){
	case LAYOUT_REQ:
		send(msg, "msout");
		break;
	case LAYOUT_RESP:
		send(msg, "cout", ((qPacket *)msg)->getClientID());
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
		send(msg, "cout", ((gPacket *)msg)->getClientID());
		break;
	case PROP_SCH:
		handleSPacketPropagation((sPacket *)msg);
		break;
	}
}

void Routing::handleSPacketPropagation(sPacket * spkt){
	cGate * arrgate = spkt->getArrivalGate();
	for(int i = 0; i < numSchedulers; i ++){
		if(arrgate != gate("interschin", i)){
			sPacket * tmp = new sPacket("PROP_SCH", PROP_SCH);
			tmp->setApp(spkt->getApp());
			tmp->setLength(spkt->getLength());
			send(tmp, "interschout", i);
		}
	}
	delete spkt;
}

