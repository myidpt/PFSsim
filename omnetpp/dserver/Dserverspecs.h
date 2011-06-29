/*
 * DServerspecs.h
 *
 *  Created on: Aug 16, 2010
 *      Author: yonggang
 */

#ifndef DSERVERSPECS_H_
#define DSERVERSPECS_H_
#include "../client/Clientspecs.h"

// General
#define DS_TOTAL 4
#define MAX_DS_NUM 1024

// Data server daemon
#define DSD_NEWJOB_PROC_TIME 0.000001
#define DSD_FINJOB_PROC_TIME 0.000001
#define DSD_DEGREE 4

// Local file system
#define PAGESIZE 4096
#define MAX_PAGENUM 1024*256
#define LOCALFS_DEGREE	4
#define CACHE_R_SPEED	0.0000001 // The time cost of reading a page in cache.
#define CACHE_W_SPEED	0.0000002 // The time cost of writing a page in cache.
#define MAX_PR_PER_REQ 1000 // The maximum number of disk access page-ranges a request will incur. Internal use.

// Disk
#define DISK_ID_BASE (OMNET_CID_BASE+C_TOTAL+DS_TOTAL*3)
#define DISK_DEGREE 1
#define APP_DATA_RANGE (1024*1024*4) // set to be 4M pages (Data size = 4M pages * 4KB/page = 16GB).
#define BASE_PORT 8800

#endif /* DSERVERSPECS_H_ */
