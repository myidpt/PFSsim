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

#include "PVFS2File.h"

PVFS2File::PVFS2File(int id) : PFSFile(id) {
}

bool PVFS2File::informationIsSet() {
	if (layout == NULL) {
		return false;
	}
	else {
		return true;
	}
}

void PVFS2File::setFromPFSFileMetadataPacket(qPacket * packet) {
	if (layout == NULL) {
		layout = new Layout();
	}
	layout->setLayout(packet);
}

qPacket * PVFS2File::createPFSFileMetadataPacket(int pktid, int srcid, int dstid){
	qPacket * qpkt = new qPacket("qpacket");
	qpkt->setID(pktid);// Important, otherwise the response packet won't know where to be sent.
	qpkt->setFileId(ID);
	qpkt->setKind(LAYOUT_REQ);
	qpkt->setByteLength(DATA_HEADER_LENGTH); // Set the length of packet
	qpkt->setClientID(srcid);
	qpkt->setMetadataServerID(dstid);
	return qpkt;
}

void PVFS2File::ReplyPFSFileMetadataPacket(qPacket * packet) {
	if (layout == NULL) {
		PrintError::print("PVFS2File::ReplyPFSFileMetadataPacket", "Layout is NULL.");
	}
	layout->setqPacket(packet);
}

PVFS2File::~PVFS2File() {

}
