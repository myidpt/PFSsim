package ned;

import ned.client.Client;
import ned.dataserver.Dserver;
import ned.metadataserver.MetadataServer;
import ned.router.Router;
import ned.proxy.Proxy;

network pfssim_1
{
    parameters:
        int numClients;
        int numDservers;
        int numProxies;
    types:
        channel Channel extends ned.DatarateChannel
        {
            delay = 0.04 ms; // client <-> switch, switch <-> data server
//            delay = 0.003 ms; // local
            datarate = 940 Mbps; // client <-> switch, switch <-> data server
//            datarate = 11.3 Gbps; // local
        }
        channel PChannel extends ned.DatarateChannel
        {
            delay = 0 ms;
        }
        channel InterSchChannel extends ned.DatarateChannel
        {
            delay = 0.05 ms;
        }
    submodules:
        c[numClients]: Client {
            parameters:
                @display("p=121,52");
        }

        ds[numDservers]: Dserver {
            parameters:
                @display("p=265,258");
        }

        proxy[numProxies]: Proxy {
            parameters:
                @display("p=121,258");
        }

        ms: MetadataServer {
            parameters:
                @display("p=353,146");
        }
        switch: Router {
            parameters:
                @display("p=189,146");
            gates:
                c[numClients];
                ds[numDservers];
                proxy[numProxies];
                intersch[numProxies];
        }
    connections:
        // C-Router channels
        for i=0..numClients-1 {
            c[i].gate <--> Channel <--> switch.c[i];
        }

        // Router-DS channels
        for i=0..numDservers-1 {
            switch.ds[i] <--> Channel <--> ds[i].g;
        }

        // Router-DSscheduler and inter-scheduler channels
        for i=0..numProxies-1 {
			switch.proxy[i] <--> PChannel <--> proxy[i].g;
            switch.intersch[i] <--> InterSchChannel <--> proxy[i].schg;
        }

        // MS-Router channels
        ms.g <--> Channel <--> switch.ms;
        
}
