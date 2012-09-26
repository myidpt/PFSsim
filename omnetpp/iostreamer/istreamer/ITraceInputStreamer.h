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

#ifndef ITRACEINPUTSTREAMER_H_
#define ITRACEINPUTSTREAMER_H_

#include "trace/ITrace.h"
#include "General.h"
#include "iostreamer/istreamer/ifiles/InputFiles.h"

class ITraceInputStreamer {
public:
	ITraceInputStreamer();
	virtual bool readTrace(int index, ITrace * trace) = 0;
	virtual int getTraceFlowCount() = 0;
	virtual ~ITraceInputStreamer();
};

#endif /* ITRACEINPUTSTREAMER_H_ */
