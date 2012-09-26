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

#ifndef STREAMERSFACTORY_H_
#define STREAMERSFACTORY_H_

#include "iostreamer/istreamer/ifiles/InputFiles.h"
#include "iostreamer/istreamer/PFSFileInputStreamer.h"
#include "iostreamer/istreamer/TraceInputStreamer.h"
#include "iostreamer/ostreamer/ofiles/OutputFiles.h"
#include "iostreamer/ostreamer/DataPacketOutputStreamer.h"
#include "iostreamer/ostreamer/MetadataPacketOutputStreamer.h"
#include "iostreamer/ostreamer/SchedulerPacketOutputStreamer.h"
#include "iostreamer/ostreamer/TraceOutputStreamer.h"

class StreamersFactory {
public:
	const string FILE_OUTPUT_SIGNITURE;
	const string NONE_OUTPUT_SIGNITURE;

	StreamersFactory();
	IPFSFileInputStreamer * createPFSFileInputStreamer(int num, int digits, string prefix, string postfix);

	ITraceInputStreamer * createClientTraceInputStreamer(string prefixbeforeClientID, int clientID,
			int clientIDDigits, string prefixAfterClientID, int num, int digits, string postfix);
	ITraceOutputStreamer * createClientTraceOutputStreamer(string prefixbeforeClientID, int clientID,
			int clientIDDigits,	string prefixAfterClientID, int num, int digits, string postfix);

	IPacketOutputStreamer * createDataPacketOutputStreamer(string prefixbefore, int entityID,
			int entityIDDigits, string prefixafter, int num, int digits, string postfix);
	IPacketOutputStreamer * createMetadataPacketOutputStreamer(string prefixbefore, int entityID,
			int entityIDDigits, string prefixafter, int num, int digits, string postfix);
	IPacketOutputStreamer * createSchedulerPacketOutputStreamer(int num, int digits, string prefix,
			string postfix);
protected:
	inline string formEntityNamePrefix(string prefix, int id, int digits, string postfix);
};

#endif /* STREAMERSFACTORY_H_ */
