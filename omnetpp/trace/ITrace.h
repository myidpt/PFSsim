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

#ifndef ITRACE_H_
#define ITRACE_H_

#include "General.h"

class ITrace {
protected:
	int ID; // Trace ID.
	long long offset; // The offset (start edge) of the entire data.

	long totalSize; // The total size of the data.
	int read;
	int applicationID;
	double startTime;
	double finishTime;
	int sync;
	int traceFileID; // Input file ID.
	int pfsFileID; // PFS file ID.
public:
	ITrace();
	virtual void initialize(int id, double stime, int fid, long long offset, long size, int read, int trcid, int app,
			int sync);
	virtual void initialize(const AppRequest * request);
	virtual AppRequest * createAppRequest();
    int getID();
    int getTraceFileID();
    double getStartTime();
    double getFinishTime();
    long long getOffset();
    long getTotalSize();
    int getRead();
    int getApplicationID();
    int getFileID();
	int getSync();
	~ITrace();
};

#endif /* ITRACE_H_ */
