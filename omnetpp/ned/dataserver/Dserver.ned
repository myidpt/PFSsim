package ned.dataserver;

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

            @display("p=234,175");
        }
        vfs: VFS {
            parameters:
                @display("p=137,123");
        }
        lfs: LFS_M {
            parameters:
                @display("p=137,231");
        }
        disk: Disk {
            parameters:
                @display("p=137,305");
        }
    connections:
        g <--> dsd.eth;
        dsd.vfs <--> vfs.dsd;
        lfs.disk <--> disk.g;
        lfs.diskcache <--> diskcache.lfs;
        vfs.diskcache <--> diskcache.vfs;
        vfs.lfs <--> lfs.vfs;
}
