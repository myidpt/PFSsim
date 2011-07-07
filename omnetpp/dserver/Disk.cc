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

// The Disk module has to communicate with the Disksim process through TCP.
#include "Disk.h"

Define_Module(Disk);

Disk::Disk() {
}

void Disk::initialize(){
	myId = (getId() - DISK_ID_BASE) / 3;
	portno = BASE_PORT + myId; // Port number is got by this.

	if(sock_init(portno) == -1){
		fprintf(stderr, "Socket creation failure.\n");
		deleteModule();
	}

	outstanding = 0;
	queue = new FIFO(DISK_DEGREE);

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
}

int Disk::sock_init(int portno){
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

void Disk::handleMessage(cMessage *cmsg){
	/*
	 *           DATA_REQ >
	 * LocalFS ------------- Disk
	 *          < DATA_RESP
	 */
	switch(cmsg->getKind()){
	case BLK_REQ:
		handleDataReq((gPacket *)cmsg);
		break;
	case SELF_EVENT: // self sync message.
		delete cmsg;
		handleDisksimSync();
		break;
	case BLK_RESP:
		sendSafe((gPacket *)cmsg);
		dispatchJobsAndSync();
		break;
	}
}

void Disk::handleDataReq(gPacket * datareq){
	queue->pushWaitQ(datareq);
	dispatchJobsAndSync();
}

void Disk::dispatchJobsAndSync(){
	if(dispatchJobs() > 0)
		handleDisksimSync();
}

void Disk::handleDisksimSync(){
	if(outstanding == 0) // Don't need to do sync.
		return;
	int n;
	syncNojob->time = SIMTIME_DBL(simTime()); // Transmit the current time of Dserver.
	n = write(sockfd, syncNojob, sizeof(struct syncjob_type));
	if(n < 0){
		fprintf(stderr, "disksim_sync: write error.\n");
		fflush(stdout);
		return;
	}
	while(1){
		int i;
		for(i = 0; i < 10; i ++){
			if((n = read(sockfd, syncReply, sizeof(struct syncjobreply_type))) == 0)
				sleep(0.1);
			else
				break;
		}
		if(i == 10){
			fprintf(stderr, "ERROR Disk: read from socket #%d timed out.\n", portno);
			finish();
			deleteModule();
		}
		if(n < 0){
			fprintf(stderr, "disksim_sync: read error.\n");
			return;
		}

		if(syncReply->fid > 0){ // One job is finished
			outstanding --;
			gPacket * gpkt = NULL;
			if((gpkt = queue->popOsQ(syncReply->fid)) == NULL){
				fprintf(stderr, "disksim_sync: error when dequeue from outstanding queue.\n");
				return;
			}
			gpkt->setKind(BLK_RESP);
			scheduleAt(syncReply->time, gpkt);
//			sendResult(gpkt); // Send the result back to the client.
		}else if(syncReply->time > 0){ // We still have future events, schedule next SELF_EVENT packet.
			gPacket * timemsg = new gPacket("Self-time-Dserver");
			timemsg->setKind(SELF_EVENT);
			if(SIMTIME_DBL(simTime()) > syncReply->time + 0.000001){
				fprintf(stderr, "%d - Strange: %lf > %lf\n", getId(), SIMTIME_DBL(simTime()), syncReply->time);
				syncReply->time = SIMTIME_DBL(simTime());
			}
			scheduleAt((simtime_t)(syncReply->time), timemsg);
			return;
		}else // Disk goes to idle.
			return;
	}


}

int Disk::dispatchJobs(){
	gPacket * gpkt;
	int prev = outstanding;
	while(1){
		if((gpkt = queue->dispatchNext()) == NULL)
			break;
		outstanding ++;
		syncJob->id = gpkt->getId();
		syncJob->time = SIMTIME_DBL(simTime());
		syncJob->off = gpkt->getLowoffset() % 2000000; // Disksim doesn't support very big offset.
		syncJob->len = gpkt->getSize();

		if (write(sockfd, syncJob, sizeof(struct syncjob_type)) < 0){
			fprintf(stderr, "ERROR writing to socket\n");
			return -1;
		}
	}
	return (outstanding - prev);
}

void Disk::sendSafe(gPacket * gpkt){
	send(gpkt, "g$o");
}

void Disk::finish(){
	write(sockfd, syncEnd, sizeof(struct syncjob_type));
	sleep(0.5);
	close(sockfd);
	free(syncJob);
	free(syncNojob);
	free(syncEnd);
}

Disk::~Disk() {
}