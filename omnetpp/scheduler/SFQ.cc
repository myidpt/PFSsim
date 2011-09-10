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

#include "scheduler/SFQ.h"

SFQ::SFQ(int deg, int totalc):IQueue(deg) {
	vtime = 0;
	totalClients = totalc;
	for(int i = 0; i < MAX_APP; i ++)
		maxftags[i] = 0;
	for(int wi = 0; wi < 1; wi ++){
		IQueue::weight[wi] = 1000;
	}
}

void SFQ::pushWaitQ(bPacket * pkt){
	struct Job * job = (struct Job *)malloc(sizeof(struct Job));
	int app = pkt->getApp();
	job->pkt = pkt;

	// stag(k) = max{ftag(k-1), vtime}
	job->stag = maxftags[app];
	if(vtime > job->stag)
		job->stag = vtime;

	job->ftag = job->stag + job->pkt->getSize() / IQueue::weight[app];

	maxftags[app] = job->ftag;

	waitQ[app].push_back(job);
}

bPacket * SFQ::dispatchNext(){
	if((signed int)(osQ.size()) >= IQueue::degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;

	// Get the job with the lowest start tag.
	double mintag = 1000000000;
	int minindex = -1;

	// Random, for fairness.
	int start = (float)rand() / RAND_MAX * totalClients;
	bool firstround = true;
	for(int i = start; ; i ++){
		if(i == totalClients)
			i = 0;
		if(i == start){
			if(firstround == true)
				firstround = false;
			else
				break;
		}
		if(waitQ[i].empty()) // No job in queue
			continue;
		// Only compare to the front (The earliest job).
		if(mintag > ((Job *)(waitQ[i].front()))->stag){ // Select the smallest stag.
			minindex = i;
			mintag = ((Job *)(waitQ[i].front()))->stag;
		}
	}
	if(minindex == -1){ // No job to schedule.
		return NULL;
	}

	Job * job = waitQ[minindex].front();
	bPacket * pkt = job->pkt;

	waitQ[minindex].pop_front();
	pushOsQ(job);
	// Update vtime
	vtime = job->stag;
	return pkt;
}

void SFQ::pushOsQ(Job * job){
	if(osQ.empty()){
		osQ.push_back(job);
		return;
	}
	typename list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->stag > job->stag ){
			osQ.insert(iter, job);
			return;
		}
	}
	osQ.push_back(job);
}

bPacket * SFQ::popOsQ(long id){
	bPacket * pkt = NULL;
	if(osQ.empty())
		return NULL;

	typename list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->pkt->getID() == id ){
			pkt = ((Job *)(*iter))->pkt;
			osQ.erase(iter);
			free(*iter);
			break;
		}
	}
	if(pkt == NULL){
		fprintf(stderr, "ERROR: Didn't find the job %ld in OsQ!\n", id);
		fflush(stdout);
		return NULL;
	}
	return pkt;
}

bPacket * SFQ::queryJob(long id){
	bPacket * pkt = NULL;
	if(osQ.empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->pkt->getID() == id ){
			pkt = ((Job *)(*iter))->pkt;
			break;
		}
	}
	if(pkt == NULL){
		fprintf(stderr, "ERROR: Didn't find the job %ld in OsQ!\n", id);
		fflush(stdout);
		return NULL;
	}
	return pkt;
}
