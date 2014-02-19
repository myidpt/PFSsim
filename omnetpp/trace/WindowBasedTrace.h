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

#ifndef WINDOWBASEDTRACE_H_
#define WINDOWBASEDTRACE_H_

#include "General.h"
#include "trace/ITrace.h"
#include "layout/Layout.h"


#define MAX_WINDOW_SIZE JOB_MAXSIZE

// This class only represents one request from the client, it may be divided into multiple packets.
class WindowBasedTrace : public ITrace {
private:
	const static int SW_SENT; // 0: The data request is sent.
	const static int SW_RECEIVED; // -1: For the server window: received the reply for this request.
	const static int SW_NULL; // -2: This data server slot does not need to be accessed.

	Layout * layout;

	long long dsoffsets[MAX_DS]; // The offset of the data stored on each data server.
	long dataSizeInWindow[MAX_DS]; // The window for the packets waiting for reply.
	// >0: unsent (the quantity indicates the data amount) / SW_SENT / SW_RECEIVED / SW_NULL
	// other options are defined in SW_*.
	long sentPktSize[MAX_DS];
	// You need to record the size of packets sent out, because the one comes back may not have the same size as you sent.

	long unProcessedSize; // The size of the data whose packets are still not put into the window.
	long aggregateSize; // The aggregate size of the current window.
	int offset_start_server; // To calculate the remainder.
	long offset_start_position;
	int totalSubreqsInWindow; // Record the number of subreqs in the current window.
/*
 * Explanation of data transmission process.
 *             offset           index    window
 * --------------[++++++++++++++++|*****************|====================]----------------
 * <- unwanted ->|<-  finished  ->|<--transmitting->|<-  not sent yet  ->|<-  unwanted  ->
 *                                |<-aggregateSize->|<--unProcessedSize->|
 *               |<---------------------totalSize----------------------->|
 *
 */

	inline gPacket * getNextgPacketFromWindow();
	inline bool openNewWindow();
//	int dsList[MAX_DS_NUM]; // the list of DServers storing this requested data.

public:
	enum status {
		INVALID,
		NEED_WAIT,
		MORE_TO_SEND,
		ALL_DONE
	};
	WindowBasedTrace();

	void initialize(int id, double stime, int fid, long long offset, long size, int read, int trcid, int app,
			int sync);
	void initialize(const AppRequest * request);

	void setLayout(Layout * layout);

	gPacket * nextgPacket();
	status finishedgPacket(const gPacket * gpkt);

	virtual ~WindowBasedTrace();
};

#endif /* WINDOWBASEDTRACE_H_ */
