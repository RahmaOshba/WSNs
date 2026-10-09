package wsn;

// The field: one base station, the sensor nodes (created at run time, so the
// node count follows the protocol settings) and the protocol Controller.
network WsnNetwork
{
    parameters:
        @display("bgb=440,440;bgg=40,1,lightgrey");
    submodules:
        controller: Controller;
        bs: BaseStation;
}
