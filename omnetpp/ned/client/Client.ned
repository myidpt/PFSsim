package ned.client;


module Client
{
    parameters:
        @display("i=device/pc2");
        bool sendInitialMessage = true;
    gates:
        inout gate;
    submodules:
        application: Application {
            parameters:
                @display("p=137,52");
        }
        pfsclient: PFSClient {
            parameters:
                @display("p=137,235");
        }
    connections:
        gate <--> pfsclient.eth;
        pfsclient.app <--> application.pfs;
}
