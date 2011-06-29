/*
 * Clientspec.h
 * Created by Yonggang Liu. Jul. 14, 2010.
 */

#ifndef __CLINETSPECS_H__
#define __CLINETSPECS_H__

#define OMNET_CID_BASE 2 // The client IDs are the OMNeT++ numbered IDs minus 2.
#define C_TOTAL 32 // Client total number.
#define MAX_APP C_TOTAL // The maximum number of applications.

#define CID_OFFSET 10000 // The offset of the Client ID in the packet IDs.

#define C_TRC_PROC_TIME 0.00003 // The processing time between requests.
#define C_PKT_PROC_TIME 0.000005 // The processing time between packets.

#define TRACE_PATH_PREFIX "traces/trace"
#define RESULT_PATH_PREFIX "results/result"

#endif
