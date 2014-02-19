// Controller for Interposed 2-level I/O Scheduling Framework for Performance Virtualization.
// The controller makes decisions at the end of each time window k.

#ifndef AVATAR_H_
#define AVATAR_H_

#include "IQueue.h"
#include "EDF.h"
#define MAX_REQ_IN_WINDOW 500
#define RESP_TIME_RANK 0.95
#define AVATAR_SAFE_L_O 4
#define AVATAR_INFINITY 100
//#define APPR(X) ((X) + 0.5) // Approximation.
#define MIN(X,Y) ((X) <= (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) >= (Y) ? (X) : (Y))


using namespace std;

class AVATAR {
private:
    EDF * queue; // EDF queue.

    int windowIndex; // Window index.
    double currentWindowEnd;// The time when the current window ends.
    int appNum; // Number of applications.
    double QoSDelay[MAX_APP]; // QoS delay.

    int L_E_New; // The number of request arrivals in one window.
    // Updated when new request arrives. Used for prediction. Cleared when a new window is opened.
    int L_E_New_Deadline; // The number of request arrivals whose deadlines lie in the same time window, too.
    // Updated when new request arrives and it's deadline is in the same window. Used for prediction.
    // Cleared when a new window is opened.

    int X; // The number of completed requests within the window.

    double T_E_All[MAX_APP]; // The total waiting time for all requests in one window, for calculating MT_E.
    int T_E_Count[MAX_APP]; // To count the number of requests dispatched in the last window. For MT_E.
    double MT_E[MAX_APP]; // Average waiting time for requests for each class.

    double T_O_All[MAX_APP][MAX_REQ_IN_WINDOW]; // The response time for all requests. Used to calculate T_O.
    int T_O_Index[MAX_APP]; // The number of requests dispatched for each application. Used to calculate T_O.
    double T_O[MAX_APP]; // The 95th percentile of response time for each class.

    int L_O_max; // Max number of outstanding requests during the current time window at the storage utility S_O.
    double L_O; // Queue threshold at the storage utility.
    double L_O_PerApp[MAX_APP]; // The queue length at the server for each application.
    double L_O_RT[MAX_APP]; // The queue length threshold at the server for each application, based on the response time.

    double L_O_X_Lower; // The lower bound of queue length at server.
    double L_O_X_Upper; // The upper bound of queue length at server.

    double windowSize; // The time length of the window.

    bool idle_status;

    bool miss_deadline;

    inline int L_E_Exist(); // The number of requests exist in the EDF queue.
    // The queue size of the the EDF queue. Calculate whenever it is called.
    inline int L_E_Exist_Deadline(double t); // The number of requests exist in EDF queue and deadline is in the same window.
    // Calculate whenever it is called.
    inline int L_O_Exist(); // The existing (outstanding) requests at the server.
    // Calculate whenever it is called.
    inline void calculateStatistics(); // Calculate T_O and MT_E.
    // A small utility function to get a value from an array by its value rank (low to high).
    double getValueByRank(double array[], int total, double rank);
public:

    AVATAR(int ttlapps, double delay[], double wsize);
    void pushWaitQ(bPacket * packet);
    bPacket * dispatchNext();
    bPacket * popOsQ(long id);
    bPacket * queryJob(long id);
    void windowEnd();
    double getCurrentWindowEnd();
    bool getIdleStatus();
    ~AVATAR();
};

#endif /* AVATAR_H_ */
