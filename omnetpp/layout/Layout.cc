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

#include "Layout.h"

Layout::Layout() {
	fileID = -1;
	setWindowSize(-1);
	setServerNum(0);
}

Layout::Layout(int fid) {
	fileID = fid;
	setWindowSize(-1);
	setServerNum(0);
}

void Layout::setFileID(int fid) {
	fileID = fid;
}

void Layout::setWindowSize(long size){
	windowSize = size;
}
void Layout::setServerNum(int num){
	if(num > MAX_DS)
		serverNum = MAX_DS;
	else
		serverNum = num;
}
void Layout::setServerList(int servernum, int list[]){
	if(servernum > MAX_DS)
		serverNum = MAX_DS;
	else
		serverNum = servernum;
	for(int i = 0; i < serverNum; i ++)
		serverList[i] = list[i];
}
void Layout::setServerStripeSizes(int servernum, long size[]){
	if(servernum > MAX_DS)
		serverNum = MAX_DS;
	else
		serverNum = servernum;
	windowSize = 0;
	for(int i = 0; i < serverNum; i ++){
		serverStripeSizes[i] = size[i];
		windowSize = windowSize + size[i]; // windowSize = sum {size[0...servernum-1]}
	}
}
void Layout::setLayout(qPacket * qpkt){
	fileID = qpkt->getFileId();
	serverNum = qpkt->getDsNum();
	windowSize = 0;
	for(int i = 0; i < serverNum; i ++){
		serverList[i] = qpkt->getDsList(i);
		serverStripeSizes[i] = qpkt->getDsStripeSizes(i);
		windowSize = windowSize + serverStripeSizes[i];
	}
}
void Layout::setqPacket(qPacket * qpkt){
	qpkt->setDsNum(serverNum);
	for(int i = 0; i < serverNum; i ++){
		qpkt->setDsList(i, serverList[i]);
		qpkt->setDsStripeSizes(i, serverStripeSizes[i]);
	}
}

void Layout::setServerID(int index, int serverid){
	serverList[index] = serverid;
}
void Layout::setServerStripeSize(int index, long size){
	serverStripeSizes[index] = size;
}
int Layout::getFileID() const {
	return fileID;
}
long Layout::getWindowSize() {
	if(windowSize == -1)
		calculateWindowSize();
	return windowSize;
}
int Layout::getServerNum() const {
	return serverNum;
}
int Layout::getServerID(int index) const {
	// assert: index < serverNum
	if(index >= serverNum) {
		throw "The index is bigger than serverNum.";
	}
	return serverList[index];
}
long Layout::getServerStripeSize(int index) const {
	if(index >= serverNum)
		throw "The index is bigger than serverNum.";
	return serverStripeSizes[index];
}
int Layout::findServerIndex(int id) const {
	for(int i = 0; i < serverNum; i ++)
		if(serverList[i] == id)
			return i;
	return -1;
}
void Layout::calculateWindowSize(){
	windowSize = 0;
	for(int i = 0; i < serverNum; i ++)
		windowSize += serverStripeSizes[i];
#ifdef LAYOUT_DEBUG
	printf("calculateWindowSize == %ld, serverNum == %d, serverList[0] == %d, serverList[1] == %d,"
			" serverStripeSizes[0] == %ld, [1] == %ld.\n",windowSize, serverNum, serverList[0], serverList[1],
			serverStripeSizes[0], serverStripeSizes[1]);
#endif
}

Layout::~Layout() {
}

