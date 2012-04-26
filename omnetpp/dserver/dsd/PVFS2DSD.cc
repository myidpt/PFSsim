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

#include "PVFS2DSD.h"

PVFS2DSD::PVFS2DSD(int id, int deg, int objsize, int subreqsize) : IDSD(id, deg, objsize, subreqsize){
	reqQ = new FIFO(999, deg);
	subreqQ = new FIFO(12345, 1); // You can only access sequentially for each single request.
	for(int i = 0; i < MAX_DSD_OSREQS; i ++){ // Init
		info[i].id = 0;
	}
}

void PVFS2DSD::newReq(gPacket * gpkt){
	reqQ->pushWaitQ(gpkt); // Push the request to reqQ->osQ.
	dispatchPVFSReqs();
}


// 1. Check if the packet is a write. If so, put COMP information to info[].decision. Then dispatch.
// 2. Resize the request according to the object size. Temporarily store the information into info[].
// 3. Push the first sub-request to subreqQ->waitQ.
void PVFS2DSD::dispatchPVFSReqs(){
	while(1){
		gPacket * gpkt = (gPacket *)(reqQ->dispatchNext());
		if(gpkt == NULL)
			return;

		// 1. Check if the packet is a write. If so, put COMP information to info[].decision. Then dispatch.
		if(!gpkt->getRead()){
			int i;
			for(i = 0; i < MAX_DSD_OSREQS; i ++){
				if(info[i].id == 0){ // Unclaimed slot, record the original information.
					info[i].id = gpkt->getID();
#ifdef PVFS2DSD_DEBUG
					cout << "PVFS2DSD #" << myID << ": Put into info[].id=" << info[i].id << endl;
#endif
					if(gpkt->getKind() == SELF_JOB_DISP_LAST)
						info[i].decision = 1;
					else
						info[i].decision = 0;
					break;
				}
			}
			if(i == MAX_DSD_OSREQS){
				PrintError::print("PVFS2DSD - dispatchNext", "The info table is full.");
				return;
			}
			subreqQ->pushWaitQ(gpkt);
			return;
		}

		// It is a READ if you reach here.
		// 2. Resize the request according to the object size. Temporarily store the information into info[].
		// Calculate the offset of the "new" request based on the object size.
		// Dispatch the smaller requests one by one (next one is dispatched upon the finish of the previous one).
		int small_req; // If the request size is smaller than obj size, you may need to issue smaller requests.
		if(gpkt->getSize() < obj_size)
			small_req = 1;
		else
			small_req = 0;

//	cout << "small_req=" << small_req << endl;
		int i;
		for(i = 0; i < MAX_DSD_OSREQS; i ++){
			if(info[i].id == 0){ // Unclaimed slot, record the original information.
				info[i].id = gpkt->getID();
				info[i].small_req = small_req;
				info[i].orig_loff = gpkt->getLowoffset(); // Original low offset
				info[i].orig_size = gpkt->getSize();
				info[i].orig_ub = info[i].orig_loff + info[i].orig_size;
				// Here I set them for small_req, but for big reqs, change them soon:
				info[i].obj_loff = info[i].orig_loff;
				info[i].obj_size = info[i].orig_size;
				info[i].obj_ub = info[i].orig_ub;

				info[i].hoff = gpkt->getHighoffset(); // high offset is not changed
				info[i].read = gpkt->getRead();
				info[i].app = gpkt->getApp();
				info[i].fid = gpkt->getFileId();
				info[i].decision = gpkt->getDsID();
				info[i].cid = gpkt->getClientID();
#ifdef PVFS2DSD_DEBUG
				cout << "PVFS2DSD #" << myID << ": Put into info[].id=" << info[i].id << endl;
#endif
//			cout << info[i].id << " " << info[i].orig_loff << " " << info[i].orig_size << " " << info[i].orig_ub << " $ "
//					<< info[i].obj_loff << " " << info[i].obj_size << " " << info[i].obj_ub << endl;
				break;
			}
		}

		if(i == MAX_DSD_OSREQS){
			PrintError::print("PVFS2DSD - dispatchNext", "The info table is full.");
			return;
		}

		if(!small_req){
			long long off = (gpkt->getLowoffset() + gpkt->getHighoffset() * LOWOFFSET_RANGE) / obj_size * obj_size;
			int size = gpkt->getSize();
			long frontskew = obj_size - gpkt->getLowoffset() % obj_size; // the skew in front
			long endskew = (size - frontskew + obj_size) % obj_size; // the skew in end
			size = ((frontskew != 0 ? 1 : 0) + (endskew != 0 ? 1 : 0)) * obj_size + size - frontskew - endskew;

			info[i].obj_loff = off % LOWOFFSET_RANGE; // object based low offset
			info[i].obj_size = size;
			info[i].obj_ub = info[i].obj_loff + info[i].obj_size;
		}

		// 3. Push the first sub-request to subreqQ->waitQ.
	//	cout << "Get: " << gpkt->getID() << ": " << gpkt->getLowoffset() << " " << gpkt->getSize() << endl;
		if(info[i].obj_ub <= info[i].obj_loff + subreq_size || !gpkt->getRead()){ // Write or the last data object.
			// This is the last sub-request. We push the original request to the subreqQ.
			gpkt->setKind(LFILE_REQ);
			gpkt->setLowoffset(info[i].obj_loff); // This is necessary because you may have changed it due to object size limit.
			gpkt->setSize(info[i].obj_size);
			subreqQ->pushWaitQ(gpkt);
//			cout << "push last: " << gpkt->getID() << ": " << gpkt->getLowoffset() << " " << gpkt->getSize()
//				<< " ClientID=" << gpkt->getClientID() << endl;
		}else{
			gPacket * subreq = new gPacket("LFILE_REQ", LFILE_REQ);
			subreq->setID(gpkt->getID() + SUBREQ_OFFSET);
			subreq->setRisetime(gpkt->getRisetime());
			subreq->setSubmittime(gpkt->getSubmittime());
			subreq->setArrivaltime(gpkt->getArrivaltime());
			subreq->setLowoffset(info[i].obj_loff);
			subreq->setHighoffset(info[i].hoff);
			subreq->setSize(subreq_size); // The maximum size allowed.
			subreq->setRead(info[i].read);
			subreq->setApp(info[i].app);
			subreq->setFileId(info[i].fid);
			subreq->setDsID(info[i].decision);
			subreq->setClientID(info[i].cid);
			subreqQ->pushWaitQ(subreq);
//			cout << "push: " << subreq->getID() << ": " << subreq->getLowoffset() << " " << subreq->getSize()
//					<< " ClientID=" << subreq->getClientID() << endl;
		}
	}
}

gPacket * PVFS2DSD::dispatchNext(){
	dispatchPVFSReqs();
	gPacket * gpkt = (gPacket *)subreqQ->dispatchNext();
	if(gpkt == NULL)
		return NULL;
#ifdef PVFS2DSD_DEBUG
	cout << "PVFS2DSD #" << myID << ": dispatchNext. ID=" << gpkt->getID() << endl;
#endif
	return gpkt;
}

// When receive a finished sub-request:
// 1. Pop it from subreqQ.
// 2. Find the information for this original request from info[].
// 3. See if this is a write packet. If so, check the original packet to decide if it will be returned.
// 4. If this is a read packet. Generate the next one and put it into subreqQ (if the next one exists) and send it back.
gPacket * PVFS2DSD::finishedReq(gPacket * finsubreq){
	// 1. Pop it from subreqQ.
	long id = finsubreq->getID();
	long origid;
	if(finsubreq->getRead())
		origid = id / PID_OFFSET * PID_OFFSET;
	else
		origid = id;
	if(subreqQ->popOsQ(id) == NULL){
		PrintError::print("PVFS2DSD - finishedReq", "Can not pop from OsQ.", id);
	}

	// 2. Find the information for this original request from info[].
	int i;
	for(i = 0; i < MAX_DSD_OSREQS; i ++){
		if(info[i].id == origid)
			break;
	}
	if(i == MAX_DSD_OSREQS){
		PrintError::print("PVFS2DSD", "Can't find the id in info[].", origid);
		return NULL; // You did not find the sub-request record.
	}

	// 3. See if this is a write packet. If so, check the original packet to decide if it will be returned.
	if(!finsubreq->getRead()){
		if(reqQ->popOsQ(origid) == NULL){
			PrintError::print("PVFS2DSD", "Can't find the id in osQ.", origid);
			return NULL;
		}
		info[i].id = 0; // Nullify it first.
#ifdef PVFS2DSD_DEBUG
			cout << "Write done. Nullified info[].id=" << origid << endl;
#endif
		if(info[i].decision == 1){ // This indicates a COMP packet.
			finsubreq->setName("JOB_FIN_LAST");
			finsubreq->setKind(JOB_FIN_LAST);
			return finsubreq;
		}else{
			delete finsubreq;
			return NULL;
		}
	}

	// If you come here, it is a read.
	// 4. If this is a read packet. Generate the next one and put it into subreqQ (if the next one exists) and send it back.

	finsubreq->setName("JOB_FIN");
	finsubreq->setKind(JOB_FIN); // Set the common kind.

//	cout << info[i].obj_ub << " " << finsubreq->getLowoffset() << " " << finsubreq->getSize() << " " <<
//		info[i].orig_ub << " " << info[i].orig_loff << " " << info[i].orig_size << endl;
	if(info[i].obj_ub <= finsubreq->getLowoffset() + finsubreq->getSize()){ // The finished sub-request is the last one sent.
		finsubreq->setName("JOB_FIN_LAST");
		finsubreq->setKind(JOB_FIN_LAST); // Set the special kind if it is the last sub-request.
		if(reqQ->popOsQ(origid) == NULL){ // Pop the original PVFS request.
			PrintError::print("PVFS2DSD", "Can't find the id in osQ.", origid);
			return NULL;
		}
		info[i].id = 0; // Nullify the record.
#ifdef PVFS2DSD_DEBUG
			cout << "PVFS2DSD #" << myID << ": Read all done. Nullified info[].id=" << origid << endl;
#endif
	}else{ // Push the next sub-request to subreqQ.
		gPacket * nextsubreq;
		long loff = finsubreq->getLowoffset() + finsubreq->getSize();
		if(info[i].obj_ub <= loff + subreq_size){ // going to deal with the last one.
			nextsubreq = (gPacket *)reqQ->queryJob(origid);
			nextsubreq->setKind(LFILE_REQ);
			if(nextsubreq == NULL){
				PrintError::print("PVFS2DSD", "Cannot find the original request from reqQ.", id);
			}
			nextsubreq->setSize(info[i].obj_ub - loff);
		}else{
			nextsubreq = new gPacket("LFILE_REQ", LFILE_REQ);
			if((finsubreq->getID() + SUBREQ_OFFSET) % PID_OFFSET == 0) // We limit the ID space for this module.
				nextsubreq->setID(origid + SUBREQ_OFFSET);
			else
				nextsubreq->setID(finsubreq->getID()+1);
			nextsubreq->setSize(subreq_size);
			nextsubreq->setHighoffset(info[i].hoff);
			nextsubreq->setRead(info[i].read);
			nextsubreq->setApp(info[i].app);
			nextsubreq->setFileId(info[i].fid);
			nextsubreq->setDsID(info[i].decision);
			nextsubreq->setClientID(info[i].cid);
		}
		nextsubreq->setLowoffset(loff);
		subreqQ->pushWaitQ(nextsubreq);
	}

	// Reset the bounds if not fit the original bounds and return.
	if(finsubreq->getLowoffset() < info[i].orig_loff)
		finsubreq->setLowoffset(info[i].orig_loff);
	if(finsubreq->getLowoffset() + finsubreq->getSize() > info[i].orig_ub)
		finsubreq->setSize(info[i].orig_ub - finsubreq->getLowoffset());

	return finsubreq;
}

PVFS2DSD::~PVFS2DSD() {
	cout << "PVFS2DSD finish." << endl;
	fflush(stdout);
	if(reqQ != NULL)
		delete reqQ;
	if(subreqQ != NULL)
		delete subreqQ;
	cout << "PVFS2DSD finish end." << endl;
	fflush(stdout);
}
