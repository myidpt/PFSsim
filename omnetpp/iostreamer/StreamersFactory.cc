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

#include "StreamersFactory.h"

StreamersFactory::StreamersFactory() : FILE_OUTPUT_SIGNITURE("file"), NONE_OUTPUT_SIGNITURE("none") {

}

IPFSFileInputStreamer * StreamersFactory::createPFSFileInputStreamer
	(int num, int digits, string prefix, string postfix) {
	InputFiles * files = new InputFiles(num, digits, prefix, postfix);
	PFSFileInputStreamer * streamer = new PFSFileInputStreamer(files); // Copy by value.
	return streamer;
}

ITraceInputStreamer * StreamersFactory::createClientTraceInputStreamer
	(string prefixbeforeClientID, int clientID, int clientIDDigits,
			string prefixAfterClientID, int num, int digits, string postfix) {
	string prefix = formEntityNamePrefix(prefixbeforeClientID, clientID, clientIDDigits, prefixAfterClientID);
	InputFiles * files = new InputFiles(num, digits, prefix, postfix);
	TraceInputStreamer * streamer = new TraceInputStreamer(files);
	return streamer;
}

ITraceOutputStreamer * StreamersFactory::createClientTraceOutputStreamer
	(string prefixbeforeClientID, int clientID, int clientIDDigits,
			string prefixAfterClientID, int num, int digits, string postfix) {
	string prefix = formEntityNamePrefix(prefixbeforeClientID, clientID, clientIDDigits, prefixAfterClientID);
	OutputFiles * files = new OutputFiles(num, digits, prefix, postfix);
	TraceOutputStreamer * streamer = new TraceOutputStreamer(files);
	return streamer;
}

IPacketOutputStreamer * StreamersFactory::createDataPacketOutputStreamer
	(string prefixbefore, int entityID, int entityIDDigits,
			string prefixafter, int num, int digits, string postfix) {
	string prefix = formEntityNamePrefix(prefixbefore, entityID, entityIDDigits, prefixafter);
	OutputFiles * files = new OutputFiles(num, digits, prefix, postfix);
	DataPacketOutputStreamer * streamer = new DataPacketOutputStreamer(files);
	return streamer;
}

IPacketOutputStreamer * StreamersFactory::createMetadataPacketOutputStreamer
	(string prefixbefore, int entityID, int entityIDDigits,
		string prefixafter, int num, int digits, string postfix) {
	string prefix = formEntityNamePrefix(prefixbefore, entityID, entityIDDigits, prefixafter);
	OutputFiles * files = new OutputFiles(num, digits, prefix, postfix);
	MetadataPacketOutputStreamer * streamer = new MetadataPacketOutputStreamer(files);
	return streamer;
}

IPacketOutputStreamer * StreamersFactory::createSchedulerPacketOutputStreamer
	(int num, int digits, string prefix, string postfix) {
	OutputFiles * files = new OutputFiles(num, digits, prefix, postfix);
	SchedulerPacketOutputStreamer * streamer = new SchedulerPacketOutputStreamer(files);
	return streamer;
}

string StreamersFactory::formEntityNamePrefix(string prefix, int id, int digits, string postfix) {
	int tmpID = id;
	int IDlimit = 1;
	for (int i = 0; i < digits; i ++) {
		IDlimit *= 10;
		prefix.append("0");
	}
	// Is the digit enough for the clientID?
	if (id > IDlimit) {
		PrintError::print("StreamersFactory::createTraceInputStreamer",
				"ClientID is bigger than permitted digits.", id);
		return NULL;
	}
	// prefix: "client00"

	int lowestdigit = prefix.length() - 1;
	for (int i = 0; ; i ++) {
		prefix[lowestdigit - i] = tmpID % 10 + '0';
		tmpID /= 10;
		if (tmpID == 0) {
			break;
		}
	}
	// prefix: "client01"

	prefix.append(postfix);
	// prefix: "client01trace"

	return prefix;
}
