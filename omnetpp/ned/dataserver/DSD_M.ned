package ned.dataserver;

simple DSD_M
{
    parameters:
        @display("i=device/device");
        double parallel_job_proc_time;
        double write_data_proc_time;
        double write_metadata_proc_time;
        double read_metadata_proc_time;
        double small_io_size_threshold;
        double max_subreq_size;
        int packet_size_limit;
        int degree;
        string pfsname;
        int numDservers;
        int O_DIRECT; // If set, the IO will go directly to the LFS.
    gates:
        inout eth;
        inout vfs;
}