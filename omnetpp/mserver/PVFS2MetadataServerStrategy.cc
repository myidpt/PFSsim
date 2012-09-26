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

#include "PVFS2MetadataServerStrategy.h"

PVFS2MetadataServerStrategy::PVFS2MetadataServerStrategy(int id) : myID(id) {
}

string PVFS2MetadataServerStrategy::getSignature() {
	return "PVFS2";
}

void PVFS2MetadataServerStrategy::readPFSFileInformationFromFile(int num, int digits, string prefix, string postfix) {
#ifdef MS_DEBUG
	cout << "PVFS2MetadataServerStrategy::readPFSFileInformationFromFile" << endl;
#endif
	StreamersFactory streamersfactory;
	IPFSFileInputStreamer * pfsstreamer = streamersfactory.createPFSFileInputStreamer(num, digits, prefix, postfix);
	pfsFiles = pfsstreamer->readPFSFiles();
	delete pfsstreamer;
}

vector<cPacket *> * PVFS2MetadataServerStrategy::handleMetadataPacket(qPacket * packet) {
#ifdef MS_DEBUG
	cout << "PVFS2MetadataServerStrategy::handleMetadataPacket, FileID=" << packet->getFileId() << endl;
#endif
	int fileid = packet->getFileId();

	PFSFile * pfsfile = pfsFiles->findPFSFile(fileid);
	if(pfsfile == NULL){
		PrintError::print("PVFS2MetadataServerStrategy::handleMetadataPacket",
				"Information of this file ID is not defined in the input file.", fileid);
	}
	pfsfile->ReplyPFSFileMetadataPacket(packet); // Set the reply packet, reuse the object.

	packet->setByteLength(METADATA_LENGTH); // Assume query reply length is METADATA_LENGTH.
	packet->setKind(LAYOUT_RESP);
	vector<cPacket *> * retList = new vector<cPacket *>();
	retList->push_back(packet);
	return retList;
}

vector<cPacket *> * PVFS2MetadataServerStrategy::handleDataPacketReply(gPacket * packet) {
	return new vector<cPacket *>();
}

PVFS2MetadataServerStrategy::~PVFS2MetadataServerStrategy() {
}
