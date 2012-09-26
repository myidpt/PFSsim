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

#ifndef TRACEINPUTSTREAMER_H_
#define TRACEINPUTSTREAMER_H_

#include "iostreamer/istreamer/ifiles/InputFiles.h"
#include "ITraceInputStreamer.h"

#define MAX_TRACE_PER_CLIENT 100

class TraceInputStreamer : public ITraceInputStreamer {
protected:
	InputFiles * inputFiles;
	int traceID;
public:
	TraceInputStreamer(InputFiles * files);
	bool readTrace(int index, ITrace * trace);
	int getTraceFlowCount();
	~TraceInputStreamer();
};

#endif /* TRACEINPUTSTREAMER_H_ */
