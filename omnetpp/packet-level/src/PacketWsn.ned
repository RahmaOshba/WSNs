package pktwsn;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.SensorNode;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

// Packet-level WSN: the thesis protocols over INET's IEEE 802.15.4
// (CSMA/CA MAC with ACKs and retransmissions, 250 kb/s, one shared channel).
network PacketWsn
{
    parameters:
        int numNodes = default(100);
        brain.numNodes = this.numNodes;
        @display("bgb=760,560");
    submodules:
        brain: Brain {           // first: it places the nodes before they initialise
            @display("p=700,80");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=700,100;is=s");
        }
        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=700,150;is=s");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=700,200;is=s");
        }
        sink: SensorNode {
            @display("i=device/antennatower;is=l");
        }
        sensor[numNodes]: SensorNode {
            @display("i=misc/node;is=vs");
        }
}
