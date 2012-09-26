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

#ifndef PVFS2METADATASERVERSTRATEGY_H_
#define PVFS2METADATASERVERSTRATEGY_H_

#include <math.h>
#include <omnetpp.h>
#include "General.h"
#include "mserver/PFSMetadataServerStrategy.h"
#include "layout/Layout.h"
#include "iostreamer/StreamersFactory.h"
#include "iostreamer/istreamer/PFSFileInputStreamer.h"

class PVFS2MetadataServerStrategy : public PFSMetadataServerStrategy {
protected:
	int myID;
	PFSFiles * pfsFiles;
public:
	PVFS2MetadataServerStrategy(int id);
	string getSignature();
	void readPFSFileInformationFromFile(int num, int digits, string prefix, string postfix);
	vector<cPacket *> * handleMetadataPacket(qPacket * packet);
	vector<cPacket *> * handleDataPacketReply(gPacket * packet);
	virtual ~PVFS2MetadataServerStrategy();
};

#endif /* PVFS2METADATASERVERSTRATEGY_H_ */
