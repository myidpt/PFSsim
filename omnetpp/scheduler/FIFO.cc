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

FIFO::FIFO(int deg):IQueue(deg) {
}

void FIFO::pushWaitQ(gPacket * gpkt){
	waitQ.push_back(gpkt);
//	fprintf(sfile, "%lf\t%ld A\n", SIMTIME_DBL(simTime()), gpkt->getId());
}

gPacket * FIFO::dispatchNext(){
	if(waitQ.empty())// No more jobs in queue.
		return NULL;
	if((signed int)(osQ.size()) >= degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;
	gPacket * ret = (gPacket *)waitQ.front();
	waitQ.pop_front();
	osQ.push_back(ret); // Push into osQ.
//	fprintf(sfile, "%lf\t%ld D\n", SIMTIME_DBL(simTime()), ret->getId());
	return ret;
}

gPacket * FIFO::popOsQ(long id){
	gPacket * ret = NULL;
	list<gPacket*>::iterator i;
	for(i=osQ.begin(); i != osQ.end(); i++){
		if((*i)->getId() == id){
			ret = *i;
			osQ.erase(i);
			break;
		}
	}
//	if(ret != NULL)
//		fprintf(sfile, "%lf\t%ld F\n", SIMTIME_DBL(simTime()), ret->getId());

	return ret;
}

sPacket * FIFO::overhear(sPacket * spkt, int i){
	return NULL;
}

gPacket * FIFO::queryJob(long id){
	gPacket * ret = NULL;
	list<gPacket*>::iterator i;
	for(i=osQ.begin(); i != osQ.end(); i++){
		if((*i)->getId() == id){
			ret = *i;
			break;
		}
	}
	return ret;
}
