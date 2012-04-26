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

#ifndef DISK_H_
#define DISK_H_
#include "General.h"
#include "scheduler/FIFO.h"

#define JUMPSIZECOUNT	22
#define BLKSIZECOUNT	9 // 4K -> 1M
//#define ZONECOUNT	8
#define BASEBLKSIZE		4096

class Disk : public cSimpleModule{
protected:
	long int diskSize; // In the unit of 4096B block.
//	long int zoneSize; // The size of each zone.
    int degree;
	int myID;
	int portno;
	int outstanding;
	FIFO * queue;

	long last_offset; // The offset of the last access (Consider 4096 as the unit).
	long last_jump;
	double last_access_time;
	int seq_read_sofar; // Serves for the read-ahead functionality. RA is triggered if seq_read_sofar hits some point.
	double return_zero_period; // If the idle time exceeds this bound, the disk head will be moved to other locations.

	long jumpsizes[JUMPSIZECOUNT];
	long blksizes[BLKSIZECOUNT];
	double jumptime[2][BLKSIZECOUNT][JUMPSIZECOUNT]; // The first dimension is for read / write.
	double seqtime[2][BLKSIZECOUNT];
	long ra_size; // Read-ahead at the disk drive level. A simple algorithm is implemented. If ra_size < 0, that means ra is disabled.
	int return_zero; // Return zero after every access.
	static int idInit;

	FILE * debugfp;

public:
	Disk();
	int getID();
	void initialize();
	void handleMessage(cMessage *);
	inline void handleBlockReq(BlkRequest *);
	inline void handleBlockResp(BlkRequest *);
	void dispatchJobs();
	bool checkReadahead(BlkRequest *);
	void sendSafe(BlkRequest *);
	int readParmFile(const char *);
	void finish();
	virtual ~Disk();
};

#endif /* DISK_H_ */
