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
    numDS = par("numDservers").longValue();
	numProxies = par("numProxies").longValue();
	readProxyRoutingFile(par("proxy_routing_path").stringValue());
}

void Routing::readProxyRoutingFile(const char * path) {
    FILE * fp = fopen(path, "r");
    if(fp == NULL){
        PrintError::print(
            "Routing-readProxyRoutingFile", string("can not open file") + path);
        return;
    }
    char line[800];
    while(fgets(line, 300, fp) != NULL){
        if(line[0] == '#') { // Comment.
            continue;
        }
        int server;
        int proxy;
        sscanf(line, "%d %d", &server, &proxy);
        if (server >= numDS) {
            PrintError::print(
                "Routing-readProxyRoutingFile", 0,
                "Server ID exceeds server number.", server);
        }
        else if (proxy >= numProxies){
            PrintError::print(
                "Routing-readProxyRoutingFile", 0,
                "Proxy ID exceeds proxy number.", proxy);
        }
        StoPRoutingTable[server] = proxy;
    }
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
	case PFS_R_REQ: // client -> proxy, proxy -> ds
		if(strstr(msg->getArrivalGate()->getFullName(), "cin") != NULL)
			send(msg, "schout", StoPRoutingTable[((gPacket *)msg)->getDsID()]);
		else if(strstr(msg->getArrivalGate()->getFullName(), "schin") != NULL)
			send(msg, "dsout", ((gPacket *)msg)->getDsID());
		break;
	case PFS_W_RESP:
	case PFS_R_DATA:
	case PFS_R_DATA_LAST:
	case PFS_W_FIN: // ds -> proxy, proxy -> client
		if(strstr(msg->getArrivalGate()->getFullName(), "dsin") != NULL)
			send(msg, "schout", StoPRoutingTable[((gPacket *)msg)->getDsID()]);
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
		for(int i = 0; i < numProxies; i ++){
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

