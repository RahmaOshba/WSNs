package inetdemo;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.SensorNode;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

// INET version of code/0_FIRST_EXPERIMENTS/demo_clustered_wsn.cc:
// 12 sensors in 3 clusters + 1 sink, IEEE 802.15.4 (2.4 GHz, 250 kb/s,
// CSMA/CA with ACKs), UDP/IPv4, battery energy from the radio state.
network ClusterDemo
{
    parameters:
        @display("bgb=680,520");
    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=30,30;is=s");
        }
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=30,90;is=s");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=30,150;is=s");
        }
        coordinator: ClusterCoordinator {
            @display("p=620,30;is=s");
        }
        sink: SensorNode {
            @display("i=device/antennatower;is=l");
        }
        sensor[12]: SensorNode;
}
