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

package router;

module Router
{
    parameters:
        @display("i=device/router");
    gates:
        inout c[]; // The size is defined in pfs.ned.
        inout ds[];
        inout dssch[];
        inout ms;
    submodules:
        routing: Routing {
            parameters:
                @display("p=194,180");
            gates:
                cin[sizeof(c)];
                dsin[sizeof(ds)];
                dsschin[sizeof(dssch)];
                cout[sizeof(c)];
                dsout[sizeof(dssch)];
                dsschout[sizeof(dssch)];
        }
        cq[sizeof(c)]: Queue {
            @display("p=194,59");
        }
        dsq[sizeof(ds)]: Queue {
            @display("p=194,303");
        }
        dsschq[sizeof(dssch)]: Queue {
            @display("p=45,180");
        }
        msq: Queue {
            @display("p=345,180");
        }
    connections:
        // clinet
        for i=0..sizeof(c)-1 {
            routing.cout[i] --> cq[i].in;
            routing.cin[i] <-- cq[i].out;
            c[i] <--> cq[i].line;
        }
        // data server and dsscheduler
        for i=0..sizeof(ds)-1 {
            routing.dsout[i] --> dsq[i].in;
            routing.dsin[i] <-- dsq[i].out;
            ds[i] <--> dsq[i].line;
            
            routing.dsschout[i] --> dsschq[i].in;
            routing.dsschin[i] <-- dsschq[i].out;
            dssch[i] <--> dsschq[i].line;
        }
        // metadata server
        routing.msout --> msq.in;
        routing.msin <-- msq.out;
        ms <--> msq.line;
}
