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

//
// Two instances (tic and toc) of Txc connected.
//
// DatarateChannel
import ned.client.Client;
import ned.dataserver.Dserver;
import ned.metadataserver.MetadataServer;
import ned.router.Router;
import ned.proxy.Proxy;

package ned;

network pfssim_1
{
    parameters:
        int numClients;
        int numDservers;
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

        proxy[numDservers]: Proxy {
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
                proxy[numDservers];
                intersch[numDservers];
        }
    connections:
        // C-Router channels
        for i=0..numClients-1 {
            c[i].gate <--> Channel <--> switch.c[i];
        }

        // Router-DS and Router-DSscheduler channels
        for i=0..numDservers-1 {
            switch.ds[i] <--> Channel <--> ds[i].g;
            switch.proxy[i] <--> PChannel <--> proxy[i].g;
            switch.intersch[i] <--> InterSchChannel <--> proxy[i].schg;
        }

        // MS-Router channels
        ms.g <--> Channel <--> switch.ms;
        
}
