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

FILE * SFQ::schfp = NULL;

SFQ::SFQ(int id, int deg, int tlapps, const char * alg_param) : IQueue(id, deg) {
	vtime = 0;
	appNum = tlapps;
	for(int i = 0; i < MAX_APP; i ++)
		maxftags[i] = 0;

	osQ = new list<Job*>();
	startpos = 0;
	setWeights(alg_param);
#ifdef SCH_PRINT
	if(id == 0){
		char schfn[30] = "schedprint/queue";
		schfp = fopen(schfn, "w+");
		if(schfp == NULL){
			PrintError::print("SFQ", "Can't open schedprint/queue file.");
		}
	}
#endif
}

void SFQ::pushWaitQ(bPacket * pkt){
#ifdef SCH_DEBUG
	cout << "pushWaitQ: " << pkt->getID() << endl;
#endif
	struct Job * job = new Job();
	int app = pkt->getApp();
	job->pkt = pkt;
	if(waitQ[app] == NULL){ // Not initialized yet
		waitQ[app] = new list<Job*>();
	}

	if(waitQ[app]->empty()) // No back-logged jobs.
		job->stag = (maxftags[app] > vtime) ? maxftags[app] : vtime;
	else // queue not empty
		job->stag = waitQ[app]->back()->ftag;
	job->ftag = job->stag + job->pkt->getSize() / weight[app];

	waitQ[app]->push_back(job);
#ifdef SCH_PRINT
	printNJ(job);
#endif
}

bPacket * SFQ::dispatchNext(){
	if((signed int)(osQ->size()) >= IQueue::degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;

	// Get the job with the lowest start tag.
	double mintag = MAX_TAG_VALUE;
	int minindex = -1;

	// Random, for fairness.
	bool firstround = true;
	int i = startpos;
	while(1){
		if(i >= appNum)
			i = 0;
		if(i == startpos){
			if(firstround == true)
				firstround = false;
			else
				break;
		}
		if(waitQ[i] == NULL || waitQ[i]->empty()){ // No job in queue
			i++;
			continue;
		}
		// Only compare to the front (The earliest job).
		if(minindex == -1 || mintag > waitQ[i]->front()->stag){ // Select the smallest stag.
			minindex = i;
			mintag = waitQ[i]->front()->stag;
		}
		i ++;
	}
	if(minindex == -1){ // No job to schedule.
		return NULL;
	}

	Job * job = waitQ[minindex]->front();
	waitQ[minindex]->pop_front();
	pushOsQ(job);
	startpos = minindex; // Update the start position.
	startpos ++;
	if(startpos >= appNum)
		startpos = 0;

	// Update vtime
	vtime = job->stag;

	// Set the tags for the next one in wait queue.
	if(waitQ[minindex]->empty()){
		maxftags[minindex] = job->ftag;
	}else{ // Set the tags of the next one in the wait queue.
		// This is necessary because in DSFQ-D and DSFQ-F the stags and ftags of requests at front of queue may be changed.
		Job * next = waitQ[minindex]->front();
		next->ftag += job->ftag - next->stag;
		next->stag = job->ftag; // vtime should be equal to job->stag, which is smaller than job->ftag.
//		next->ftag = next->stag + next->pkt->getSize() / weight[minindex];
	}
	bPacket * pkt = job->pkt;

#ifdef SCH_PRINT
	printDP(job);
#endif

#ifdef SCH_DEBUG
	cout << "dispatchNext: " << pkt->getID() << endl;
#endif

	return pkt;
}

void SFQ::pushOsQ(Job * job){
	/*
	if(osQ->empty()){
		osQ->push_back(job);
		return;
	}
	typename list<Job*>::iterator iter;
	for(iter = osQ->begin(); iter != osQ->end(); iter ++){
		if( (*iter)->stag > job->stag ){
			osQ->insert(iter, job);
			return;
		}
	}
	*/
	osQ->push_back(job);
}

bPacket * SFQ::popOsQ(long id){
	bPacket * pkt = NULL;
	if(osQ->empty()){
		PrintError::print("SFQ", "Didn't find the job in OsQ, OsQ is empty.", id);
		return NULL;
	}

	list<Job*>::iterator iter;
	for(iter = osQ->begin(); iter != osQ->end(); iter ++){
		if( (*iter)->pkt->getID() == id ){
			pkt = (*iter)->pkt;
			osQ->erase(iter);
#ifdef SCH_PRINT
	printFIN(*iter);
#endif
			delete *iter;
			break;
		}
	}
	if(pkt == NULL){
		PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
		return NULL;
	}

	return pkt;
}

bPacket * SFQ::popOsQ(long id, long subid){
	bPacket * pkt = NULL;
	if(osQ->empty()){
		PrintError::print("SFQ", "Didn't find the job in OsQ, OsQ is empty.", id);
		return NULL;
	}

	list<Job*>::iterator iter;
	for(iter = osQ->begin(); iter != osQ->end(); iter ++){
		if((*iter)->pkt->getID() == id && (*iter)->pkt->getSubID() == subid){
			pkt = (*iter)->pkt;
			osQ->erase(iter);
#ifdef SCH_PRINT
	printFIN(*iter);
#endif
			delete *iter;
			break;
		}
	}
	if(pkt == NULL){
		PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
		return NULL;
	}

	return pkt;
}

bPacket * SFQ::queryJob(long id){
	bPacket * pkt = NULL;
	if(osQ->empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ->begin(); iter != osQ->end(); iter ++){
		if( (*iter)->pkt->getID() == id ){
			pkt = (*iter)->pkt;
			break;
		}
	}
	if(pkt == NULL){
		PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
		return NULL;
	}
	return pkt;
}


bPacket * SFQ::queryJob(long id, long subid){
	bPacket * pkt = NULL;
	if(osQ->empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ->begin(); iter != osQ->end(); iter ++){
		if( (*iter)->pkt->getID() == id && (*iter)->pkt->getSubID() == subid){
			pkt = (*iter)->pkt;
			break;
		}
	}
	if(pkt == NULL){
		PrintError::print("SFQ", "Didn't find the job in OsQ!", id);
		return NULL;
	}
	return pkt;
}

sPacket * SFQ::propagateSPacket(){
	PrintError::print("SFQ", "Calling propagateSPacket method in SFQ is prohibited.");
	return NULL;
}

void SFQ::receiveSPacket(sPacket *){
	PrintError::print("SFQ", "Calling receiveSPacket method in SFQ is prohibited.");
}


void SFQ::printNJ(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tNJ #%d {%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}

void SFQ::printDP(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tDP #%d [%.2lf %.2lf] {%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), job->stag, job->ftag, waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}
void SFQ::printFIN(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tFIN #%d {%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[job->pkt->getApp()]->size(), osQ->size());
#endif
}

SFQ::~SFQ(){
	if(osQ != NULL){
		osQ->clear();
		delete osQ;
	}
	for(int i = 0; i < MAX_APP; i ++){
		if(waitQ[i] != NULL){
			waitQ[i]->clear();
			delete waitQ[i];
		}
	}
}

void SFQ::setWeights(const char * alg_param){
    const char * tmp = alg_param;
    int i;
    double w;
    cout << " weights:";
    while(1){
        tmp = strchr(tmp, '[');
        if(tmp == NULL)
            break;
        sscanf(tmp, "[%d %lf]", &i, &w);
        weight[i] = w;
        cout << " [" << i << "] $" << weight[i];
        tmp ++;
    }
    cout << "." << endl;
}
