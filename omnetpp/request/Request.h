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

#ifndef REQUEST_H_
#define REQUEST_H_
#include "General.h"
#define MAX_WINDOW_SIZE JOB_MAXSIZE
// This class only represents one request from the client, it may be divided into multiple packets.
class Request : public cNamedObject {
private:
	long long offset;
	long windowsizeInByte; // Pre-determined. Don't change in the middle.
	int window[MAX_DS_NUM]; // The window for the packets waiting for reply. 0: unsent, 1: sent, 2: received.
	int windowSize; // number of DServer storing this requested data.
	long long index; // current offset in the data, before which you have already finished.
/*
 * Explanation of data transmission process.
 *             offset           index   window
 * --------------[++++++++++++++++|****************|====================]----------------
 * <- unwanted ->|<-  finished  ->|<-transmitting->|<-  not sent yet  ->|<-  unwanted  ->
 *
 */
	int read;
	int app;
	double stime;
	double ftime;
	long size;
	int sync;

	int dsList[MAX_DS_NUM]; // the list of DServers storing this requested data.

public:
	Request(double stime, long long offset, long size, int read, int app, int sync);
	virtual gPacket * nextgPacket();
	virtual int finishedgPacket(gPacket *);
	virtual void setLayout(qPacket *);
	double getStarttime();
	double getFinishtime();
	long long getOffset();
	long getSize();
	int getRead();
	int getApp();
	int getSync();
	virtual ~Request();
};

#endif /* REQUEST_H_ */
