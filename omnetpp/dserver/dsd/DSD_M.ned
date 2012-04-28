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

package dserver.dsd;

simple DSD_M
{
    parameters:
        @display("i=device/device");
        double parallel_job_proc_time;
        double write_data_proc_time;
        double write_metadata_proc_time;
        double read_metadata_proc_time;
        double small_io_size_threshold;
        double object_size;
        double max_subreq_size;
        int degree;
        string pfsname;
        
        int O_DIRECT; // If set, the IO will go directly to the LFS.
    gates:
        inout eth;
        inout vfs;
}