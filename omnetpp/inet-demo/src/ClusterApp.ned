package inetdemo;

import inet.applications.contract.IApp;

// Application of a sensor or of the sink in the INET demo (see ClusterApp.h)
simple ClusterApp like IApp
{
    parameters:
        int port = default(5000);
        bool isSink = default(false);
        string coordinatorModule = default("coordinator");
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);
        double stopOperationTimeout @unit(s) = default(2s);
    gates:
        input socketIn @labels(UdpCommand/up);
        output socketOut @labels(UdpCommand/down);
}

// Runs the rounds (global set-up, as in the ns-3 demo)
simple ClusterCoordinator
{
    parameters:
        int rounds = default(6);
        double roundTime @unit(s) = default(10s);
        double slotTime @unit(s) = default(0.5s);
        int numClusters = default(3);
        @display("i=block/cogwheel");
}
