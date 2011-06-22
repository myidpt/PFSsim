#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "physim_driver.h"
#include "disksim_interface.h"
#include "disksim_rand48.h"

static SysTime now = 0;		/* current time */
static SysTime next_internal_event = 0;	/* next event */
static Stat st;
FILE * outstream = NULL;
long finishedjobid = -1;
double finishtime = 0.0;
FILE * dfp = NULL;

void add_statistics(Stat *s, double x)
{
  s->n++;
  s->sum += x;
  s->sqr += x*x;
}


void print_statistics(Stat *s, const char *title)
{
  double avg, std;

  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  fprintf(dfp, "%s: n=%d average=%f std. deviation=%f\n", title, s->n, avg, std);
}


/*
 * Schedule next callback at time t.
 * Note that there is only *one* outstanding callback at any given time.
 * The callback is for the earliest event.
 */
void syssim_schedule_callback(disksim_interface_callback_t fn, SysTime t, void *ctx)
{
  next_internal_event = t;
//  fprintf(dfp, "schedule_callback: %f\n", next_internal_event);
}


/*
 * de-scehdule a callback.
 */
void
syssim_deschedule_callback(double t, void *ctx)
{
  next_internal_event = -1;
}


void
syssim_report_completion(SysTime t, struct disksim_request *req, void *ctx) // Completion of one request.
{
//  fprintf(dfp, "Request %ld is completed.%lf\n", req->id, t);
  now = t;
  finishtime = t;
  finishedjobid = req->id;
  add_statistics(&st, t - req->start);
  free(req);
}


int main(int argc, char *argv[])
{
  int i;
  int nsectors; // scale of sectors that you want to access.
  int outfdes;
	int busy = 0;
  syncrecv = (struct syncjob_type *)malloc(sizeof(struct syncjob_type));
  syncsendjob = (struct syncjobreply_type *) malloc(sizeof(struct syncjobreply_type));
  syncsendnojob = (struct syncjobreply_type *) malloc(sizeof(struct syncjobreply_type));
  syncsendnojob->fid = -1;
  struct disksim_interface *disksim;
  struct disksim_request * r;

  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n; //measure sent bytes

  if (argc < 5) {
    fprintf(stderr, "usage: %s <portnumber> <param file> <output file> <debug info file>\n", argv[0]);
    exit(1);
  }

  if((dfp=fopen(argv[4],"w+")) == NULL){
    fprintf(stderr, "Debug file open error.\n");
    exit(1);
  }

//  dfp = stdout;
  fprintf(dfp, "Initializing disksim interface ...\n");
  disksim = disksim_interface_initialize(argv[2],
			 argv[3],
			 syssim_report_completion,
			 syssim_schedule_callback,
			 syssim_deschedule_callback,
			 0,
			 0,
			 0);
 
  // socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sockfd < 0){ 
    fprintf(stderr, "ERROR opening socket\n");
    return -1;
  }
 
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
 
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr, "ERROR on binding\n");
    return -1;
  }

  listen(sockfd, 5);

  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
   
  if (newsockfd < 0){
    fprintf(stderr, "ERROR on accept\n");
    close(sockfd);
    return -1;
  }

  fprintf(dfp, "Connection established ...\n");

  // socket end

  while(1){
 
    // *************************************
    // Blocking --- Waiting to receive the sync message.
    // *************************************
    while( (n = read(newsockfd, syncrecv, SYNC_SIZE)) == 0)sleep(0.1);
    if (n < 0){
      fprintf(stderr, "Error: socket read.\n");
      break;
    }
    
//    printf("Received: %ld %lf\nNext_internal_event = %lf\n", syncrecv->id, syncrecv->time, MS_TO_S(next_internal_event));
    fflush(stdout);

    if(syncrecv->id < 0){ // End of simulation
//      fprintf(dfp, "Received end signal. End of simulation.\n");
      break;
    }
    else if(syncrecv->id == 0){
      // ***************************************************
      // id == 0. No new job now. Send synchronization message.
      // If a job is finished, its id will be sent out. Or it will send id=0 to do synchronization.
      // ***************************************************
      
      if(next_internal_event < 0){
        fprintf(stderr, "ERROR: next_internal_event < 0\n");
        return -1;
      }
      while(MS_TO_S(next_internal_event) <= (syncrecv->time+0.0000001)){ // In the time interval (Disksim.now, OMNeT++.now), do all the things in disksim.
        now = next_internal_event;
        next_internal_event = S_TO_MS(-1); // Set to invalid

//        printf("Next_internal. now = %lf\n", now);
        fflush(stdout);
				disksim_interface_internal_event(disksim, now, 0); // Invoke next event.
        while(busy == DEGREE && finishedjobid <= 0){
					now = next_internal_event;
					next_internal_event = S_TO_MS(-1);
					disksim_interface_internal_event(disksim, now, 0); // Invoke next event.
				}
         
        
        // Have finished jobs.
        if(finishedjobid > 0){
          syncsendjob->time = MS_TO_S(finishtime);
          syncsendjob->fid = finishedjobid; // Get the real job id.
     
          if(syncsendjob->fid % 20 == 0){ // Just print information.
            printf("%d: #%ld job finished.\n", portno, syncsendjob->fid);
            fflush(stdout);
          }
            
          n = write(newsockfd, syncsendjob, sizeof(struct syncjobreply_type));
					busy --;
          
//          printf("Write: %ld %lf\n", syncsendjob->fid, syncsendjob->time);

          if (n < 0){
            fprintf(stderr, "Error: socket write.\n");
            return -1;
          }
          finishedjobid = -1;
        }

        
        // If next_internal_event < 0, no more future events.
        if (next_internal_event < 0) 
          break;
      }
     
      syncsendnojob->time = MS_TO_S(next_internal_event); // It can be a sync message (time > 0) or a message indicating no more future events (time < 0).
      n = write(newsockfd, syncsendnojob, sizeof(struct syncjobreply_type));// Send Nojob syncreply.
      
//      printf("Write: %ld %lf\n", syncsendnojob->fid, syncsendnojob->time);
      fflush(stdout);

      if (n < 0){
        fprintf(stderr, "Error: socket write.\n");
        return -1;
      }
    }
    else{
      // *******************************************
      // id > 0, New job.
      // *******************************************
      r = (struct disksim_request *)malloc(sizeof(struct disksim_request));
      r->id = syncrecv->id;
      r->flags = syncrecv->read;
      r->devno = 0;
      r->blkno = syncrecv->off;
      r->bytecount = syncrecv->len * BLOCK;
      r->start = S_TO_MS(syncrecv->time);
//      printf("Req_arrive.id = %ld, blkno = %ld, bytecount = %d, start = %lf\n", r->id, r->blkno, r->bytecount, r->start);
      fflush(stdout);
      disksim_interface_request_arrive(disksim, r->start, r);
			busy ++;
//      printf("Job submitted. %ld %lf\n", r->id, r->start);
//      printf("next = %lf\n", next_internal_event);
      fflush(stdout);
      syncrecv->time = -1; // In new job, the time is not for sync.
    }
  }

  fprintf(dfp, "Closing everything ...\n");
  close(sockfd);
  close(newsockfd);
  disksim_interface_shutdown(disksim, now);

  print_statistics(&st, "response time");
  
  wait(NULL); // Wait for child.

  exit(0);
}
