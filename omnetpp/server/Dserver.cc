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

#include "server/Dserver.h"

Define_Module(Dserver);

void Dserver::initialize()
{
	myid = getId() - DS_ID_BASE;
	int portno = BASE_PORT + myid; // Port number is got by this.

	if(sock_init(portno) == -1){
		fprintf(stderr, "Socket creation failure.\n");
		deleteModule();
	}

	outstanding = 0;
	degree = DEGREE;
	char type[5] = {'F','I','F','O','\0'};
	if(!strcmp(type, "FIFO"))
		queue = new FIFO(getId() - DS_ID_BASE);
	else if(!strcmp(type,"SFQ"))
		queue = new SFQ(getId() - DS_ID_BASE);

	syncNojob = (struct syncjob_type *)malloc(sizeof(struct syncjob_type));
	bzero((char *)syncNojob, sizeof(struct syncjob_type));
	syncNojob->id = 0;
	syncEnd = (struct syncjob_type *)malloc(sizeof(struct syncjob_type));
	bzero((char *)syncEnd, sizeof(struct syncjob_type));
	syncEnd->id = -1;
	syncJob = (struct syncjob_type *)malloc(sizeof(struct syncjob_type));
	bzero((char *)syncJob, sizeof(struct syncjob_type));
	syncReply = (struct syncjobreply_type *)malloc(sizeof(struct syncjobreply_type));
	bzero((char *)syncReply, sizeof(struct syncjobreply_type));

	wBuffer = 0;
	rCache = MAXRCACHESIZE;
	serverIdle = true;
}

int Dserver::sock_init(int portno){
	struct sockaddr_in serv_addr;
	struct hostent *server;
	server = gethostbyname("localhost");

	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "ERROR opening socket.\n");
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		fprintf(stderr, "ERROR connecting - ID: %d - Port: %d\n", getId(), portno);
		return -1;
	}
	fprintf(stdout, "Connected! - ID: %d - Port: %d\n",getId(), portno);
	return 1;
}

void Dserver::handleMessage(cMessage * cmsg)
{
	switch(cmsg->getKind()){
	case SELF_EVENT: // self sync message.
		printf("handleDisksimSync start %ld\n", cmsg->getId());
		fflush(stdout);
		handleDisksimSync((gPacket *)cmsg);
		printf("handleDisksimSync end %ld\n", cmsg->getId());
		fflush(stdout);
		break;
	case JOB_REQ: // New job from client.
		printf("handleJobReq start %ld\n", cmsg->getId());
		fflush(stdout);
		handleJobReq((gPacket *)cmsg);
		printf("handleJobReq end %ld\n", cmsg->getId());
		fflush(stdout);
		break;
	case SCH_JOB:
		printf("handleSchJob start %ld\n", cmsg->getId());
		fflush(stdout);
		handleSchJob((gPacket *)cmsg);
		printf("handleSchJob end %ld\n", cmsg->getId());
		fflush(stdout);
		break;
	case R_CACHE_JOB_DONE:
	case R_DISK_JOB_DONE:
	case W_CACHE_JOB_DONE:
	case W_DISK_JOB_DONE:
		printf("handleDoneJob start %ld\n", cmsg->getId());
		fflush(stdout);
		handleDoneJob(((gPacket *)cmsg)->getId());
		printf("handleDoneJob end %ld\n", cmsg->getId());
		fflush(stdout);
	}
}

void Dserver::handleDisksimSync(gPacket * gpkt){
	delete gpkt;
	disksim_sync(); // Prob.
}

void Dserver::handleDoneJob(long id){
	outstanding --;
	gPacket * gpkt = NULL;
	if((gpkt = queue->popOsQ(id)) == NULL){
		fprintf(stderr, "disksim_sync: error when dequeue from outstanding queue.\n");
	}
	if(gpkt->getKind() == W_DISK_JOB_DONE)
		wBuffer -= WRITTENBACKSIZE; // Write back partial of the buffer.
	gpkt->setFinishtime(SIMTIME_DBL(simTime())); // Set the finish time as now.
	sendResult(gpkt);
	dispatchJobs();
}

void Dserver::handleSchJob(gPacket * gpkt){
	queue->pushWaitQ(gpkt);
	dispatchJobs();
}

void Dserver::handleJobReq(gPacket * gpkt){
	gpkt->setArrivaltime(SIMTIME_DBL(simTime()));
	if(SCH_DELAY != 0){
		gpkt->setKind(SCH_JOB);
		scheduleAt((simtime_t)(SIMTIME_DBL(simTime()) + SCH_DELAY), gpkt);
	}
	else
		handleSchJob(gpkt);
}

// Return is the number of jobs dispatched.
// Dispatches the jobs until outstanding == degree, or no more waiting jobs.
int Dserver::dispatchJobs(){

	printf("dispatchJobs start\n");
	fflush(stdout);

	int prev = outstanding;
	gPacket * gpkt;

	while(outstanding < degree){
		if((gpkt = queue->dispatchNext()) == NULL)
			break;

		outstanding ++;

		gpkt->setDispatchtime(SIMTIME_DBL(simTime()));

		double sizekb = gpkt->getSize() / 1024;
		if(sizekb < 1)
			sizekb = 1; // minimum cost.
		if(gpkt->getRead() == 0){ // Write to buffer
			wBuffer += sizekb;
			if(wBuffer < MAXWBUFFSIZE){ // Writing Buffer not full.
				printf("Buffer not full = %d.\n",wBuffer);
				fflush(stdout);
				gpkt->setKind(W_CACHE_JOB_DONE);
				scheduleAt(SIMTIME_DBL(simTime()) + WBUFF_TIME * sizekb, gpkt);
			} else {
				printf("Buffer full now.\n");
				fflush(stdout);
				syncJob->id = gpkt->getId();
				syncJob->time = gpkt->getDispatchtime();
				syncJob->off = (gpkt->getHighoffset() * LOWOFFSET_RANGE + gpkt->getLowoffset())%10000000;
		//		syncJob->len = MAXWBUFFSIZE;
				syncJob->len = WDISK_SIZE;
				syncJob->read = gpkt->getRead();

				if (write(sockfd, syncJob, sizeof(struct syncjob_type)) < 0){
					fprintf(stderr, "ERROR writing to socket\n");
					return -1;
				}

//				if(prev == 0 && outstanding > 0){ // From idle to busy
					printf("Dserver %d goes to busy.\n", getId()-DS_ID_BASE);
					fflush(stdout);
					serverIdle = false;
					disksim_sync();
//				}
			}
		} else { // Limited read cache. Extra read requests directly go to the disk.
			if(rCache > 0) {
				rCache -= sizekb;
				gpkt->setKind(R_CACHE_JOB_DONE);
				scheduleAt(SIMTIME_DBL(simTime()) + RBUFF_TIME * sizekb, gpkt);
			} else {
				degree = 1; // when accessing the disk, we set it to be sequential.
				gpkt->setKind(R_DISK_JOB_DONE);
				scheduleAt(SIMTIME_DBL(simTime()) + RDISK_TIME * sizekb, gpkt);
				// Temporarily simpilfied
				/*
				syncJob->id = gpkt->getId();
				syncJob->time = gpkt->getDispatchtime();
				syncJob->off = gpkt->getOffset()%10000000;
				syncJob->len = RDISK_SIZE;
				if (write(sockfd, syncJob, sizeof(struct syncjob_type)) < 0){
					fprintf(stderr, "ERROR writing to socket\n");
					return -1;
				}
				serverIdle = false;
	//				if(prev == 0 && outstanding > 0){ // From idle to busy
				disksim_sync();
		//		printf("Dserver %d goes to busy.\n", getId()-DS_ID_BASE);
//				}
 * */
			}
		}
	}

	return (outstanding - prev);
}

int Dserver::disksim_sync(){
	int n;
	if(serverIdle) // Don't need to do sync.
		return 0;
//	printf("inside sync.");
//	fflush(stdout);
	syncNojob->time = SIMTIME_DBL(simTime()); // Transmit the current time of Dserver.
	n = write(sockfd, syncNojob, sizeof(struct syncjob_type));
	if(n < 0){
		fprintf(stderr, "disksim_sync: write error.\n");
		fflush(stdout);
		return -1;
	}
//	printf("Go to getting sync message (meanwhile finish job) while loop.\n");
//	fflush(stdout);
	while(1){
		while((n = read(sockfd, syncReply, sizeof(struct syncjobreply_type))) == 0)sleep(0.1);
		if(n < 0){
			fprintf(stderr, "disksim_sync: read error.\n");
			return -1;
		}

		if(syncReply->fid > 0){ // One job is finished
/*			outstanding --;
			gPacket * gpkt = NULL;
			if((gpkt = queue->popOsQ(syncReply->fid)) == NULL){
				fprintf(stderr, "disksim_sync: error when dequeue from outstanding queue.\n");
				return -1;
			}*/
			gPacket * gpkt = queue->queryJob(syncReply->fid);
			if(gpkt == NULL){
				fprintf(stderr, "Can't find job from the queue!");
				return -1;
			}
			if(gpkt->getRead() == 0)
				gpkt->setKind(W_DISK_JOB_DONE);
			else
				gpkt->setKind(R_DISK_JOB_DONE);
			scheduleAt(syncReply->time, gpkt);
			printf("job to be finished at %f\n",syncReply->time);
			fflush(stdout);
//			sendResult(gpkt); // Send the result back to the client.
		}
		else // time > 0 or time < 0, sync message
			break;
	}

	if(syncReply->time > 0){ // We still have future events.
//		printf("%lf: Dserver %d still gets syncReply.\n", SIMTIME_DBL(simTime()), getId()-DS_ID_BASE);
//		fflush(stdout);
		gPacket * timemsg = new gPacket("Self-time-Dserver");
		timemsg->setKind(SELF_EVENT);
		if(SIMTIME_DBL(simTime()) > syncReply->time + 0.000001){
			fprintf(stderr, "%d - Strange: %lf > %lf\n", getId(), SIMTIME_DBL(simTime()), syncReply->time);
			syncReply->time = SIMTIME_DBL(simTime());
		}
		scheduleAt((simtime_t)(syncReply->time), timemsg);
	}
	else{
		printf("%lf: Dserver %d goes to idle.\n", SIMTIME_DBL(simTime()), getId()-DS_ID_BASE);
		fflush(stdout);
		serverIdle = true;
	}
	return 1;
}

int Dserver::sendResult(gPacket * gpkt){
	gpkt->setName("Result");
	gpkt->setKind(JOB_RESP);
	gpkt->setByteLength(50); // Assume length of results are 50.
	sendSafe(gpkt);
	return 1;
}

void Dserver::sendSafe(gPacket * gpkt){
	cChannel * cch = gate("g$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(gpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//		printf("Send delayed.\n");
	}
	else
		send(gpkt, "g$o");
}

void Dserver::finish(){
	write(sockfd, syncEnd, sizeof(struct syncjob_type));
	sleep(0.5);
	close(sockfd);
	free(syncJob);
	free(syncNojob);
	free(syncEnd);
}
