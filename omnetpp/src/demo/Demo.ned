package wsn.demo;

// One sensor of the demo. Packets travel "through the air" with sendDirect:
// the propagation delay follows the distance and the duration follows the
// bit rate, so Qtenv shows every frame flying from sender to receiver.
simple DemoSensor
{
    parameters:
        double x @unit(m);
        double y @unit(m);
        int cluster;
        double initialEnergy @unit(J) = default(0.5J);
        @display("i=misc/node_vs");
    gates:
        input radioIn @directIn;
}

// The sink / base station. It also runs the set-up of every round
// (the same global rule as the ns-3 demo): in each cluster the node with the
// most energy that has not been CH in this epoch becomes CH.
simple DemoSink
{
    parameters:
        double x @unit(m) = 40m;
        double y @unit(m) = 10m;
        int rounds = default(6);
        double roundTime @unit(s) = default(10s);
        double slotTime @unit(s) = default(0.5s);
        @display("i=device/antennatower");
    gates:
        input radioIn @directIn;
}

// Demo network: 12 sensors in 3 clusters of 4 + one sink on top
// (same positions as code/0_FIRST_EXPERIMENTS/demo_clustered_wsn.cc).
network DemoNetwork
{
    parameters:
        double scale = default(8);   // pixels per metre (positions are set from x, y)
        @display("bgb=640,480;bgg=80,1,lightgrey");
    submodules:
        sink: DemoSink;
        sensor[12]: DemoSensor {
            cluster = int(index / 4);
            x = (index < 4 ? 12m : (index < 8 ? 40m : 68m)) + (index % 4 == 0 ? -6m : (index % 4 == 1 ? 6m : (index % 4 == 2 ? -5m : 7m)));
            y = 45m + (index % 4 == 0 ? -5m : (index % 4 == 1 ? -6m : (index % 4 == 2 ? 6m : 5m)));
        }
}
