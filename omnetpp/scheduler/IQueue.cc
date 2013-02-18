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

#include "scheduler/IQueue.h"

IQueue::IQueue(int id, int deg){
	myID = id;
	degree = deg;
	appNum = 0;
	srand((unsigned)time(0));
}

void IQueue::pushWaitQ(bPacket * pkt) {
    PrintError::print("IQueue", 0, "pushWaitQ(bPacket *) is not defined.");
}

void IQueue::pushWaitQ(bPacket * packet, double time) {
    PrintError::print("IQueue", 0, "pushWaitQ(bPacket *, double) is not defined.");
}

bPacket * IQueue::dispatchNext() {
    PrintError::print("SFQRC", 0, "dispatchNext() is not defined.");
    return NULL;
}

bPacket * IQueue::dispatchNext(double time) {
    PrintError::print("IQueue", 0, "dispatchNext(double) is not defined.");
    return NULL;
}

bPacket * IQueue::popOsQ(long id) {
    PrintError::print("IQueue", 0, "popOsQ(long) is not defined.");
    return NULL;
}

bPacket * IQueue::popOsQ(long id, double time) {
    PrintError::print("IQueue", 0, "popOsQ(long, double) is not defined.");
    return NULL;
}

bPacket * IQueue::popOsQ(long id, long subid) {
    PrintError::print("IQueue", 0, "popOsQ(long, long) is not defined.");
    return NULL;
}

bPacket * IQueue::popOsQ(long id, long subid, double time) {
    PrintError::print("IQueue", 0, "popOsQ(long, long, double) is not defined.");
    return NULL;
}

bool IQueue::isEmpty() {
    PrintError::print("IQueue", 0, "isEmpty() is not defined.");
    return true;
}

sPacket * IQueue::propagateSPacket() {
    PrintError::print("IQueue", 0, "propagateSPacket() is not defined.");
    return NULL;
}

void IQueue::receiveSPacket(sPacket * pkt) {
    PrintError::print("IQueue", 0, "receiveSPacket(sPacket *) is not defined.");
}

IQueue::~IQueue(){
}
