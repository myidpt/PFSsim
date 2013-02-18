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

#ifndef PROXY_H_
#define PROXY_H_
#include "General.h"

#include "scheduler/FIFO.h"
#include "scheduler/SFQ.h"
#include "scheduler/SSFQ.h"
#include "scheduler/DSFQA.h"
#include "scheduler/DSFQD.h"
#include "scheduler/DSFQF.h"
#include "scheduler/DSFQALB.h"
#include "scheduler/DSFQATB.h"
#include "scheduler/SFQRC.h"

class Proxy : public cSimpleModule{
protected:
	static int proxyID;
	int myID;
	int algorithm;
	int degree;
	double newjob_proc_time;
	double finjob_proc_time;
	IQueue * queue;
	gPacket * alg_timer;
	double alg_prop_int;
public:
	Proxy();
	void initialize();
	void handleMessage(cMessage *);

	inline void handleJobReq(gPacket *);
	inline void handleJobReq2(gPacket *);

	inline void handleReadLastWriteFin(gPacket *);
	inline void handleReadLastWriteFin2(gPacket *);

	inline void handleMinorReadWrite(gPacket *);

	inline void handleAlgorithmTimer();

	inline void scheduleJobs();
	inline void sendSafe(gPacket *);

	void handleInterSchedulerPacket(sPacket *);
	void propagateSPackets();
	void finish();
	virtual ~Proxy();
};

#endif /* PROXY_H_ */
