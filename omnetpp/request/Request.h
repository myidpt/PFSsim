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

#include "packet/GPacket_m.h"

#include "packet/QPacket_m.h"

#include "../General.h"
#include "../layout/Layout.h"
#define MAX_WINDOW_SIZE JOB_MAXSIZE
// This class only represents one request from the client, it may be divided into multiple packets.
class Request : public cNamedObject {
private:
	int myId;
	long long offset; // The offset (start edge) of the entire data.
	long long dsoffsets[MAX_DS_NUM]; // The offset of the data stored on each data server.
	long serverWindow[MAX_DS_NUM]; // The window for the packets waiting for reply.
	// >0: unsent (the quantity indicates the data amount), 0: sent, -1: received, -2: data not stored on this dserver.
	long size; // The total size of the data.
	long unProcessedSize; // The size of the data whose packets are still not put into the window.
	Layout * layout; // Specifies the layout of the data.
/*
 * Explanation of data transmission process.
 *             offset           index   window
 * --------------[++++++++++++++++|****************|====================]----------------
 * <- unwanted ->|<-  finished  ->|<-transmitting->|<-  not sent yet  ->|<-  unwanted  ->
 *               |<------------------------size------------------------>|
 *
 */
	int read;
	int app;
	double stime;
	double ftime;
	int sync;

	gPacket * getNextgPacketFromWindow();
//	int dsList[MAX_DS_NUM]; // the list of DServers storing this requested data.

public:
	Request(int id, double stime, long long offset, long size, int read, int app, int sync);
	virtual gPacket * nextgPacket();
	virtual int finishedgPacket(gPacket *);
	virtual void setLayout(qPacket *);
	virtual double getStarttime();
	virtual double getFinishtime();
	virtual long long getOffset();
	virtual int getId();
	virtual long getSize();
	virtual int getRead();
	virtual int getApp();
	virtual int getSync();
	virtual ~Request();
};

#endif /* REQUEST_H_ */
