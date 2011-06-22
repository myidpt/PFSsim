/*
 * DServerspecs.h
 *
 *  Created on: Aug 16, 2010
 *      Author: yonggang
 */

#ifndef DSERVERSPECS_H_
#define DSERVERSPECS_H_
#include "../client/Clientspecs.h"

#define DS_TOTAL 4
#define MAX_DS_NUM 1024
#define BASE_PORT 8800
#define DS_DAEMON_DELAY 0
#define DS_DAEMON_DEGREE 4
#define DISK_ID_BASE (C_ID_BASE+C_TOTAL+DS_TOTAL*3)
#define DISK_DEGREE 1
#define APP_DATA_RANGE (1024*1024*4) // set to be 4M pages (4M*4MB=16MB).
#define PAGESIZE 4096
#define MAXPAGENUM 1024*256
#define MAXPRPERREQ 1000 // The maximum number of disk access page-ranges a request will incur.

#define MAXWBUFFSIZE 410624 // KB, 401 * 1M
#define WRITTENBACKSIZE 5120 //KB, 5 M, got from stat.
#define MAXRCACHESIZE 0 // KB, 802 * 1M
#define WBUFF_TIME 0.00018 // For each KB!
#define WDISK_SIZE 22000
#define RBUFF_TIME 0.000078 // For each KB!
#define RDISK_SIZE 110
#define RDISK_TIME 0.000095 // For each KB!

#define LOCALFS_DEGREE	4
#define CACHE_R_SPEED	0.0000001
#define CACHE_W_SPEED	0.0000002

#endif /* DSERVERSPECS_H_ */
