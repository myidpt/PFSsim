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

#ifndef ILFS_H_
#define ILFS_H_

#include <stdio.h>
#include <omnetpp.h>
#include "General.h"
using namespace std;

class ILFS {
private:
    void initExtLists();
    void printExtLists();
protected:
	int myId;
	int degree;
	int page_size;
	int blk_size;
	long long disk_size;
	list<PageRequest *> * pageReqQ;
//	long long spaceallocp; // Disk space allocate pointer.

	FILE * ifp;
	FILE * ofp;

    struct ext_t{ // Note that this extent is in the unit of a block size (typically 4K).
        long logistart; // Logical start address of this extent.
        long phystart; // Physical start address of this extent.
        int length; // The length of this extent.
        char accessed; // Indicates if this extent was accessed before
    };

//	long long * extsl[MAX_LFILE_NUM]; //  Meta-data list, keeps the start location of each each extent.
//	int * extll[MAX_LFILE_NUM]; //  Meta-data list, keeps the length of each each extent.
	ext_t * extlist[MAX_LFILE_NUM];
	int extentryNum[MAX_LFILE_NUM]; // Used to count the entry number of each element in extlist.

	int findExtEntry(int fid, long logiaddress);
public:
	ILFS(int, int, long long, int, int, const char *, const char *);
	virtual void newReq(PageRequest *) = 0;
	virtual BlkRequest * dispatchNext() = 0;
	virtual PageRequest * finishedReq(BlkRequest *) = 0;
	virtual ~ILFS();
};

#endif /* ILFS_H_ */
