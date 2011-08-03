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

package dserver;

simple LocalFS {
    parameters:
        int pagesize;
        int totalpage;
        int degree;
    	int app_data_range;
        double cache_r_speed; // The time cost of reading a page in cache.
        double cache_w_speed; // The time cost of writing a page in cache.
        int max_pr_per_req; // The maximum number of disk access page-ranges a request will incur. Internal use.
        @display("i=device/memory");
    gates:
        inout dsd;
        inout disk;
}