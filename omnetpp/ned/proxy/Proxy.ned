package ned.proxy;

//
// TODO auto-generated module
//
simple Proxy
{
    parameters:
        string algorithm;
        string alg_param;
        int degree;
        double newjob_proc_time;
        double finjob_proc_time;
        int numApps;
        int numProxies;
        @display("i=device/server2");
        bool sendInitialMessage = false;
    gates:
        inout g;
        inout schg;
}
