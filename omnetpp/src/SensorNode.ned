package wsn;

// One sensor node. Created by the Controller at deployment; its energy,
// role (CH / member / dead) and cluster colour follow the protocol engine.
simple SensorNode
{
    parameters:
        double x @unit(m) = default(0m);   // position in the field (set by the Controller)
        double y @unit(m) = default(0m);
        @display("i=misc/node_vs,grey,30");
}
