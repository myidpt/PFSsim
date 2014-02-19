// Yonggang Liu

#include "IQueue.h"

IQueue::IQueue(int id, int deg){
	myID = id;
	degree = deg;
	appNum = 0;
	srand((unsigned)time(0));
}

bPacket * IQueue::popOsQ(long id) {
    PrintError::print("IQueue", 0, "popOsQ(long) is not defined.");
    return NULL;
}

bPacket * IQueue::popOsQ(long id, long subid) {
    PrintError::print("IQueue", 0, "popOsQ(long, long) is not defined.");
    return NULL;
}

bPacket * IQueue::queryJob(long id, long subid) {
    PrintError::print("IQueue", 0, "queryJob(long, long) is not defined.");
    return NULL;
}

bool IQueue::isEmpty() {
    PrintError::print("IQueue", 0, "isEmpty() is not defined.");
    return true;
}

double IQueue::notify() {
    PrintError::print("IQueue", 0, "notify() is not defined.");
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
