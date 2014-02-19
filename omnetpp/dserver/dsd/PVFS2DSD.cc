// Yonggang Liu 2014.

#include "PVFS2DSD.h"

using namespace std;

PVFS2DSD::PVFS2DSD(int id, int deg, int subreqsize)
: IDSD(id, deg, subreqsize){
    infoMap = new map<int, info_t *>();
	reqQ = SchedulerFactory::createScheduler(SchedulerFactory::FIFO_ALG, id, deg);
	subreqQ = SchedulerFactory::createScheduler(SchedulerFactory::FIFO_ALG, id, 1);
}

void PVFS2DSD::newReq(gPacket * gpkt){
    if(gpkt == NULL) {
        return;
    }
    // Push the request to reqQ->osQ.
    reqQ->pushWaitQ(gpkt);
    reqQ->dispatchNext();

    info_t * newinfo = new info_t();
    infoMap->insert(pair<int, info_t *>(gpkt->getID(), newinfo));
    // 1. Check if the packet is a write.
    // If so, put COMP information to info.decision. Then dispatch.
    if(!gpkt->getRead()){
        if(gpkt->getKind() == SELF_PFS_W_DATA_LAST) {
            newinfo->decision = 1; // This is the last one.
        }
        else {
            newinfo->decision = 0;
        }
        subreqQ->pushWaitQ(gpkt);
        return;
    }

    // It is a READ if you reach here.
    // Temporarily store the information into info[].
    // Dispatch the smaller requests one by one (next one is dispatched
    // upon the finish of the previous one).
    int small_req;
    // If the request size is smaller than obj size,
    // you may need to issue smaller requests.
    if(gpkt->getSize() < subreq_size) {
        small_req = 1;
    }
    else {
        small_req = 0;
    }

    newinfo->small_req = small_req;
    newinfo->loff = gpkt->getLowoffset();
    newinfo->size = gpkt->getSize();
    newinfo->lub = newinfo->loff + newinfo->size;

    newinfo->hoff = gpkt->getHighoffset(); // high offset is not changed
    newinfo->read = gpkt->getRead();
    newinfo->subID = gpkt->getSubID();
    newinfo->app = gpkt->getApp();
    newinfo->fid = gpkt->getFileId();
    newinfo->decision = gpkt->getDsID();
    newinfo->cid = gpkt->getClientID();

    // It is a READ.
    // 3. Push the first sub-request to subreqQ->waitQ.
    if(newinfo->lub - newinfo->loff <= subreq_size) {
        // The last data sub-request.
        // We push the original request to the subreqQ.
        gpkt->setKind(LFILE_REQ);
        gpkt->setLowoffset(newinfo->loff);
        // This is necessary because you may have changed it due to
        // object size limit.
        gpkt->setSize(newinfo->size);
        subreqQ->pushWaitQ(gpkt);
    }else{ // Not the last sub-request, create a new one.
        gPacket * subreq = new gPacket("LFILE_REQ", LFILE_REQ);
        subreq->setID(gpkt->getID() + SUBREQ_OFFSET);
        subreq->setRisetime(gpkt->getRisetime());
        subreq->setSubmittime(gpkt->getSubmittime());
        subreq->setArrivaltime(gpkt->getArrivaltime());
        subreq->setLowoffset(newinfo->loff);
        subreq->setHighoffset(newinfo->hoff);
        subreq->setSize(subreq_size); // The maximum size allowed.
        subreq->setRead(newinfo->read);
        subreq->setSubID(newinfo->subID);
        subreq->setApp(newinfo->app);
        subreq->setFileId(newinfo->fid);
        subreq->setDsID(newinfo->decision);
        subreq->setClientID(newinfo->cid);
        subreqQ->pushWaitQ(subreq);
    }
}

gPacket * PVFS2DSD::dispatchNext(){
	gPacket * gpkt = (gPacket *)subreqQ->dispatchNext();
	if(gpkt == NULL)
		return NULL;
#ifdef PVFS2DSD_DEBUG
	cout << "[" << SIMTIME_DBL(simTime()) << "] PVFS2DSD#" << myID
	     << ": dispatchNext. ID=" << gpkt->getID() << endl;
#endif
	return gpkt;
}

gPacket * PVFS2DSD::finishedReq(gPacket * finsubreq){
#ifdef PVFS2DSD_DEBUG
    cout << "[" << SIMTIME_DBL(simTime()) << "] PVFS2DSD#" << myID
         << ": finishedReq. ID=" << finsubreq->getID() << endl;
#endif
	// 1. Pop it from subreqQ.
	long id = finsubreq->getID();
	long origid;
	if(finsubreq->getRead())
		origid = id / RID_OFFSET * RID_OFFSET;
	else
		origid = id;
	if(subreqQ->popOsQ(id) == NULL){
		PrintError::print("PVFS2DSD - finishedReq", "Can not pop from OsQ.", id);
	}

	// 2. Find the information for this original request from infoMap.
	map<int, info_t *>::iterator it = infoMap->find(origid);

	if (it == infoMap->end()) {
        PrintError::print("PVFS2DSD", "Can't find the id in infoMap.", origid);
	}
    info_t * theinfo = it->second;

	// 3. See if this is a write packet.
	// If so, check the original packet to decide if it will be returned.
	if(!finsubreq->getRead()){
		if(reqQ->popOsQ(origid) == NULL){
			PrintError::print("PVFS2DSD", "Can't find the id in osQ.", origid);
			return NULL;
		}
	    infoMap->erase(it);
#ifdef PVFS2DSD_DEBUG
			cout << "Write done. Removed #" << origid << endl;
#endif
		if(theinfo->decision == 1){ // This indicates a LAST packet.
			finsubreq->setName("PFS_W_FIN");
			finsubreq->setKind(PFS_W_FIN);
			return finsubreq;
		}else{
			delete finsubreq;
			return NULL;
		}
	}

	// If you come here, it is a read.
	// 4. If this is a read packet. Generate the next one and put it into subreqQ
	// (if the next one exists) and send this one back.

	finsubreq->setName("PFS_R_DATA");
	finsubreq->setKind(PFS_R_DATA); // Set the common kind.

	if(theinfo->lub <= finsubreq->getLowoffset() + finsubreq->getSize()) {
	    // The finished sub-request is the last one sent.
		finsubreq->setName("PFS_R_DATA_LAST");
        // Set the special kind if it is the last sub-request.
		finsubreq->setKind(PFS_R_DATA_LAST);
		if(reqQ->popOsQ(origid) == NULL){
		    // Pop the original PVFS request.
			PrintError::print("PVFS2DSD", "Can't find the id in osQ.", origid);
			return NULL;
		}
        infoMap->erase(it); // Remove the info record.
		return finsubreq;
#ifdef PVFS2DSD_DEBUG
			cout << "PVFS2DSD #" << myID
			     << ": Read all done. Removed info #" << origid << endl;
#endif
	}
	else { // Not the last one. Push the next sub-request to subreqQ.
		gPacket * nextsubreq;
		long loff = finsubreq->getLowoffset() + finsubreq->getSize();
		if(theinfo->lub <= loff + subreq_size){
		    // Going to deal with the last one,
		    // use the original request as the last subreq.
		    // Note: only the last response has the ID equal to the request.
			nextsubreq = (gPacket *)reqQ->queryJob(origid);
			nextsubreq->setKind(LFILE_REQ);
			if(nextsubreq == NULL){
				PrintError::print("PVFS2DSD",
				        "Cannot find the original request from reqQ.", id);
			}
            nextsubreq->setLowoffset(loff);
			nextsubreq->setSize(theinfo->lub - loff);
		}else{
			nextsubreq = new gPacket("LFILE_REQ", LFILE_REQ);
			if((finsubreq->getID() + SUBREQ_OFFSET) % RID_OFFSET == 0) {
			    // We limit the ID space for this module.
				nextsubreq->setID(origid + SUBREQ_OFFSET);
			}
			else {
				nextsubreq->setID(finsubreq->getID() + SUBREQ_OFFSET);
			}
			nextsubreq->setSize(subreq_size);
	        nextsubreq->setLowoffset(loff);
			nextsubreq->setHighoffset(theinfo->hoff);
			nextsubreq->setRead(theinfo->read);
			nextsubreq->setSubID(theinfo->subID);
			nextsubreq->setApp(theinfo->app);
			nextsubreq->setFileId(theinfo->fid);
			nextsubreq->setDsID(theinfo->decision);
			nextsubreq->setClientID(theinfo->cid);
		}
		subreqQ->pushWaitQ(nextsubreq);
		return finsubreq;
	}
}

PVFS2DSD::~PVFS2DSD() {
	cout << "PVFS2DSD finish." << endl;
	fflush(stdout);
	if (infoMap != NULL) {
	    delete infoMap;
	}
	if(reqQ != NULL) {
		delete reqQ;
	}
	if(subreqQ != NULL) {
		delete subreqQ;
    }
}
