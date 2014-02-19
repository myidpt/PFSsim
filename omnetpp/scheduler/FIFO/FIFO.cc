// Author: Yonggang.

#include "FIFO.h"

FIFO::FIFO(int id, int deg):IQueue(id, deg) {
	osQ = new list<bPacket *>();
	waitQ = new list<bPacket *>();
}

bPacket * FIFO::seeWaitQNext() {
    if(waitQ->empty()){// No jobs in queue.
        return NULL;
    }
    return (bPacket *)waitQ->front();
}

bPacket * FIFO::seeOsQNext() {
    if(osQ->empty()){// No jobs in queue.
        return NULL;
    }
    return (bPacket *)osQ->front();
}

// Push one job to the waitQ.
void FIFO::pushWaitQ(bPacket * job){
	waitQ->push_back(job);
}

// Push one job to the OsQ.
void FIFO::pushOsQ(bPacket * job) {
    osQ->push_back(job);
}

// Pop one job from the front of the waitQ, and push it to the osQ. Return the job.
bPacket * FIFO::dispatchNext(){
	if(waitQ->empty()){// No more jobs in queue.
		return NULL;
	}
	if((signed int)(osQ->size()) >= degree && degree >= 0 ){
	    // If outstanding queue is bigger than the degree, stop dispatching more jobs.
	    // A negative degree means no degree control.
		return NULL;
	}
	bPacket * ret = (bPacket *)waitQ->front();
	waitQ->pop_front();
	osQ->push_back(ret); // Push into osQ.
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

int FIFO::getWaitQSize() {
    return waitQ->size();
}

int FIFO::getOsQSize() {
    return osQ->size();
}

bool FIFO::waitQisEmpty() {
    return waitQ->empty();
}
bool FIFO::osQisEmpty() {
    return osQ->empty();
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
