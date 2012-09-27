/*
 * General.h
 *
 *  Created on: Apr 21, 2010
 *      Author: yonggang
 */

#ifndef GENERAL_H_
#define GENERAL_H_
#include <stdio.h>
#include <stdlib.h>

#define MAX_FILE 1024 		// The maximum number of files.
#define MAX_APP 256 		// The maximum number of applications.
#define MAX_DS 64
#define MAX_APP_PER_C		100
#define TRC_MAXSIZE			0x3DE00000 // According to PVFS 0x40000000, 990MB
//#define JOB_MAXSIZE		0xa00000 // According to PVFS 0xa00000, 10MB, one trace can have 99 JOBs
//#define FLOW_MAXSIZE		0x40000 // According to PVFS 0x40000, 256KB
#define JOB_MAXSIZE			0x40000 // According to PVFS 0x40000, 256KB
#define MAX_TIME 			360000 // The time limit on the simulation.

#define CID_OFFSET			1000000 // The offset of the Client ID.
#define RID_OFFSET			1000 // The offset of the request ID.
#define SUBREQ_OFFSET		1	// The offset of the sub-request from DSD.
#define LOWOFFSET_RANGE 	0x10000000 // offset = lowoffset + lowoffset_range * highoffset
#define MAX_DISK_OFFSET 	1000000000 // Limit to 4000 GB total disk size.
#define MAX_LFILE_NUM		1024 // Max number of files in each local file system.
#define MAX_EXT_ENTRY_NUM	10000 // Max number of entries in the ext_list

//#define MAX_SUBREQ_SIZE 	65536 // 64KB
#define MAX_SUBREQ_SIZE 	131072 // 128KB
//#define MAX_SUBREQ_SIZE 	262144 // 256KB
//#define MAX_SUBREQ_SIZE 	524288 // 512KB
#define MAX_RA						64
#define RA_FLAG_INCACHE_SEILING		256

// These sub-request ID bases are used for marking the sub-requests from the VFS layer.
#define SUB_ID_BASE			100
#define VFS_SUB_ID_BASE		10
#define LFS_SUB_ID_BASE		1

#define DATA_HEADER_LENGTH 	48 // In the packet, in byte.
#define METADATA_LENGTH 	2048 // For the packet, in byte.

// For messages types
#define SELF_EVENT			0
#define LAYOUT_REQ 			1
#define LAYOUT_RESP 		2
#define LFILE_REQ			3
#define BLK_REQ				4
#define BLK_RESP			5
#define LFILE_RESP			6
#define SCH_JOB				7
#define TRC_SYN				8
#define PAGE_REQ			9
#define PAGE_RESP			10
#define PROP_SCH			11
#define DISK_REQ			12
#define DISK_RESP			13
#define CACHE_ACCESS		14
#define DIRTY_PAGE_EXPIRE	15
#define ALG_TIMER			16
#define TRACE_REQ			17
#define TRACE_RESP			18
#define PFS_W_REQ			19
#define PFS_R_REQ			20
#define PFS_W_RESP			21
#define PFS_W_DATA			22
#define PFS_W_DATA_LAST			23
#define PFS_W_FIN				24
#define PFS_R_DATA				25
#define PFS_R_DATA_LAST			26
#define SELF_PFS_W_REQ			27
#define SELF_PFS_R_REQ			28
#define SELF_PFS_W_RESP			29
#define SELF_PFS_W_DATA			30
#define SELF_PFS_W_DATA_LAST	31
#define SELF_PFS_W_FIN			32
#define SELF_PFS_R_DATA			33
#define SELF_PFS_R_DATA_LAST	34





#define M_DIRTYPAGE			50
#define M_CACHEDPAGE		51

// Schedule mechanisms
#define UNSCHEDULED 					-1
#define PVFS_SYS_LAYOUT_NONE			1
#define PVFS_SYS_LAYOUT_ROUND_ROBIN 	2
#define PVFS_SYS_LAYOUT_RANDOM 			3
#define PVFS_SYS_LAYOUT_LIST 			4

#define FIFO_ALG			0
#define SFQ_ALG				1
#define DSFQA_ALG			2
#define DSFQD_ALG			3
#define DSFQF_ALG			4
#define SSFQ_ALG			5
#define DSFQATB_ALG			6
#define DSFQALB_ALG			7

#define DISKCACHE_TRIMCACHE // This will shrink the cache's doubly-linked list by merging adjacent elements in address. It improves performance.
//#define DISKCACHE_CHECKHEALTH // This checks the cache doubly-linked list health every time; it down grades performance greatly.

//#define TRACE_PKT_STAT // Print the packet statistic information in the output files.
//#define VERBOSE

//#define APP_DEBUG
//#define PFSCLIENT_DEBUG
//#define LAYOUT_DEBUG
//#define TRACE_DEBUG
//#define MS_DEBUG
//#define DSD_DEBUG
//#define PVFS2DSD_DEBUG
//#define VFS_DEBUG
//#define DISKCACHE_DEBUG
//#define DISKCACHE_RA_DEBUG
//#define LFS_DEBUG
//#define DISK_DEBUG
//#define PROXY_DEBUG
//#define SCH_DEBUG

//#define SCH_PRINT
//#define MONITOR_DIRTYPAGE
//#define MONITOR_CACHEDPAGE
//#define MONITOR_DISKACCESS

#define MONITOR_INT 		0.4

//#define DEBUG
#ifdef DEBUG
#define C_DEBUG
#define LAYOUT_DEBUG
#define TRACE_DEBUG
#define MS_DEBUG
#define PROXY_DEBUG
#define DSD_DEBUG
#define PVFS2DSD_DEBUG
#define VFS_DEBUG
#define DISKCACHE_DEBUG
#define DISKCACHE_RA_DEBUG
#define LFS_DEBUG
#define DISK_DEBUG
#endif

//#define MSG_CLIENT
//#define MSG_MSERVER
//#define MSG_ROUTER
//#define MSG_DSERVER
//#define MSG_PROXY


#define PVFS2_SIGNATURE "PVFS2"
#define LUSTRE_SIGNATURE "Lustre"
#define PANFS_SIGNATURE "PanFS"
#define CEPH_SIGNATURE "Ceph"

#include "packet/Packets_m.h"
#include "util/PrintError.h"
#include "util/MessageKind.h"
#include "scheduler/IQueue.h"

#define SMALLDBL 			0.000001

#endif /* GENERAL_H_ */
