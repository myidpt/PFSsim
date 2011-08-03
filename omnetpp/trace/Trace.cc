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

#include "Trace.h"

const int Trace::SW_SENT = 0;
const int Trace::SW_RECEIVED = -1;
const int Trace::SW_NULL = -2;

Trace::Trace(int id, double t, long long off, long s, int r, int a, int s2):
	cNamedObject("Trace"){
	myId = id;
	stime = t;
	offset = off;
	size = s;
	unProcessedSize = s;
	read = r;
	app = a;
	sync = s2;
	for(int i = 0; i < MAX_DS; i ++){
		serverWindow[i] = SW_NULL; // Mark as not related server.
		dsoffsets[i] = 0; // Mark the data server offsets as 0.
	}
	layout = new Layout(app); // Initialize the layout object
}

// Check if there are still unsent packets in the window.
// If yes, return it, if no, return NULL.
gPacket * Trace::getNextgPacketFromWindow(){
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(serverWindow[i] > 0){ // Unsent
			gPacket * gpkt = new gPacket("gpacket"); // Create new gPacket
			gpkt->setLowoffset(dsoffsets[i] % LOWOFFSET_RANGE);
			gpkt->setHighoffset(dsoffsets[i] / LOWOFFSET_RANGE);
			gpkt->setSize(serverWindow[i]);
			gpkt->setRead(read);
			gpkt->setApp(app);
			gpkt->setDecision(layout->getServerID(i));
			serverWindow[i] = SW_SENT;
			return gpkt;
		}
	}
	return NULL;
}

/*
 * If there are still unsent packets in the window, return a packet in this window.
 * Otherwise, return NULL (there are 3 cases):
 * 1. Layout not queried .
 * 2. All packets are sent in this window, but waiting for the response.
 * 3. You have received all the packets, and this is the last window (normally this case does not happen in this method).
 */
gPacket * Trace::nextgPacket() {
	if (layout->getWindowSize() == 0){
		fprintf(stderr, "ERROR Trace: the trace did not query the data layout.\n");
		return NULL;
	}

	// If the current window has not sent all the packets.
	gPacket * gpkt = getNextgPacketFromWindow();
	if(gpkt != NULL){
		return gpkt;
	}

	for(int i = 0; i < layout->getServerNum(); i ++){
		if(serverWindow[i] == SW_SENT){ // Un-received packets exist.
			return NULL;
		}
	}

	if(unProcessedSize == 0){ // This is the last window, and it has sent all its packets.
		return NULL;
	}

	// Current window is all done (sending and receiving), and next window exists.
	// Do next window.
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(unProcessedSize == 0){
			serverWindow[i] = SW_NULL;
		}else if(layout->getServerShare(i) < unProcessedSize){
			serverWindow[i] = layout->getServerShare(i);
			unProcessedSize -= layout->getServerShare(i);
		}else{
			serverWindow[i] = unProcessedSize;
			unProcessedSize = 0;
		}
	}
	return getNextgPacketFromWindow();
}

// Return -1: Wrong packet.
// Return 0: Still have more to receive / send, continue to wait.
// Return 1: You have done the current window, move forward to the next window.
// Return 2: This trace is all done.
int Trace::finishedgPacket(gPacket * gpkt){
	// Mark the slot in the window as done. 0 -> 1
	int serverindex = layout->findServerIndex(gpkt->getDecision());
	if(serverindex < 0){
		fprintf(stderr, "ERROR trace: received wrong packet from server #%d", gpkt->getDecision());
		return -1;
	}

	// Mark: Received one packet, and increment the offset of the data on that server.
	serverWindow[serverindex] = SW_RECEIVED;
	dsoffsets[serverindex] += gpkt->getSize();

	// Check if all the slots in the window are done.
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(serverWindow[i] >= 0)
			return 0; // This window is undone.
	}

	// This window is done.
	if(unProcessedSize == 0){ // This trace is all done.
		ftime = SIMTIME_DBL(simTime());
		return 2;
	}

	// Still have more windows.
	return 1;
}

void Trace::setLayout(qPacket * qpkt) {
	layout->setLayout(qpkt);
	// Set the correct offset on each data server.
	long jumps = offset / layout->getWindowSize();
	long remainder = offset % layout->getWindowSize();
	for(int i = 0; i < layout->getServerNum(); i ++){
		dsoffsets[i] += jumps * layout->getServerShare(i);
		if(remainder < layout->getServerShare(i)){
			dsoffsets[i] += remainder;
			remainder = 0;
		}
		else{
			dsoffsets[i] += layout->getServerShare(i);
			remainder -= layout->getServerShare(i);
		}
	}
	// Set the correct first window.
	long smalleroffset = offset % layout->getWindowSize();
	long rest;
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(layout->getServerShare(i) > 0 && smalleroffset >= layout->getServerShare(i)){
			serverWindow[i] = SW_NULL; // For the first window, it is closed.
			smalleroffset -= layout->getServerShare(i);
		}
		else{
			rest = layout->getServerShare(i) - smalleroffset;
			smalleroffset = 0;
			if(rest > unProcessedSize){
				serverWindow[i] = unProcessedSize;
				unProcessedSize = 0;
			}
			else{
				serverWindow[i] = rest;
				unProcessedSize -= rest;
			}
		}
	}
}

double Trace::getStarttime(){
	return stime;
}
double Trace::getFinishtime(){
	return ftime;
}
long long Trace::getOffset(){
	return offset;
}
int Trace::getId(){
	return myId;
}
long Trace::getSize(){
	return size;
}
int Trace::getRead(){
	return read;
}
int Trace::getApp(){
	return app;
}
int Trace::getSync(){
	return sync;
}

Trace::~Trace() {
	delete layout;
}
