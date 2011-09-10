/*
 * General.h
 *
 *  Created on: Apr 21, 2010
 *      Author: yonggang
 */

#ifndef GENERAL_H_
#define GENERAL_H_
#include "packet/Packets_m.h"

#define SMALLDBL 0.000001

#define MAX_FILE 1024 // The maximum number of files.
#define MAX_APP 256 // The maximum number of applications.
#define MAX_DS 64
#define TRC_MAXSIZE 0x40000000 // According to PVFS 0x40000000, 1GB
#define JOB_MAXSIZE 0xa00000 // According to PVFS 0xa00000, 10MB
#define MAX_TIME 36000 // The time limit on the simulation.

#define CID_OFFSET_IN_PID 10000 // The offset of the Client ID in the packet IDs.
#define LOWOFFSET_RANGE 0x800000 // offset = lowoffset + lowoffset_range * highoffset
#define MAX_DISK_OFFSET 1000000 // Limit to 4 GB total size.


// For messages types
#define SELF_EVENT	0
#define LAYOUT_REQ 	1
#define LAYOUT_RESP 2
#define JOB_REQ 	3
#define JOB_DISP	4 // This job is out of scheduler now
#define LFILE_REQ	5
#define BLK_REQ		6
#define BLK_RESP	7
#define LFILE_RESP	8
#define JOB_FIN		9
#define JOB_RESP 	10
#define SCH_JOB		11
#define TRC_SYN		12
#define PAGE_REQ	13
#define PAGE_RESP	14
#define DISK_REQ	17
#define DISK_RESP	18
#define W_CACHE_JOB_DONE	19 // When the job is done by committing to the cache.
#define W_DISK_JOB_DONE		20
#define R_CACHE_JOB_DONE	21
#define R_DISK_JOB_DONE		22

// Schedule mechanisms
#define UNSCHEDULED -1
#define PVFS_SYS_LAYOUT_NONE 1
#define PVFS_SYS_LAYOUT_ROUND_ROBIN 2
#define PVFS_SYS_LAYOUT_RANDOM 3
#define PVFS_SYS_LAYOUT_LIST 4

#define FIFO_ALG 0
#define SFQ_ALG 1

//#define VERBOSE
//#define DEBUG


#endif /* GENERAL_H_ */
