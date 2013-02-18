/*
 * Author: Yonggang Liu
 * Application class simulates the applications in the real world. it sends out the trace requests to the PFSClient,
 * and gets the trace responses from the PFSClient. The number of outstanding traces for each application is managed
 * by this class.
 */

#include "Application.h"

Define_Module(Application);

/*
 * Used to initialize the IDs for each application class.
 */
int Application::initID = 0;

/*
 * Initialize the ID.
 */
Application::Application() {
	myID = initID ++;
}

/*
 * 1. Initialize the input and output streamers.
 * 2. generate the initial traces.
 */
void Application::initialize() {
	StreamersFactory streamersfactory;
	// For initialization of trace input files.
	int count = par("trace_count").longValue();
	int traceDigits = par("trace_file_trace_index_digits").longValue();
	int clientDigits = par("trace_file_client_index_digits").longValue();

	string prefixBeforeClientID = par("trace_input_file_prefix_before_client_ID").stdstringValue();
	string prefixAfterClientID = par("trace_input_file_prefix_after_client_ID").stdstringValue();
	string postfix = par("trace_input_file_postfix").stdstringValue();
	string method = par("trace_input_method").stdstringValue();

	if(method.compare(streamersfactory.FILE_OUTPUT_SIGNITURE) == 0) {
		traceInput = streamersfactory.createClientTraceInputStreamer(prefixBeforeClientID, myID, clientDigits,
				prefixAfterClientID, count, traceDigits, postfix);
	}
	else {
		PrintError::print("Application::initialize", "trace input method is not correct.");
	}

	// For initialization of trace output files.
	prefixBeforeClientID = par("trace_output_file_prefix_before_client_ID").stdstringValue();
	prefixAfterClientID = par("trace_output_file_prefix_after_client_ID").stdstringValue();
	postfix = par("trace_output_file_postfix").stdstringValue();
	method = par("trace_output_method").stdstringValue();

	if (method.compare(streamersfactory.FILE_OUTPUT_SIGNITURE) == 0) {
		traceOutput = streamersfactory.createClientTraceOutputStreamer(prefixBeforeClientID, myID, clientDigits,
				prefixAfterClientID, count, traceDigits, postfix);
	}
	else if (method.compare(streamersfactory.NONE_OUTPUT_SIGNITURE) == 0) {
		traceOutput = NULL;
	}
	else {
		PrintError::print("Application::initialize", "trace output method is not correct.");
	}

	traceProcessTime = par("trace_proc_time").doubleValue();

	// Process the initial trace.
	generateInitialTraces();
}

/*
 * Handle the messages:
 * TRACE_REQ comes from itself.
 * TRACE_RESP comes from PFSClient.
 */
void Application::handleMessage(cMessage * message) {
	switch(message->getKind()) {
	case TRACE_REQ:
		sendNewAppRequest((AppRequest *)message);
		break;
	case TRACE_RESP:
		handleFinishedTrace((AppRequest *)message);
		break;
	default:
		PrintError::print("Application::handleMessage", "Message kind not recognized: ", message->getKind());
		break;
	}
}

/*
 * Generate the initial traces.
 * Generate one trace request for each trace input flow.
 */
void Application::generateInitialTraces() {
	for(int i = 0; i < traceInput->getTraceFlowCount(); i ++) {
	    readOneTrace(i);
	}
}

/*
 * Print the finished trace.
 * Destroy the old request and create a new request.
 * Send the AppRequest to PFSClient.
 */
void Application::handleFinishedTrace(AppRequest * request) {
	// Print the trace.
	request->setFinishtime(SIMTIME_DBL(simTime()));
	SimpleTrace trace;
	trace.initialize(request);
	if (traceOutput != NULL) {
		traceOutput->writeTrace(&trace);
	}

	int id = request->getTraceFileID();
    delete request;

    readOneTrace(id);
}

/*
 * Read one trace.
 */
void Application::readOneTrace(int id) {
    // Read trace.
    SimpleTrace trace;
    if(! traceInput->readTrace(id, &trace)) {
        // End of file.
        return;
    }

    // Create AppRequest.
    AppRequest * request = trace.createAppRequest();
    request->setKind(TRACE_REQ);

    // Schedule AppRequest.
    double sendTime;
    if (trace.getSync() == 0) { // Non-sync.
        sendTime = trace.getStartTime();
    } else { // sync.
        sendTime = SIMTIME_DBL(simTime()) + trace.getStartTime();
    }

    double earliestSendTime = SIMTIME_DBL(simTime()) + traceProcessTime;
    if (earliestSendTime > sendTime) {
        sendTime = earliestSendTime;
    }

    request->setStarttime(sendTime); // Set the start time in trace.
    if(sendTime > SIMTIME_DBL(simTime())) {
        scheduleAt(sendTime, request);
    } else {
        sendNewAppRequest(request);
    }
#ifdef APP_DEBUG
    cout << "Application-" << myID << "::GenerateInitialTrace TraceID=" << request->getID()
            << " TraceFileID=" << request->getTraceFileID() << " PFSFileID=" << request->getFileID()
            << " APP=" << request->getApp() << " HO=" << request->getHighoffset()
            << " LO=" << request->getLowoffset() << endl;
#endif
}

/*
 * Send the AppRequest.
 */
void Application::sendNewAppRequest(AppRequest * request) {
    sendSafe(request);
}

/*
 * Send the AppRequest to PFSClient.
 * Because the connection between Application and PFSClient is ideal, just send it.
 */
void Application::sendSafe(AppRequest * request) {
	send(request, "pfs$o");
}

/*
 * Clean the memory.
 */
void Application::finish() {
	if (traceInput != NULL) {
		delete traceInput;
	}
	if (traceOutput != NULL) {
		delete traceOutput;
	}
}
