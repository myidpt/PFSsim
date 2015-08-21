package ned.dataserver;

simple Disk
{
    parameters:
        double disk_size;
        int ra_size; // Read-ahead size
        int degree;
        string disk_parm_path;
        int return_zero;
        double return_zero_period;
        int numDservers;
        @display("i=device/disk");
    gates:
        inout g;
}