/*
 * Author: Yonggang Liu
 */

#include <omnetpp.h>
#include "General.h"
#include "iostreamer/StreamersFactory.h"
#include "trace/SimpleTrace.h"


// This class counts the application trace time.
class Application : public cSimpleModule
{
protected:
    static int activeApplications;
    bool active;

    int myID;
	static int initID;
	static int numClients;

	ITraceInputStreamer * traceInput;
	ITraceOutputStreamer * traceOutput;

	int count;

	// Time overhead.
	double traceProcessTime;

    inline void generateInitialTraces();
    inline void sendNewAppRequest(AppRequest * request);
    inline void handleFinishedTrace(AppRequest * request);
    inline void readOneTrace(int id);
    inline void sendSafe(AppRequest * request);
    void finish();
public:
	Application();
    void initialize();
    void handleMessage(cMessage * msg);
};
