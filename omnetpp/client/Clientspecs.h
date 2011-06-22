/*
 * Clientspec.h
 * Created by Yonggang Liu. Jul. 14, 2010.
 */

#ifndef __CLINETSPECS_H__
#define __CLINETSPECS_H__

#define C_TOTAL 32 // Client total number
#define MAX_APP C_TOTAL // Max number of applications / flows
#define APP_TOTAL 1 // App total number

#define C_ID_BASE 2
#define CLIENT_INDEX 10000

#define MAX_JOB_SIZE 4096 * 512 // According to Disksim

#define C_REQ_PROC_TIME 0.00003 // The processing time between requests.
#define C_PKT_PROC_TIME 0.000005 // The processing time between packets.

// Weight of applications / clients
// Weight bigger-> higher priority
#define SET_WEIGHT do {\
	for(int wi = 0; wi < APP_TOTAL/2; wi ++){\
		weight[wi] = 1000;\
	}\
	for(int wi = APP_TOTAL/2; wi < APP_TOTAL; wi ++){\
		weight[wi] = 2000;\
	}\
} while(0);

#endif
