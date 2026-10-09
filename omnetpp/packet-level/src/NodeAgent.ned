package pktwsn;

import inet.applications.contract.IApp;

// Node side of the packet-level runs (see NodeAgent.h)
simple NodeAgent like IApp
{
    parameters:
        int port = default(6000);
        string brainModule = default("brain");
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);
        double stopOperationTimeout @unit(s) = default(2s);
    gates:
        input socketIn @labels(UdpCommand/up);
        output socketOut @labels(UdpCommand/down);
}
