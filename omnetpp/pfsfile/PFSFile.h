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

#ifndef PFSFILE_H_
#define PFSFILE_H_

#include "layout/Layout.h"
#include "General.h"

class PFSFile {
protected:
	int ID;
	Layout * layout;
public:
	PFSFile(int id);
	virtual int getID();
	virtual void setLayout(Layout * inputlayout);
	virtual Layout * getLayout();
	virtual bool informationIsSet() = 0;
	virtual void setFromPFSFileMetadataPacket(qPacket * packet) = 0;
	virtual qPacket * createPFSFileMetadataPacket(int id, int srcid, int dstid) = 0;
	virtual void ReplyPFSFileMetadataPacket(qPacket * packet) = 0;
	virtual ~PFSFile();
};

#endif /* ABSTRACTPFSFILE_H_ */
