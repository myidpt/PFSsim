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

#ifndef EXT3_H_
#define EXT3_H_

#include "dserver/lfs/ILFS.h"

class EXT3 : public ILFS{
protected:
	struct br_t { // block range type. unit: page. This is the block range seen by this file.
		long reqid;
		long reqsubid;

		// File space view:
		long long start;
		long long end; // actually "end" block will not be accessed.

		// Physical disk space view:
		long long extindex; // Keeps track of the index of the extent being accessed.
		long long extoff; // Keeps track of the offset inside the extent.

		br_t(long, long, long long, long long);
	};

//	char * extmap[MAX_LFILE_NUM]; // Bit-map for the meta-data of each extent, keeping track if they are accessed.
	int outstanding; // Number of outstanding requests dispatched.

	// Note: the following two variables are simplified version. Should implement stochastic model to allocate the disk space.
	int new_ext_size; // The size of each newly created extent.
	int new_ext_gap; // The gap size between each newly created extent.

	map<PageRequest *, br_t *> * brs; //  The block-ranges which record the status of all queued requests.

public:
	EXT3(int, int, long long, int, int, const char *, const char *, int, int);
	void newReq(PageRequest *);
	BlkRequest * dispatchNext();
	PageRequest * finishedReq(BlkRequest *);
	~EXT3();
};

#endif /* EXT3_H_ */
