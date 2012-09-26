//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Application.h"

Define_Module(Application);

int Application::initID = 0;

Application::Application() {
	myID = initID ++;
}

void Application::initialize() {
	StreamersFactory streamersfactory;
	// For initialization of input trace files.
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

void Application::handleMessage(cMessage * message) {
	switch(message->getKind()) {
	case TRACE_REQ:
		sendNewTrace((AppRequest *)message);
		break;
	case TRACE_RESP:
		handleFinishedTrace((AppRequest *)message);
		break;
	default:
		PrintError::print("Application::handleMessage", "Message kind not recognized: ", message->getKind());
	}
}

/// <summary>
/// Process the initial traces.
/// </summary>
void Application::generateInitialTraces() {
	SimpleTrace trace;
	for(int i = 0; i < traceInput->getTraceFlowCount(); i ++) {
		// Read trace.
		if(! traceInput->readTrace(i, &trace)) {
			// End of file.
			return;
		}

		// Create AppRequest.
		AppRequest * request = trace.createAppRequest();
		request->setKind(TRACE_REQ);

		// Schedule AppRequest.
		double earliestSendTime = SIMTIME_DBL(simTime()) + traceProcessTime;
		if(trace.getStartTime() > earliestSendTime) {
			scheduleAt(trace.getStartTime(), request);
		}
		else {
			request->setStarttime(earliestSendTime); // Renew the start time in trace.
			sendNewTrace(request);
		}
#ifdef APP_DEBUG
	cout << "Application-" << myID << "::GenerateInitialTrace TraceID=" << request->getID()
			<< " TraceFileID=" << request->getTraceFileID() << " PFSFileID=" << request->getFileID()
			<< " APP=" << request->getApp() << " HO=" << request->getHighoffset()
			<< " LO=" << request->getLowoffset() << endl;
#endif
	}
}

/// <summary>
/// Send the AppRequest to PFSClient_M.
/// </summary>
void Application::sendNewTrace(AppRequest * request) {
	sendSafe(request);
}

/// <summary>
/// Print the trace.
/// Destroy the old request and create a new request.
/// Send the AppRequest to PFSClient_M.
/// </summary>
void Application::handleFinishedTrace(AppRequest * request) {
	// Print the trace.
	request->setFinishtime(SIMTIME_DBL(simTime()));
	SimpleTrace trace;
	trace.initialize(request);
	if (traceOutput != NULL) {
		traceOutput->writeTrace(&trace);
	}

	// Destroy the old request and create a new one.
	if(! traceInput->readTrace(request->getTraceFileID(), &trace)) {
		// End of file.
		return;
	}
	delete request;

	// This is a new request
	AppRequest * newrequest = trace.createAppRequest();
	newrequest->setKind(TRACE_REQ);

	// Schedule AppRequest.
	double earliestSendTime = SIMTIME_DBL(simTime()) + traceProcessTime;
	if(trace.getStartTime() > earliestSendTime) {
		scheduleAt(trace.getStartTime(), newrequest);
	}
	else {
		newrequest->setStarttime(earliestSendTime);
		sendNewTrace(newrequest);
	}
#ifdef APP_DEBUG
	cout << "Application-" << myID << "::GenerateInitialTrace TraceID=" << newrequest->getID()
			<< " TraceFileID=" << newrequest->getTraceFileID() << " PFSFileID=" << newrequest->getFileID()
			<< " APP=" << newrequest->getApp() << " HO=" << newrequest->getHighoffset()
			<< " LO=" << newrequest->getLowoffset() << endl;
#endif
}

void Application::sendSafe(AppRequest * request) {
	send(request, "pfs$o");
}

void Application::finish() {
	if (traceInput != NULL) {
		delete traceInput;
	}
	if (traceOutput != NULL) {
		delete traceOutput;
	}
}
