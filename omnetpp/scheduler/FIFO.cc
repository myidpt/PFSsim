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

#include "scheduler/FIFO.h"

FIFO::FIFO(int id, int deg):IQueue(id, deg) {
}

// Push one job to the waitQ.
void FIFO::pushWaitQ(bPacket * job){
	waitQ.push_back(job);
//	fprintf(sfile, "%lf\t%ld A\n", SIMTIME_DBL(simTime()), gpkt->getID());
}

// Pop one job from the front of the waitQ, and push it to the osQ. Return the job.
bPacket * FIFO::dispatchNext(){
	if(waitQ.empty())// No more jobs in queue.
		return NULL;
	if((signed int)(osQ.size()) >= IQueue::degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;
	bPacket * ret = (bPacket *)waitQ.front();
	waitQ.pop_front();
	osQ.push_back(ret); // Push into osQ.
//	fprintf(sfile, "%lf\t%ld D\n", SIMTIME_DBL(simTime()), ret->getID());
	return ret;
}

// Pop the element with ID == id from osQ, and return it.
bPacket * FIFO::popOsQ(long id){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ.begin(); i != osQ.end(); i++){
		if((*i)->getID() == id){
			ret = *i;
			osQ.erase(i);
			break;
		}
	}
//	if(ret != NULL)
//		fprintf(sfile, "%lf\t%ld F\n", SIMTIME_DBL(simTime()), ret->getID());
	return ret;
}

// Pop the front element from osQ, and return it.
bPacket * FIFO::popOsQ(){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i = osQ.begin();
	ret = *i;
	osQ.erase(i);
	return ret;
}

// Only return the job with ID == id, do not mutate the queue.
bPacket * FIFO::queryJob(long id){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ.begin(); i != osQ.end(); i++){
		if((*i)->getID() == id){
			ret = *i;
			break;
		}
	}
	return ret;
}

sPacket * FIFO::propagateSPacket(){
	fprintf(stderr, "[ERROR] FIFO: calling propagateSPacket method in FIFO is prohibited.\n");
	fflush(stderr);
	return NULL;
}

void FIFO::receiveSPacket(sPacket *){
	fprintf(stderr, "[ERROR] FIFO: calling receiveSPacket method in FIFO is prohibited.\n");
	fflush(stderr);
}
