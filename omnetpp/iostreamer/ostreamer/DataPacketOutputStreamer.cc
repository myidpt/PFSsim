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

#include "DataPacketOutputStreamer.h"

DataPacketOutputStreamer::DataPacketOutputStreamer(OutputFiles * files)
: SecToMS(1000), MaxLineLength(150), outputFiles(files) {
	fileCount = outputFiles->fileCount();
}

void DataPacketOutputStreamer::writePacket(const cPacket * cpacket) {
	const gPacket * packet = (gPacket *)cpacket;
	char buff[MaxLineLength];
	snprintf(buff, MaxLineLength, "RiseTime=%lf $ID=%ld: APP=%d FID=%d"
	        " START=%ld\tSIZE=%d\tSVR=%d\tS=%lf T=%lf",
			packet->getRisetime(),
	        packet->getID(),
			packet->getApp(),
			packet->getFileId(),
			packet->getLowoffset(),
			packet->getSize(),
			packet->getDsID(),
			SecToMS*(packet->getFinishtime() - packet->getDispatchtime()),
			// Note: Read packets may not have risetime
			// because they are generated on the server side.
			SecToMS*(packet->getReturntime() - packet->getRisetime()));
	int index = 0;
	if(fileCount == 1) {
		index = 0;
	} else {
		index = packet->getClientID();
	}
	outputFiles->writeLine(index, string(buff));
}

DataPacketOutputStreamer::~DataPacketOutputStreamer() {
	if (outputFiles != NULL) {
		delete outputFiles;
	}
}
