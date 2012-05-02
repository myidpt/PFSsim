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
// TODO auto-generated module
//
package client;

simple Client
{
    parameters:
        double trc_proc_time;
        double pkt_proc_time;
        string trc_path_prefix;
        string rslt_path_prefix;
        double pkt_size_limit;
        int small_io_size_threshold; // Considered to be small IO only when smaller than this limit.
        int trcs_per_client; // Each app has an independent trace file.
        int max_window_size;
//        int degree;
        
        @display("i=device/pc2");
        bool sendInitialMessage = true;
    gates:
        inout g;
}
