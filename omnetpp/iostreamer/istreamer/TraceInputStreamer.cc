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

#include "TraceInputStreamer.h"

TraceInputStreamer::TraceInputStreamer(InputFiles * files) : inputFiles(files) {
	if (files->fileCount() > MAX_TRACE_PER_CLIENT) {
		PrintError::print("TraceInputStreamer::TraceInputStreamer", "Input file number is bigger than upperbound ",
				files->fileCount());
		return;
	}
	traceID = 1;
}

// Return is a boolean notifying if the trace is gotten. If false, EOF is met.
// Index is the trace file ID / process ID.
bool TraceInputStreamer::readTrace(int index, ITrace * trace) {
	double time;
	long long offset;
	int size;
	int read;
	int fileid;
	int appid;
	int sync;

	string line;
	while(1){
		if ( ! inputFiles->readLine(index, line) ) {
			cout << "Trace #" << index << ": Reach the end of the trace file." << endl;
			return false;
		}
		else {
			break;
		}
	}

	sscanf(line.c_str(), "%lf %d %lld %d %d %d %d",
			&time, &fileid, &offset, &size, &read, &appid, &sync);

	if(size >= TRC_MAXSIZE) {
		PrintError::print("TraceInputStreamer::readTrace", "Trace size is bigger than TRC_MAXSIZE", TRC_MAXSIZE);
	}

	trace->initialize(traceID++, time, fileid, offset, size, read, index, appid, sync);
	return true;
}

int TraceInputStreamer::getTraceFlowCount() {
	return inputFiles->fileCount();
}

TraceInputStreamer::~TraceInputStreamer() {
	if (inputFiles != NULL) {
		delete inputFiles;
	}
}
