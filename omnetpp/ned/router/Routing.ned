package ned.router;

//
// TODO auto-generated module
//
simple Routing
{
    parameters:
        int numDservers;
        int numProxies;
        string proxy_routing_path;
        @display("i=block/routing");
    gates:
        input cin[];
        input dsin[];
        input msin;
        input schin[];
        input interschin[];
        
        output cout[];
        output dsout[];
        output msout;
        output schout[];
        output interschout[];
}
