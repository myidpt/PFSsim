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

#include "TraceOutputStreamer.h"

TraceOutputStreamer::TraceOutputStreamer(OutputFiles * files)
: SecToMS(1000), MaxLineLength(150), outputFiles (files) {

}

void TraceOutputStreamer::writeTrace(ITrace * trace) {
	char buff[MaxLineLength];
	/*
	snprintf(buff, MaxLineLength, "TIME=%lf, ID=%d FileID=%d R=%d OFFSET=%lld SIZE=%ld TT=%lf",
	        trace->getStartTime(),
			trace->getID(),
			trace->getFileID(),
			trace->getRead(),
			trace->getOffset(),
			trace->getTotalSize(),
			trace->getFinishTime() - trace->getStartTime());
    */
	snprintf(buff, MaxLineLength, "%lf,%lf,%d",
            trace->getStartTime(),
            trace->getFinishTime() - trace->getStartTime(),
            trace->getFileID());
	outputFiles->writeLine(trace->getTraceFileID(), buff);
}

TraceOutputStreamer::~TraceOutputStreamer() {
	if (outputFiles != NULL) {
		delete outputFiles;
	}
}
