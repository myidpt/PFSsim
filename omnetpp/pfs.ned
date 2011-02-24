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
import client.Client;
import server.Dserver;
import server.Mserver;
import router.Router;

network pfs
{
    parameters:
        int numClients = 64;
        int numDServers = 4;
    types:
        channel Channel extends ned.DatarateChannel
        {
            delay = 0.2 ms; // 100 microseconds
            datarate = 2 Gbps; // 500 Gbps
        }
        channel PChannel extends ned.DelayChannel
        {
            delay = 100 us;
        }
    submodules:
        c[numClients]: Client {
            parameters:
                @display("p=121,52");
        }

        ds[numDServers]: Dserver {
            parameters:
                @display("p=76,233");
        }

        ms: Mserver {
            parameters:
                @display("p=353,146");
        }
        router: Router {
            parameters:
                @display("p=189,146");
            gates:
                c[numClients];
                ds[numDServers];
        }
    connections:
        // C-Router channels
        for i=0..numClients-1 {
            c[i].g <--> Channel <--> router.c[i];
        }

        // DS-Router channels
        for i=0..numDServers-1 {
            ds[i].g <--> Channel <--> router.ds[i];
        }

        // MS-Router channels
        ms.g <--> Channel <--> router.ms;
}
