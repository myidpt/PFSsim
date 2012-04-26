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

Layout::Layout(int fid) {
	fileId = fid;
	setWindowSize(-1);
	setServerNum(0);
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
int const Layout::getFileID(){
	return fileId;
}
long const Layout::getWindowSize(){
	if(windowSize == -1)
		calculateWindowSize();
	return windowSize;
}
int const Layout::getServerNum(){
	return serverNum;
}
int const Layout::getServerID(int index){
	// assert: index < serverNum
	if(index >= serverNum)
		return 0;
	return serverList[index];
}
long const Layout::getServerStripeSize(int index){
	// assert: index < serverNum
	if(index >= serverNum)
		return 0;
	return serverStripeSizes[index];
}
int const Layout::findServerIndex(int id){
	for(int i = 0; i < serverNum; i ++)
		if(serverList[i] == id)
			return i;
	return -1;
}
void const Layout::calculateWindowSize(){
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

