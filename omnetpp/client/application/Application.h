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

#ifndef __V0_21_APPLICATION_H_
#define __V0_21_APPLICATION_H_

#include <omnetpp.h>
#include "General.h"
#include "iostreamer/StreamersFactory.h"
#include "trace/SimpleTrace.h"


// This class counts the application trace time.
class Application : public cSimpleModule
{
protected:
	int myID;
	static int initID;

	ITraceInputStreamer * traceInput;
	ITraceOutputStreamer * traceOutput;

	// Time overhead.
	double traceProcessTime;

    inline void generateInitialTraces();
    inline void sendNewTrace(AppRequest * reqeust);
    inline void handleFinishedTrace(AppRequest * requset);
    inline void sendSafe(AppRequest * request);
    void finish();
public:
	Application();
    void initialize();
    void handleMessage(cMessage * msg);
};

#endif
