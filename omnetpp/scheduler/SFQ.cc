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

SFQ::SFQ(int deg):IQueue(deg) {
	vtime = 0;
	for(int i = 0; i < MAX_APP; i ++)
		maxftags[i] = 0;
	SET_WEIGHT
}

void SFQ::pushWaitQ(gPacket * gpkt){
	struct Job * job = (struct Job *)malloc(sizeof(struct Job));
	int app = gpkt->getApp();
	job->gpkt = gpkt;

	// stag(k) = max{ftag(k-1), vtime}
	job->stag = maxftags[app];
	if(vtime > job->stag)
		job->stag = vtime;

	job->ftag = job->stag + job->gpkt->getSize() / weight[app];

	maxftags[app] = job -> ftag;

	waitQ[app].push_back(job);
}

gPacket * SFQ::dispatchNext(){
	if((signed int)(osQ.size()) >= degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;

	// Get the job with the lowest start tag.
	double mintag = 1000000000;
	int minindex = -1;

	// Random, for fairness.
	int start = (float)rand() / RAND_MAX * C_TOTAL;
	bool firstround = true;
	for(int i = start; ; i ++){
		if(i == C_TOTAL)
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
	gPacket * gpkt = job->gpkt;

	waitQ[minindex].pop_front();
	pushOsQ(job);
	// Update vtime
	vtime = job->stag;
	return gpkt;
}

void SFQ::pushOsQ(Job * job){
	if(osQ.empty()){
		osQ.push_back(job);
		return;
	}
	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->stag > job->stag ){
			osQ.insert(iter, job);
			return;
		}
	}
	osQ.push_back(job);
}

gPacket * SFQ::popOsQ(long id){
	gPacket * gpkt = NULL;
	if(osQ.empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->gpkt->getId() == id ){
			gpkt = ((Job *)(*iter))->gpkt;
			osQ.erase(iter);
			free(*iter);
			break;
		}
	}
	if(gpkt == NULL){
		fprintf(stderr, "ERROR: Didn't find the job %ld in OsQ!\n", id);
		fflush(stdout);
		return NULL;
	}

#ifdef DEBUG
//	fprintf(sfile, "\t\t%ld F @ %.2lf\n",
//			gpkt->getId(), vtime);
#endif

	return gpkt;
}


gPacket * SFQ::queryJob(long id){
	gPacket * gpkt = NULL;
	if(osQ.empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( ((Job *)(*iter))->gpkt->getId() == id ){
			gpkt = ((Job *)(*iter))->gpkt;
			break;
		}
	}
	if(gpkt == NULL){
		fprintf(stderr, "ERROR: Didn't find the job %ld in OsQ!\n", id);
		fflush(stdout);
		return NULL;
	}
	return gpkt;
}
