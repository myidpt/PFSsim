package ned.router;

module Router
{
    parameters:
        @display("i=device/router");
    gates:
        inout c[]; // The size is defined in pfs.ned.
        inout ds[];
        inout proxy[];
        inout intersch[];
        inout ms;
    submodules:
        routing: Routing {
            parameters:
                @display("p=211,179");
            gates:
                cin[sizeof(c)];
                dsin[sizeof(ds)];
                schin[sizeof(proxy)];
                interschin[sizeof(intersch)];
                
                cout[sizeof(c)];
                dsout[sizeof(ds)];
                schout[sizeof(proxy)];
                interschout[sizeof(intersch)];
        }
        cq[sizeof(c)]: Queue {
            @display("p=211,58");
        }
        dsq[sizeof(ds)]: Queue {
            @display("p=211,302");
        }
        schq[sizeof(proxy)]: Queue {
            @display("p=76,120");
        }
        interschq[sizeof(intersch)]: Queue {
            @display("p=76,249");
        }
        msq: Queue {
            @display("p=359,179");
        }
    connections:
        // clinet
        for i=0..sizeof(c)-1 {
            routing.cout[i] --> cq[i].in;
            routing.cin[i] <-- cq[i].out;
            c[i] <--> cq[i].line;
        }
        // data server
        for i=0..sizeof(ds)-1 {
            routing.dsout[i] --> dsq[i].in;
            routing.dsin[i] <-- dsq[i].out;
            ds[i] <--> dsq[i].line;
        }
        
        // scheduler
        for i=0..sizeof(proxy)-1 {
            routing.schout[i] --> schq[i].in;
            routing.schin[i] <-- schq[i].out;
            proxy[i] <--> schq[i].line;

            routing.interschout[i] --> interschq[i].in;
            routing.interschin[i] <-- interschq[i].out;
            intersch[i] <--> interschq[i].line;
        }
        
        // metadata server
        routing.msout --> msq.in;
        routing.msin <-- msq.out;
        ms <--> msq.line;
}
