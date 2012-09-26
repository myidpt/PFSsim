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

#include "ITrace.h"

ITrace::ITrace() {
}

void ITrace::initialize(int tid, double t, int pfsfid, long long off, long s, int r, int tfid, int a, int synch) {
	ID = tid;
	startTime = t;
	pfsFileID = pfsfid;
	offset = off;
	totalSize = s;

	read = r;
	traceFileID = tfid;
	applicationID = a;
	sync = synch;
}

void ITrace::initialize(const AppRequest * request) {
	ID = request->getID();
	startTime = request->getStarttime();
	pfsFileID = request->getFileID();
	offset = request->getHighoffset() * LOWOFFSET_RANGE + request->getLowoffset();
	totalSize = request->getTotalSize();
	read = request->getRead();
	traceFileID = request->getTraceFileID();
	applicationID = request->getApp();
	sync = request->getSync();
}

AppRequest * ITrace::createAppRequest() {
	AppRequest * request = new AppRequest();
	request->setID(ID);
	request->setStarttime(startTime);
	request->setFileID(pfsFileID);
	request->setHighoffset(offset / LOWOFFSET_RANGE);
	request->setLowoffset(offset % LOWOFFSET_RANGE);
	request->setTotalSize(totalSize);
	request->setRead(read);
	request->setTraceFileID(traceFileID);
	request->setApp(applicationID);
	request->setSync(sync);
	return request;
}

int ITrace::getID(){
	return ID;
}
int ITrace::getTraceFileID(){
	return traceFileID;
}
double ITrace::getStartTime(){
	return startTime;
}
double ITrace::getFinishTime(){
	return finishTime;
}
long long ITrace::getOffset(){
	return offset;
}
long ITrace::getTotalSize(){
	return totalSize;
}
int ITrace::getRead(){
	return read;
}
int ITrace::getFileID(){
	return pfsFileID;
}
int ITrace::getApplicationID(){
	return applicationID;
}
int ITrace::getSync(){
	return sync;
}

ITrace::~ITrace() {
}
