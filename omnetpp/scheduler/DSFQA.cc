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

#include "DSFQA.h"

DSFQA::DSFQA(int id, int deg, int totalc) : SFQ(id, deg, totalc) {
	pktToPropagate = NULL;
}

void DSFQA::receiveSPacket(sPacket * spkt){
	int app = spkt->getApp();
	if(waitQ[app].empty()){
		maxftags[app] += spkt->getLength() / weight[app]; // maxftags only available when the queue is empty
#ifdef SCH_PRINT
	fprintf(schfp, "\t[%d]\tUD (%d) [%.2lf].\n", myID, app, maxftags[app]);
#endif
	}else{
		waitQ[app].back()->ftag += spkt->getLength() / weight[app];
#ifdef SCH_PRINT
	fprintf(schfp, "\t[%d]\tUD %ld (%d) [%.2lf %.2lf].\n",
			myID, waitQ[app].back()->pkt->getID(), waitQ[app].back()->pkt->getApp(),
		waitQ[app].back()->stag, waitQ[app].back()->ftag);
#endif
	}

}

void DSFQA::pushWaitQ(bPacket * pkt){
	struct Job * job = (struct Job *)malloc(sizeof(struct Job));
	int app = pkt->getApp();
	job->pkt = pkt;
	if(waitQ[app].empty()) // No back-logged jobs.
		job->stag = (maxftags[app] > vtime) ? maxftags[app] : vtime;
	else // queue not empty
		job->stag = waitQ[app].back()->ftag;

	job->ftag = job->stag + job->pkt->getSize() / IQueue::weight[app];

	waitQ[app].push_back(job);

	pktToPropagate = pkt;
#ifdef SCH_PRINT
	printNJ(job);
#endif
}

bPacket * DSFQA::dispatchNext(){
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
		if(minindex == -1 || mintag > waitQ[i].front()->stag){ // Select the smallest stag.
			minindex = i;
			mintag = waitQ[i].front()->stag;
		}
	}
	if(minindex == -1){ // No job to schedule.
		return NULL;
	}

	Job * job = waitQ[minindex].front();
	waitQ[minindex].pop_front();
	pushOsQ(job);

	// Update vtime
	vtime = job->stag;

	if(waitQ[minindex].empty()){
		maxftags[minindex] = job->ftag;
	}

	bPacket * pkt = job->pkt;

#ifdef SCH_PRINT
	printDP(job);
#endif

	return pkt;
}

sPacket * DSFQA::propagateSPacket(){
	if(pktToPropagate == NULL){
		fprintf(stderr, "[ERROR] DSFQA: propagateSPacket could not find a job to propagate.\n");
		return NULL;
	}
	sPacket * spkt = new sPacket("PROP_SCH");
	spkt->setKind(PROP_SCH);
	spkt->setApp(pktToPropagate->getApp());
	spkt->setLength(pktToPropagate->getSize());
	pktToPropagate = NULL;
	return spkt;
}

DSFQA::~DSFQA() {
}


