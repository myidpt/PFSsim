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

#ifndef DATAPACKETOUTPUTSTREAMER_H_
#define DATAPACKETOUTPUTSTREAMER_H_

#include "IPacketOutputStreamer.h"
#include "General.h"
#include "iostreamer/ostreamer/ofiles/OutputFiles.h"

using namespace std;

class DataPacketOutputStreamer : public IPacketOutputStreamer {
protected:
	const int SecToMS;
	const int MaxLineLength;
	OutputFiles * outputFiles;
	int fileCount;
public:
	DataPacketOutputStreamer(OutputFiles * files);
	void writePacket(const cPacket * packet);
	~DataPacketOutputStreamer();
};

#endif /* DATAPACKETOUTPUTSTREAMER_H_ */
