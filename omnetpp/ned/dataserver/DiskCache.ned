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

package ned.dataserver;

//
// TODO auto-generated module
//
simple DiskCache{
    parameters:
        int total_pages;
        int usable_pages;
        double cache_r_speed; // The time cost of reading a page in cache.
        double cache_w_speed; // The time cost of writing a page in cache.
        double dirty_ratio;
        double dirty_background_ratio;
        double dirty_expire_centisecs;
        int writeout_batch; // The number of pages to write back when the dirty_ratio is met.
        int diskread_batch; // The max number of pages to read each time.
        int disable_ra; // Read ahead can be shut off by tunning it to 1.
        @display("i=device/memory");
    gates:
        inout vfs;
        inout lfs;
}
