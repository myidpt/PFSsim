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
package dserver;

import dserver.disk.Disk;
import dserver.dsd.DSD_M;
import dserver.lfs.LFS_M;
import dserver.vfs.VFS;
import dserver.diskcache.DiskCache;

module Dserver
{
    parameters:
        @display("i=device/server");
        bool sendInitialMessage = false;
    gates:
        inout g;
    submodules:
        dsd: DSD_M {
            parameters:
                @display("p=137,52");
        }
        diskcache: DiskCache {

            @display("p=229,194");
        }
        vfs: VFS {
            parameters:
                @display("p=137,123");
        }
        lfs: LFS_M {
            parameters:
                @display("p=137,259");
        }
        disk: Disk {
            parameters:
                @display("p=137,333");
        }
    connections:
        g <--> dsd.eth;
        dsd.vfs <--> vfs.dsd;
        lfs.disk <--> disk.g;
        lfs.diskcache <--> diskcache.lfs;
        vfs.diskcache <--> diskcache.vfs;
}
