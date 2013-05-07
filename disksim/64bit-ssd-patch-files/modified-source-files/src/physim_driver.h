#include "disksim_interface.h"

#define BLOCK            4096
#define DEGREE           1
#define SYNC_SIZE        24 // The size of the synchronization message.
/* OMNet++ uses second as time unit, while disksim uses millisecond as time unit. */
#define AMPLIFY          15000
#define S_TO_MS(time)    (time*AMPLIFY)
#define MS_TO_S(time)    (time/AMPLIFY)

int degree;
int outstanding;
int sockfd;
int newsockfd;
int portno;

typedef	double SysTime;		/* system time in seconds.usec */
typedef	struct	{
  int n;
  double sum;
  double sqr;
} Stat;

struct syncjob_type{ // OMNET++ -> Disksim, also used as a queue.
  long id;
  double time;
  long off;
  int len;
  int read;
} * syncrecv;

struct syncjobreply_type{ // Disksim -> OMNET++
  double time; // the time of next event in disksim.
  long fid; // If one job is finished, the ID of the finished job. Otherwise, -1;
} * syncsendjob, * syncsendnojob;


void syssim_schedule_callback(disksim_interface_callback_t, SysTime t, void *);
void syssim_report_completion(SysTime t, struct disksim_request *r, void *);
void syssim_deschedule_callback(double, void *);

int createConnection();
int submitNewJob(struct disksim_interface *);
int syncNoJob(struct disksim_interface *);
void add_statistics(Stat *, double);
void print_statistics(Stat *, const char *);


pid_t fork(void);
pid_t wait(int *);
pid_t getpid(void);
int strcmp(const char *, const char *);
void bzero(char *, int);
size_t strlen ( const char * str );
int read(int, void *, int);
int write(int, void *, int);
int close(int);
unsigned int sleep(unsigned int);
void * memcpy ( void *, const void *, size_t);
