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
	case PFS_W_DATA: // client -> server
	case PFS_W_DATA_LAST:
	case PFS_W_REQ:
	case PFS_R_REQ:
		if(strstr(msg->getArrivalGate()->getFullName(), "cin") != NULL)
			send(msg, "schout", ((gPacket *)msg)->getDsID());
		else if(strstr(msg->getArrivalGate()->getFullName(), "schin") != NULL)
			send(msg, "dsout", ((gPacket *)msg)->getDsID());
		break;
	case PFS_W_RESP:
	case PFS_R_DATA:
	case PFS_R_DATA_LAST:
	case PFS_W_FIN:
		if(strstr(msg->getArrivalGate()->getFullName(), "dsin") != NULL)
			send(msg, "schout", ((gPacket *)msg)->getDsID());
		else if(strstr(msg->getArrivalGate()->getFullName(), "schin") != NULL)
			send(msg, "cout", ((gPacket *)msg)->getClientID());
		break;
	case PROP_SCH:
		handleSPacketPropagation((sPacket *)msg);
		break;
	}
}

void Routing::handleSPacketPropagation(sPacket * spkt){
	int srcId = spkt->getSrc();
	if(spkt->getDst() == -1){
		for(int i = 0; i < numSchedulers; i ++){
			if(srcId != i){
				sPacket * tmp = new sPacket(*spkt);
				tmp->setDst(i);
				send(tmp, "interschout", i);
			}
		}
		delete spkt;
	}else{
		send(spkt, "interschout", spkt->getDst());
	}

}

