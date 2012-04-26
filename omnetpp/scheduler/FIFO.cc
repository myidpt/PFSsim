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
	osQ = new list<bPacket *>();
	waitQ = new list<bPacket *>();
}

// Push one job to the waitQ.
void FIFO::pushWaitQ(bPacket * job){
	waitQ->push_back(job);
//	fprintf(sfile, "%lf\t%ld A\n", SIMTIME_DBL(simTime()), gpkt->getID());
}

// Pop one job from the front of the waitQ, and push it to the osQ. Return the job.
bPacket * FIFO::dispatchNext(){
	if(waitQ->empty()){// No more jobs in queue.
//		cerr << "id = " << myID << ", empty" << endl;
//		fflush(stderr);
		return NULL;
	}
	if((signed int)(osQ->size()) >= degree){ // If outstanding queue is bigger than the degree, stop dispatching more jobs.
//		cerr << "id = " << myID << "wait degree = " << waitQ->size() << ", os degree = " << osQ->size() << ", degree = " << degree << endl;
//		fflush(stderr);
		return NULL;
	}
	bPacket * ret = (bPacket *)waitQ->front();
	waitQ->pop_front();
	osQ->push_back(ret); // Push into osQ.
//	fprintf(sfile, "%lf\t%ld D\n", SIMTIME_DBL(simTime()), ret->getID());
	return ret;
}

// Pop the element with ID == id from osQ, and return it.
bPacket * FIFO::popOsQ(long id){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ->begin(); i != osQ->end(); i++){
		if((*i)->getID() == id){
			ret = *i;
			osQ->erase(i);
			break;
		}
	}
//	if(ret != NULL)
//		fprintf(sfile, "%lf\t%ld F\n", SIMTIME_DBL(simTime()), ret->getID());
	return ret;
}

// Pop the element with ID == id && SubID == subid from osQ, and return it.
bPacket * FIFO::popOsQ(long id, long subid){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ->begin(); i != osQ->end(); i++){
		if(((*i)->getID() == id) && ((*i)->getSubID() == subid)){
			ret = *i;
			osQ->erase(i);
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
	list<bPacket *>::iterator i = osQ->begin();
	ret = *i;
	osQ->erase(i);
	return ret;
}

// Only return the job with ID == id, do not mutate the queue.
bPacket * FIFO::queryJob(long id){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ->begin(); i != osQ->end(); i++){
		if((*i)->getID() == id){
			ret = *i;
			break;
		}
	}
	return ret;
}

// Only return the job with ID == id && SubID == subid, do not mutate the queue.
bPacket * FIFO::queryJob(long id, long subid){
	bPacket * ret = NULL;
	list<bPacket *>::iterator i;
	for(i=osQ->begin(); i != osQ->end(); i++){
		if(((*i)->getID() == id) && ((*i)->getSubID() == subid)){
			ret = *i;
			break;
		}
	}
	return ret;
}

bool FIFO::isEmpty(){
	return waitQ->empty() && osQ->empty();
}

sPacket * FIFO::propagateSPacket(){
	PrintError::print("FIFO", "calling propagateSPacket method in FIFO is prohibited.");
	return NULL;
}

void FIFO::receiveSPacket(sPacket *){
	PrintError::print("FIFO", "calling receiveSPacket method in FIFO is prohibited.");
}

FIFO::~FIFO(){
	if(waitQ != NULL){
		waitQ->clear();
		delete waitQ;
	}
	if(osQ != NULL){
		osQ->clear();
		delete osQ;
	}
}
