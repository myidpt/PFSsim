/*
 * General.h
 *
 *  Created on: Apr 21, 2010
 *      Author: yonggang
 */

#ifndef GENERAL_H_
#define GENERAL_H_
#include "client/Clientspecs.h"
#include "server/DServerspecs.h"
#include "scheduler/DSschedulerspecs.h"
#include "packet/GPacket_m.h"
#include "packet/SPacket_m.h"
#include "packet/QPacket_m.h"
#include "utility/Cache.h"

#define SMALLDBL 0.000001

#define MAX_TIME 36000 // The time limit on the simulation.

#define REQ_MAXSIZE 0x40000000 // According to PVFS 0x40000000, 1GB
#define JOB_MAXSIZE 0xa00000 // According to PVFS 0xa00000, 10MB
#define LOWOFFSET_RANGE 0x80000000 // offset = lowoffset + lowoffset_range * highoffset

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
#define W_CACHE_JOB_DONE	17 // When the job is done by committing to the cache.
#define W_DISK_JOB_DONE		18
#define R_CACHE_JOB_DONE	19
#define R_DISK_JOB_DONE		20

// Schedule mechanisms
#define UNSCHEDULED -1
#define PVFS_SYS_LAYOUT_NONE 1
#define PVFS_SYS_LAYOUT_ROUND_ROBIN 2
#define PVFS_SYS_LAYOUT_RANDOM 3
#define PVFS_SYS_LAYOUT_LIST 4

#define DEBUG

#endif /* GENERAL_H_ */
