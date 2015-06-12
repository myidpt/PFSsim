// The Disk module has to communicate with the Disksim process through TCP.
// Yonggang Liu

#include "dserver/disk/Disk.h"

Define_Module(Disk);

int Disk::idInit = 0;
int Disk::numDisk = 0;

Disk::Disk() {
}

void Disk::initialize(){

    //Parse too many time but if configuration change we have to update the value
    numDisk=par("numDservers").longValue();//Suppose that there is 1 Disk per Server, If not change the omnet.ini and parse the correct value and change the variable in Disk.ned
    myID = idInit ++;
    if(myID>=numDisk-1){
       idInit=0;
    }

	diskSize = par("disk_size").longValue();
//	zoneSize = diskSize / ZONECOUNT;
	degree = par("degree").longValue();
	ra_size = par("ra_size").longValue();
	return_zero = par("return_zero").longValue();
	return_zero_period = par("return_zero_period").doubleValue();

	if(readParmFile(par("disk_parm_path").stringValue())){
		printf("Disk #%d: Disk parameter file read successful.\n", myID);
		fflush(stdout);
	}else{
		PrintError::print("Disk", "disk configuration file read error.");
		deleteModule();
	}

	queue = SchedulerFactory::createScheduler(SchedulerFactory::FIFO_ALG, myID, degree);

	jumpsizes[0] = 1;
	for(int i = 1; i < JUMPSIZECOUNT; i ++)
		jumpsizes[i] = jumpsizes[i-1] << 1;
	blksizes[0] = 1;
	for(int i = 1; i < BLKSIZECOUNT; i ++)
		blksizes[i] = blksizes[i-1] << 1;
#ifdef MONITOR_DISKACCESS
	char debugfname[10] = {'d', 'i', 's', 'k', '1', '\0'};
	debugfname[4] = myID + '0';
	debugfp = fopen(debugfname, "w+");
#endif

	// Init the RA helper-recorders.
	last_offset = 0;
	seq_read_sofar = 0;
	last_jump = 0;
	last_access_time = 0;
}

void Disk::handleMessage(cMessage *cmsg){
	/*
	 *           DATA_REQ >
	 * LocalFS ------------- Disk
	 *          < DATA_RESP
	 */
	switch(cmsg->getKind()){
	case BLK_REQ:
		handleBlockReq((BlkRequest *)cmsg);
		break;
	case BLK_RESP:
		handleBlockResp((BlkRequest *)cmsg);
		break;
	}
}

void Disk::handleBlockReq(BlkRequest * datareq){
#ifdef MONITOR_DISKACCESS
	fprintf(debugfp, "%lf %ld %ld %ld\n", SIMTIME_DBL(simTime()), datareq->getBlkStart(), last_offset,
			datareq->getBlkEnd()-datareq->getBlkStart());
#endif
#ifdef DISK_DEBUG
	if(myID == 4)
	cout << "Disk#" << myID << ": time[" << SIMTIME_DBL(simTime()) << "] ID[" << datareq->getID() << "] start[" << datareq->getBlkStart() <<
			"] size[" << datareq->getBlkEnd()-datareq->getBlkStart() << "] last_off[" << last_offset << "]" << endl;
#endif
	queue->pushWaitQ(datareq);
	dispatchJobs();
}

void Disk::handleBlockResp(BlkRequest * dataresp){
#ifdef DISK_DEBUG
	if(myID == 4)
	cout << "Disk#" << myID << ": time[" << SIMTIME_DBL(simTime()) << "] Finished - DiskReq ID[" << dataresp->getID() <<"]" << endl;
#endif
	if(queue->popOsQ(dataresp->getID(), dataresp->getSubID()) == NULL)
		PrintError::print("Disk", "handleBlockResp cannot find request from the OsQ.", dataresp->getID());
	last_access_time = SIMTIME_DBL(simTime());
	sendSafe(dataresp);
	dispatchJobs();
}

#define MAX_BLK_BATCH 1024

void Disk::dispatchJobs(){

	if(return_zero == 1 && SIMTIME_DBL(simTime()) - last_access_time > return_zero_period){ // The disk head may go to other places due to other tasks.
//		cout << "TIME: " << SIMTIME_DBL(simTime()) << " - " << SIMTIME_DBL(simTime()) - last_access_time << endl;
		last_offset = 0;
	}else{
//		cout << "--TIME: " << SIMTIME_DBL(simTime()) << " - " << SIMTIME_DBL(simTime()) - last_access_time << endl;
	}

	BlkRequest * req;
	while(1){
		if((req = (BlkRequest *)queue->dispatchNext()) == NULL)
			break;

#ifdef DISK_DEBUG
		if(myID == 4)
	cout << "Disk#" << myID << ": dispatchJobs. DiskReq ID[" << req->getID() << "]";
	fflush(stdout);
#endif

		req->setName("BLK_RESP");
		req->setKind(BLK_RESP);

		if(req->getBlkEnd() > diskSize){
			PrintError::print("Disk - dispatchJobs", "Target address is out of bound.", req->getBlkEnd());
			return;
		}

		int length = req->getBlkEnd() - req->getBlkStart();

		if(length > MAX_BLK_BATCH || length <= 0 || req->getBlkStart() > MAX_DISK_OFFSET || req->getBlkStart() < 0){
			PrintError::print("Disk", "Invalid block request to Disksim. start = ", req->getBlkStart());
			cerr << "length=" << length << endl;
			fflush(stderr);
			return;
		}

		long jump = abs(last_offset - req->getBlkStart());
		if(jump > jumpsizes[JUMPSIZECOUNT-1]) // Even bigger than the largest jump size.
			jump = jumpsizes[JUMPSIZECOUNT-1];

		if(req->getRead() && length < ra_size && checkReadahead(req)){
			length = ra_size;
		}

		// Update disk head.
		last_offset = req->getBlkEnd();

		int r = (int)(req->getRead());
		int jumpi, blki;
		int tlength = length;
		long tjump = jump;
		for(jumpi = 0; tjump > 0; jumpi ++)
			tjump >>= 1;
		for(blki = 0; tlength > 0; blki ++)
			tlength >>= 1;

		double timespan, timespan1, timespan2;
//		int zone = req->getBlkStart() / zoneSize;
		if(jumpi == 0){ // Sequential
#ifdef DISK_DEBUG
			if(myID == 4)
			cout << " - Disk: Sequential" << endl;
#endif
			if(blki == BLKSIZECOUNT){
				timespan = seqtime[r][blki-1];
			}
			else{
				timespan = (seqtime[r][blki]-seqtime[r][blki-1])
					*(length-blksizes[blki-1])/(blksizes[blki]-blksizes[blki-1])+seqtime[r][blki-1];
			}

//			cout << r << " " << blki-1 << " " << seqtime[r][blki-1] << endl;

//			printf("sequential: timespan=%lf, myoffset = %ld, length = %d\n", timespan * 1000, last_offset, length);
//			fflush(stdout);
		}else{
#ifdef DISK_DEBUG
			if(myID == 4)
			cout << " - Disk: Jump " << jump << endl;
#endif
			if(blki == BLKSIZECOUNT){ // Max
				if(jumpi == JUMPSIZECOUNT){
					timespan = jumptime[r][blki-1][jumpi-1];
				}else{
					timespan = (jumptime[r][blki-1][jumpi]-jumptime[r][blki-1][jumpi-1])
						*(jump-jumpsizes[jumpi-1])/(jumpsizes[jumpi]-jumpsizes[jumpi-1])+jumptime[r][blki-1][jumpi-1];
				}
			}else{
				if(jumpi == JUMPSIZECOUNT){
					timespan1 = jumptime[r][blki-1][jumpi-1];
					timespan2 = jumptime[r][blki][jumpi-1];
				}else{
					timespan1 = (jumptime[r][blki-1][jumpi]-jumptime[r][blki-1][jumpi-1])
						*(jump-jumpsizes[jumpi-1])/(jumpsizes[jumpi]-jumpsizes[jumpi-1])+jumptime[r][blki-1][jumpi-1];
					timespan2 = (jumptime[r][blki][jumpi]-jumptime[r][blki][jumpi-1])
						*(jump-jumpsizes[jumpi-1])/(jumpsizes[jumpi]-jumpsizes[jumpi-1])+jumptime[r][blki][jumpi-1];
				}
				timespan = (timespan2-timespan1)*(length-blksizes[blki-1])/(blksizes[blki]-blksizes[blki-1])+timespan1;
			}
//			printf("jump: jump=%ld, timespan=%lf, myoffset = %ld, length = %d\n", jump, timespan * 1000, last_offset, length);
//			fflush(stdout);
		}

		if(timespan < 0)
			PrintError::print("Disk - dispatchJobs", "timespan is zero or negative.", timespan);

		/*if(myID == 5){
		    cout << "Disk::dispatchJobs() timespan(ms) : " << timespan << endl;
		}*/
		scheduleAt(SIMTIME_DBL(simTime()) + timespan, req);
	}
}

void Disk::sendSafe(BlkRequest * req){
	send(req, "g$o");
}

#define LAST_JUMP_THRESHOLD 180
#define JUMP_THRESHOLD	16
#define SEQ_RA_THRESHOLD	96

bool Disk::checkReadahead(BlkRequest * req){
// If read-ahead is involved, don't read from the table as normal.
// The read-ahead judge is simplified here, and it is not suitable to all disks.
	long jump = abs(req->getBlkStart() - last_offset);
	bool ra = false;

//	printf("last_jump = %ld, jump = %ld, seq_read_sofar = %d.\n", last_jump, jump, seq_read_sofar);

	// big jump + small jump -> read-ahead
	if(last_jump >= LAST_JUMP_THRESHOLD && jump <= JUMP_THRESHOLD && jump != 0)
		ra = true;
	if(last_jump < 1024 && jump > 786432 && seq_read_sofar < 96)
		ra = true;
	// sequential read + jump -> read-ahead?
	if(last_jump == 0 && jump > 0){
		if(seq_read_sofar == 96)
			ra = true;
		else if(seq_read_sofar > 96 && jump > 786432)
			ra = true;
	}

	// sequential, and already 96 blocks -> read-ahead
	if(jump == 0 && seq_read_sofar == 96)
			ra = true;

	// Update seq_read_sofar.
	if(jump == 0)
		seq_read_sofar += req->getBlkEnd() - req->getBlkStart();
	else
		seq_read_sofar = req->getBlkEnd() - req->getBlkStart();

	last_jump = jump; // Update last_jump.
//	cout << "RA = " << ra << endl;
	return ra;
}

int Disk::readParmFile(const char * pfname){
	FILE * fp = fopen(pfname, "r");
	if(fp == NULL){
		PrintError::print("Disk-readParmFile",  string("can not open file") + pfname);
		return -1;
	}
	char line[800];
	char op[50];
	int blksize; // The one-time access size.
	int blkindex;
	int readflag;

	memset(jumptime, 0, sizeof(jumptime));
	memset(seqtime, 0, sizeof(seqtime));

	while(fgets(line, 300, fp) != NULL){
		if(line[0] != '[') // Not a header
			continue;
		sscanf(line, "[%s %d]", op, &blksize);

		if(blksize % BASEBLKSIZE != 0){
			PrintError::print("Disk - readParmFile", "sampled block size should be 1x, 2x, 4x, 8x ... of ", BASEBLKSIZE);
			fclose(fp);
			return -1;
		}

		blkindex = -1;
		for(int tmp = blksize / BASEBLKSIZE;tmp > 0; tmp >>= 1){
			blkindex ++;
		}

		if(blkindex >= BLKSIZECOUNT || blkindex == -1){
			PrintError::print("Disk-readParmFile", "sampled block size too small or too big.", blksize);
			fclose(fp);
			return -1;
		}

		if(op[1] == 'r')
			readflag = 1;
		else if(op[1] == 'w')
			readflag = 0;
		else{
			PrintError::print("Disk-readParmFile", string("Can not recognize operation type ")+op);
			fclose(fp);
			return -1;
		}

		if(op[0] == 'r'){
			for(int i = 0; i < JUMPSIZECOUNT; i ++)
				fscanf(fp, "%lf", &jumptime[readflag][blkindex][i]);
		}else if(op[0] == 's'){
			fscanf(fp, "%lf", &seqtime[readflag][blkindex]);
		}else{
			PrintError::print("Disk-readParmFile", string("Can not recognize operation type ")+op);
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);

	// Check if all the parameters are read.
	for(int i = 0; i < 2; i ++){
		for(int j = 0; j < BLKSIZECOUNT; j ++){
			for(int k = 0; k < JUMPSIZECOUNT; k ++){
#ifdef DISK_DEBUG
				cout << jumptime[i][j][k] << "\t";
#endif
				jumptime[i][j][k] /= 1000; // From ms to sec.
				if(jumptime[i][j][k] == 0){
					char tmps[50];
					sprintf(tmps, "Incomplete parameter, Rand%s-%d-%d", i==1?"read":"write", j, k);
					PrintError::print("Disk-readParmFile", tmps);
					return -1;
				}
			}
#ifdef DISK_DEBUG
			cout << endl;
#endif
			seqtime[i][j] /= 1000;
			if(seqtime[i][j] == 0){
				char tmps[50];
				sprintf(tmps, "Incomplete parameter, Seq%s-%d", i==1?"read":"write", j);
				return -1;
			}
		}
	}
	return 1;
}

int Disk::getID(){
	return myID;
}

void Disk::finish(){
#ifdef MONITOR_DISKACCESS
	fclose(debugfp);
	cout << "Disk - finish." << endl;
	fflush(stdout);
#endif
	cout << "Disk finish." << endl;
	fflush(stdout);
	if(queue != NULL)
		delete queue;
	cout << "Disk finish end." << endl;
	fflush(stdout);
}

Disk::~Disk() {
}
