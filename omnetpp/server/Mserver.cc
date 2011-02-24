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

#include "server/Mserver.h"

Define_Module(Mserver);

void Mserver::initialize() // Change here to change data layout.
{
	dsNum[0] = 4;
	dsNum[1] = 3;
	for(int i = 0; i < dsNum[0]; i ++)
		dsList[0][i] = i;
	for(int i = 0; i < dsNum[1]; i ++)
		dsList[1][i] = 3-i;
}

void Mserver::handleMessage(cMessage *msg)
{
	qPacket * qpkt = (qPacket *)msg;
	setLayout(qpkt);
	qpkt->setKind(LAYOUT_RESP);
	qpkt->setByteLength(300); // Assume schedule reply length is 100.
	sendSafe(qpkt);
}

void Mserver::setLayout(qPacket * qpkt){
	int app = qpkt->getApp();
	qpkt->setDsNum(dsNum[app]);
	for(int i = 0; i < dsNum[app]; i ++)
		qpkt->setDsList(i,dsList[app][i]);
}

void Mserver::sendSafe(cMessage * cmsg){
	cGate *ggate = gate("g$o");
	if(ggate->isBusy())
		sendDelayed(cmsg, ggate->getTransmissionFinishTime() - simTime(), "g$o");
	else
		send(cmsg, "g$o");
}
