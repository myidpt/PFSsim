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

#include "Queue.h"

Define_Module(Queue);

void Queue::initialize(){
	queue = new cQueue("queue");
	endTransmissionEvent = new cMessage("endTxEvent");
}

void Queue::startTransmitting(cMessage *msg){
	send(msg, "line$o");
    // The schedule an event for the time when last bit will leave the gate.
    simtime_t endTransmission = gate("line$o")->getTransmissionChannel()->getTransmissionFinishTime();
    scheduleAt(endTransmission, endTransmissionEvent);
}

void Queue::handleMessage(cMessage *msg){
	if (msg==endTransmissionEvent){
		// Transmission finished, we can start next one.
		if (!queue->empty())
		{
			msg = (cMessage *) queue->pop();
			startTransmitting(msg);
		}
	}
	else if(msg->arrivedOn("line$i")){ // from the outside
		send(msg,"out");
	}
	else{// from the Routing module
		if (endTransmissionEvent->isScheduled()){
			// We are currently busy, so just queue up the packet.
			queue->insert(msg);
		}
		else{
			// We are idle, so we can start transmitting right away.
			startTransmitting(msg);
		}
	}
}

void Queue::finish(){
}

Queue::~Queue(){
	if(queue != NULL)
		delete queue;
	if(endTransmissionEvent != NULL)
		cancelAndDelete(endTransmissionEvent);
}
