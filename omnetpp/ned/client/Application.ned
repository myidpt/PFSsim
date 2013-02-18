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

package ned.client;

//
// TODO auto-generated module
//
simple Application
{
    parameters:
        double trace_proc_time;
        
        string trace_input_method;
        string trace_output_method;
        
        string trace_input_file_prefix_before_client_ID;
        string trace_input_file_prefix_after_client_ID;
        string trace_input_file_postfix;
        
        string trace_output_file_prefix_before_client_ID;
        string trace_output_file_prefix_after_client_ID;
        string trace_output_file_postfix;
        
        int trace_count;
        int trace_file_trace_index_digits;
        int trace_file_client_index_digits;
        
        @display("i=block/app");
        bool sendInitialMessage = true;
    gates:
        inout pfs;
}
