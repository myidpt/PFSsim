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
        int numDservers;
        @display("i=device/memory");
    gates:
        inout vfs;
        inout lfs;
}
