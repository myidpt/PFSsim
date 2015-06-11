package ned.client;

//
// TODO auto-generated module
//
simple Application
{
    parameters:
        double trace_proc_time;
        
        string trace_input_method;
        string trace_output_method;
        
        string trace_input_file_prefix_before_client_ID;
        string trace_input_file_prefix_after_client_ID;
        string trace_input_file_postfix;
        
        string trace_output_file_prefix_before_client_ID;
        string trace_output_file_prefix_after_client_ID;
        string trace_output_file_postfix;
        
        int trace_count;
        int numClients;
        int trace_file_trace_index_digits;
        int trace_file_client_index_digits;
        
        @display("i=block/app");
        bool sendInitialMessage = true;
    gates:
        inout pfs;
}
