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

package proxy;

//
// TODO auto-generated module
//
simple Proxy
{
    parameters:
        int algorithm;
        string alg_param;
        double alg_prop_int;
        double alg_prop_wl;
        int degree;
        double newjob_proc_time;
        double finjob_proc_time;
        int numApps;
        @display("i=device/server2");
        bool sendInitialMessage = false;
    gates:
        inout g;
        inout schg;
}
