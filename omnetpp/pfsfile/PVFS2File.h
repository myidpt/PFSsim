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

#ifndef PVFS2FILE_H_
#define PVFS2FILE_H_

#include "pfsfile/PFSFile.h"

class PVFS2File : public PFSFile {
public:
	PVFS2File(int id);
	bool informationIsSet();
	void setFromPFSFileMetadataPacket(qPacket * packet);
	qPacket * createPFSFileMetadataPacket(int pktid, int srcid, int dstid);
	void ReplyPFSFileMetadataPacket(qPacket * packet);
	virtual ~PVFS2File();
};

#endif /* PVFS2FILE_H_ */
