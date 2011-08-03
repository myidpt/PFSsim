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

Layout::Layout(int id) {
	appId = id;
	setWindowSize(0);
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
void Layout::setServerShares(int servernum, long size[]){
	if(servernum > MAX_DS)
		serverNum = MAX_DS;
	else
		serverNum = servernum;
	windowSize = 0;
	for(int i = 0; i < serverNum; i ++){
		serverShares[i] = size[i];
		windowSize = windowSize + size[i]; // windowSize = sum {size[0...servernum-1]}
	}
}
void Layout::setLayout(qPacket * qpkt){
	serverNum = qpkt->getDsNum();
	windowSize = 0;
	for(int i = 0; i < serverNum; i ++){
		serverList[i] = qpkt->getDsList(i);
		serverShares[i] = qpkt->getDsShares(i);
		windowSize = windowSize + serverShares[i];
	}
}
void Layout::setqPacket(qPacket * qpkt){
	qpkt->setDsNum(serverNum);
	for(int i = 0; i < serverNum; i ++){
		qpkt->setDsList(i, serverList[i]);
		qpkt->setDsShares(i, serverShares[i]);
	}
}

void Layout::setServerID(int index, int serverid){
	serverList[index] = serverid;
}
void Layout::setServerShare(int index, long size){
	serverShares[index] = size;
}
int Layout::getAppID(){
	return appId;
}
long Layout::getWindowSize(){
	return windowSize;
}
int Layout::getServerNum(){
	return serverNum;
}
int Layout::getServerID(int index){
	// assert: index < serverNum
	if(index >= serverNum)
		return 0;
	return serverList[index];
}
long Layout::getServerShare(int index){
	// assert: index < serverNum
	if(index >= serverNum)
		return 0;
	return serverShares[index];
}
int Layout::findServerIndex(int id){
	for(int i = 0; i < serverNum; i ++)
		if(serverList[i] == id)
			return i;
	return -1;
}
void Layout::calculateWindowSize(){
	for(int i = 0; i < serverNum; i ++)
		windowSize += serverShares[i];
}

Layout::~Layout() {
}

