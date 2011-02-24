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

#include "Request.h"

Request::Request(double t, long long off, long s, int r, int a, int s2):
	cNamedObject("request"){
	stime = t;
	offset = off;
	size = s;
	read = r;
	app = a;
	sync = s2;
	index = offset;
	windowSize = 0;
	if(size > MAX_WINDOW_SIZE)
		windowsizeInByte = MAX_WINDOW_SIZE;
	else
		windowsizeInByte = size;
	for(int i = 0; i < MAX_DS_NUM; i ++){
		dsList[i] = -1;
		window[i] = -1;
	}
}

gPacket * Request::nextgPacket() {
	if (windowSize == 0){
		fprintf(stderr, "Error: the request did not query for the data layout.\n");
		return NULL;
	}
	// Check if there are still unsent packets in the window.
	for(int i = 0; i < windowSize; i ++){
		if(window[i] == -1){ // Unsent
			gPacket * gpkt = new gPacket("gpacket"); // Create new gPacket
			gpkt->setOffset(index);
			gpkt->setSize(windowsizeInByte/windowSize);
			gpkt->setRead(read);
			gpkt->setApp(app);
			gpkt->setDecision(dsList[i]);
			window[i] = 0;
			return gpkt;
		}
	}
	return NULL;
}

// Return -1: All done.
// Return 0: Still have more to receive, continue to wait.
// Return 1: You have done the current window, move forward to the next window.
int Request::finishedgPacket(gPacket * gpkt){
	for(int i = 0; i < windowSize; i ++){
		if(gpkt->getDecision() == dsList[i]){
			window[i] = 1;
			break;
		}
	}
	// Finished one packet.
	delete gpkt; // Destroy the packet.
	for(int i = 0; i < windowSize; i ++){
		if(window[i] != 1)
			return 0; // Undone.
	}
	index += windowsizeInByte; // Increment the index.
	printf("offset = %lld, size = %ld, index = %lld\n", offset, size, index);
	fflush(stdout);
	if(index == offset + size){ // You reached the end of the request.
		ftime = SIMTIME_DBL(simTime());
		return -1;
	}else if(index + MAX_WINDOW_SIZE >= offset + size){ // This is the last window.
		windowsizeInByte = offset + size - index;
	}else { // Still have more than one windows.
		windowsizeInByte = MAX_WINDOW_SIZE;
	}

	for(int i = 0; i < windowSize; i ++) // Initialize
		window[i] = -1;

	return 1;
}

// Currently we only support simple layout,
// i.e., data is evenly distributed on designated Dservers.
void Request::setLayout(qPacket * qpkt) {
	windowSize = qpkt->getDsNum();
	for(int i = 0; i < windowSize; i ++)
		dsList[i] = qpkt->getDsList(i);
}

double Request::getStarttime(){
	return stime;
}
double Request::getFinishtime(){
	return ftime;
}
long long Request::getOffset(){
	return offset;
}
long Request::getSize(){
	return size;
}
int Request::getRead(){
	return read;
}
int Request::getApp(){
	return app;
}
int Request::getSync(){
	return sync;
}

Request::~Request() {
}
