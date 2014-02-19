package ned.dataserver;

simple VFS {
    parameters:
        int page_size;
        int degree;
        @display("i=device/cpu");
    gates:
        inout dsd;
        inout diskcache;
        inout lfs;
}