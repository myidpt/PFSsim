package ned.metadataserver;

//
// TODO auto-generated module
//
simple MetadataServer
{
    parameters:
        string pfs_metadataserver_signature;
        
        double data_proc_time;
        double metadata_proc_time; // About process time.
        
        int pfs_input_file_num;
        int pfs_input_file_ID_digits;
        string pfs_input_file_prefix;
        string pfs_input_file_postfix;
        
        int numMetaServer;
        
        @display("i=device/mainframe");
        bool sendInitialMessage = false;
    gates:
        inout g; 
}
