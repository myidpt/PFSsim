package ned.dataserver;

simple LFS_M {
    parameters:
        int page_size;
        int blk_size;
        int new_ext_size;
        int new_ext_gap;
        int degree;
        string fsname;
        double disk_size;
        string ext_in_path_prefix;
        string ext_out_path_prefix;
        @display("i=device/device");
    gates:
        inout diskcache;
        inout disk;
        inout vfs;
}