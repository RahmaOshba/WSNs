package wsn;

// Runs one clustering protocol of the thesis. Every protocol round is one
// step of simulation time; after each round the node modules, the canvas
// and the statistics are updated.
simple Controller
{
    parameters:
        string protocol;                       // e.g. "leach_EDITED", "v8_chain_center" (see omnetpp.ini)
        string defines = default("");          // the ns-3 -D switches, e.g. "BS_Y=-100 I4=0"
        string extraDefines = default("");     // appended to defines (handy in derived configs)
        string label = default("");            // free text, e.g. the scenario name (recorded as a run attribute)
        double roundDuration @unit(s) = default(1s);
        string csvDir = default("");           // write the ns-3 style CSV files here ("" = off)
        bool verbose = default(false);         // print the protocol's per-round console output to the log
        bool drawLinks = default(true);        // GUI: draw member -> CH and CH -> BS links
        double scale = default(4);             // GUI: pixels per metre
        @display("i=block/cogwheel");
}
