/*
 * DServerspecs.h
 *
 *  Created on: Aug 16, 2010
 *      Author: yonggang
 */

#ifndef DSERVERSPECS_H_
#define DSERVERSPECS_H_

#define DS_TOTAL 4
#define MAX_DS_NUM 1024
#define DS_ID_BASE (C_ID_BASE+C_TOTAL)
#define BASE_PORT 8800
#define DEGREE 4
//#define SCH_DELAY 0.028
#define SCH_DELAY 0
#define MAXWBUFFSIZE 410624 // KB, 401 * 1M
#define WRITTENBACKSIZE 5120 //KB, 5 M, got from stat.
#define MAXRCACHESIZE 0 // KB, 802 * 1M
#define WBUFF_TIME 0.00018 // For each KB!
#define WDISK_SIZE 22000
#define RBUFF_TIME 0.000078 // For each KB!
#define RDISK_SIZE 110
#define RDISK_TIME 0.000095 // For each KB!

#endif /* DSERVERSPECS_H_ */
