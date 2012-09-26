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

#include "WindowBasedTrace.h"

const int WindowBasedTrace::SW_NULL = 0;
const int WindowBasedTrace::SW_RECEIVED = -1;
const int WindowBasedTrace::SW_SENT = -2;

WindowBasedTrace::WindowBasedTrace() {
	ID = -1;
}

void WindowBasedTrace::initialize(int tid, double time, int fid, long long off,
		long s, int r, int tfid, int appid, int syn) {
	ID = tid;
	startTime = time;
	pfsFileID = fid;
	offset = off;
	totalSize = s;

	read = r;
	traceFileID = tfid;
	applicationID = appid;
	sync = syn;

	unProcessedSize = s;
}

void WindowBasedTrace::initialize(const AppRequest * request) {
	ID = request->getID();
	startTime = request->getStarttime();
	pfsFileID = request->getFileID();
	offset = request->getHighoffset() * LOWOFFSET_RANGE + request->getLowoffset();
	totalSize = request->getTotalSize();

	read = request->getRead();
	traceFileID = request->getTraceFileID();
	applicationID = request->getApp();
	sync = request->getSync();

	unProcessedSize = totalSize;
}

// Check if there are still unsent packets in the window.
// If yes, return it, if no, return NULL.
gPacket * WindowBasedTrace::getNextgPacketFromWindow(){
	if (layout == NULL) {
		PrintError::print("Trace", "The data layout is unset.");
		return NULL;
	}
	gPacket * packet = NULL;
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(dataSizeInWindow[i] > 0){ // Unsent
			packet = new gPacket("JOB_REQ");
			packet->setKind(JOB_REQ);
			packet->setLowoffset(dsoffsets[i] % LOWOFFSET_RANGE);
			packet->setHighoffset(dsoffsets[i] / LOWOFFSET_RANGE);
			packet->setSize(dataSizeInWindow[i]);
			sentPktSize[i] = dataSizeInWindow[i];
			packet->setRead(read);
			packet->setFileId(pfsFileID);
			packet->setAggregateSize(aggregateSize);
			packet->setSubID(ID);
			packet->setApp(applicationID);
			packet->setDsID(layout->getServerID(i));
			dataSizeInWindow[i] = SW_SENT;
			return packet;
		}
	}
	return NULL;
}

/* This function opens a new window, unless one the the following happens:
 * 1. Un-received packets exist.
 * 2. It has sent all its packets, but this is the last window.
 */
bool WindowBasedTrace::openNewWindow(){
	if (layout == NULL) {
		PrintError::print("Trace", "The data layout is unset.");
		return NULL;
	}

	if(unProcessedSize == 0){ // The last window is the already done.
		return false;
	}

	// Current window is all done (sending and receiving), and next window exists.
	// Do next window.
	// Set up the current total window size, i.e., aggregateSize.
	if(maxWindowSize < unProcessedSize){
		aggregateSize = maxWindowSize;
		unProcessedSize -= maxWindowSize;
	}else{
		aggregateSize = unProcessedSize;
		unProcessedSize = 0;
	}

	// Set up the request size for each server.

	// Init
	for(int i = 0; i < layout->getServerNum(); i ++)
		dataSizeInWindow[i] = SW_NULL;

	long leftAggSize = aggregateSize;
	long tmpsize;
	bool firsttime = true;
	for(int i = offset_start_server;; i ++){
		if(i == layout->getServerNum())
			i = 0;

		// Do it window by window. May have multiple rounds.
		if(leftAggSize == 0){
			break;
		}

		tmpsize = layout->getServerStripeSize(i);
		if(i == offset_start_server && firsttime){
			tmpsize -= offset_start_position;
			firsttime = false;
		}

		if(tmpsize < leftAggSize){
			dataSizeInWindow[i] += tmpsize;
			leftAggSize -= tmpsize;
		}else{
			dataSizeInWindow[i] += leftAggSize;
			leftAggSize = 0;
		}
	}
	return true;
}

/*
 * If there are still unsent packets in the window, return a packet in this window.
 * Otherwise, return NULL (there are 3 cases):
 * 1. Layout not queried .
 * 2. All packets are sent in this window, but waiting for the response.
 * 3. You have received all the packets, and this is the last window (normally this case does not happen in this method).
 */
gPacket * WindowBasedTrace::nextgPacket() {
	if (layout == NULL) {
		PrintError::print("Trace", "The data layout is unset.");
		return NULL;
	}
	if (layout->getWindowSize() == 0) {
		PrintError::print("Trace", "The trace did not query the data layout.");
		return NULL;
	}

	// First see if there are still unsent packets in window.
	gPacket * packet = getNextgPacketFromWindow();
	if(packet != NULL) {
		return packet;
	}

	// Check the data chunks for all data servers: any un-replied packets?
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(dataSizeInWindow[i] == SW_SENT){ // Un-replied packets exist.
			return NULL;
		}
	}

	// All the packets from the current window are sent and received.
	if(! openNewWindow()) {
		return NULL;
	}
	return getNextgPacketFromWindow();
}

// Return -1: Wrong packet.
// Return 0: Still have more to receive / send, continue to wait.
// Return 1: You have done the current window, move forward to the next window.
// Return 2: This trace is all done.
WindowBasedTrace::status WindowBasedTrace::finishedgPacket(const gPacket * gpkt){
	// Mark the slot in the window as done. 0 -> 1
	int serverindex = layout->findServerIndex(gpkt->getDsID());
	if(serverindex < 0){
		PrintError::print("Trace", "received wrong packet from this server.", gpkt->getDsID());
		return INVALID;
	}

	// Mark: Received one packet, and increment the offset of the data on that server.
	dataSizeInWindow[serverindex] = SW_RECEIVED;
	dsoffsets[serverindex] += sentPktSize[serverindex];

	// Check if all the slots in the window are done.
	for(int i = 0; i < layout->getServerNum(); i ++){
		if(dataSizeInWindow[i] > 0 || dataSizeInWindow[i] == SW_SENT)
			return NEED_WAIT; // This window is undone.
	}

	// This window is done.
	if(unProcessedSize == 0){ // This trace is all done.
		return ALL_DONE;
	}

	// Still have more windows.
	return MORE_TO_SEND;
}

void WindowBasedTrace::setLayout(Layout * lo) {
	layout = lo;
	// Set the correct offset on each data server.
	long long jumps = offset / layout->getWindowSize();
	long long remainder = offset % layout->getWindowSize(); // can be big
	for(int i = 0; i < layout->getServerNum(); i ++){
		dsoffsets[i] = jumps * (long long)(layout->getServerStripeSize(i));
		if(remainder != -1){
			if(remainder <= layout->getServerStripeSize(i)){ // Starts from here
				offset_start_server = i;
				offset_start_position = remainder;
				dsoffsets[i] += remainder;
				remainder = -1;
			}
			else{
				dsoffsets[i] += (long long)(layout->getServerStripeSize(i));
				remainder -= (long long)(layout->getServerStripeSize(i));
			}
		}
	}

	// Initialize the data chunk size for each data server with NULL first.
	maxWindowSize = 0;
	for(int i = 0; i < layout->getServerNum(); i ++) {
		dataSizeInWindow[i] = SW_NULL;
		maxWindowSize += layout->getServerStripeSize(i);
	}

	// Set the first window.
	openNewWindow();
}

WindowBasedTrace::~WindowBasedTrace() {
}
