package ned.client;

simple PFSClient
{
    parameters:
        string pfs_client_signature;
        
        // Time overhead.
        double data_packet_process_time;
        double metadata_packet_process_time;

        double packet_size_limit;
        int small_io_size_threshold; // Considered to be small IO only when smaller than this limit.
        int max_transfer_window_size;
        
        int output_file_client_index_digits;
        
        // For data packet output.
        string data_packet_output_method;
        int data_packet_output_file_count;
        string data_packet_output_file_prefix_before_client_ID;
        string data_packet_output_file_prefix_after_client_ID;
        string data_packet_output_file_postfix;
        int data_packet_output_file_index_digits;
        
        // For metadata packet output.
        string metadata_packet_output_method;
        int metadata_packet_output_file_count;
        string metadata_packet_output_file_prefix_before_client_ID;
        string metadata_packet_output_file_prefix_after_client_ID;
        string metadata_packet_output_file_postfix;
        int metadata_packet_output_file_index_digits;
        
        @display("i=device/device");
    gates:
        inout eth;
        inout app;
}
