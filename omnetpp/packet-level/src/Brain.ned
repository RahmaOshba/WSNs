package pktwsn;

// Protocol logic of the packet-level runs (see Brain.h).
simple Brain
{
    parameters:
        string protocol = default("v8");        // "v8" (v8 / v8-Chain) or "leach" (LEACH EDITED)
        int numNodes = default(100);
        double area @unit(m) = default(100m);
        double bsX @unit(m) = default(50m);
        double bsY @unit(m) = default(50m);
        double initialEnergy @unit(J) = default(0.5J);
        int seed = default(12345);               // topology = mt19937(seed), election = mt19937(seed + 1), as in the thesis code
        double pCH = default(0.05);
        int epoch = default(20);
        int setupInterval = default(5);          // v8: clusters reused for 5 rounds
        double neighbourRange @unit(m) = default(25m);
        int chainMode = default(2);              // v8: 0 direct, 1 ordered chain, 2 energy-aware relay (v8-Chain)
        bool I1 = default(true);
        bool I2 = default(true);
        bool I3 = default(true);
        bool I4 = default(true);
        bool I5 = default(true);
        bool chargeHello = default(true);        // v8: one HELLO per node at deployment
        bool chargeBeacon = default(true);       // v8: BS average-energy beacon + 16-bit energy field in the data
        int dataBits = default(2000);
        int controlBits = default(200);
        int maxRounds = default(5000);
        string headerModel = default("6lowpan");  // energy of the network headers: "6lowpan" = IPv4+UDP counted as a 6LoWPAN compressed header; "ipv4" = full 28 B
        int compressedHeaderBytes = default(6);    // 6LoWPAN IPHC + UDP NHC, typical size
        string slotMode = default("global");     // "global": one TDMA slot per node in the whole network (802.15.4 has one shared channel); "perCluster": every cluster uses its slots at the same time (needs one channel per cluster, as LEACH assumes with CDMA) -> contention
        // timing of a round
        double advWindow @unit(s) = default(50ms);
        double joinWindow @unit(s) = default(1.5s);   // JOINs are sent with CSMA at a random time in this window; 0.4 s gives ~11% JOIN collisions (config ShortJoinWindow)
        double schedWindow @unit(s) = default(50ms);
        double guard @unit(s) = default(20ms);
        double handoverWindow @unit(s) = default(30ms);
        double dataSlot @unit(s) = default(35ms);   // a 2000-bit reading is 3 IEEE 802.15.4 frames (max 127 B each) + their ACKs
        double aggSlot @unit(s) = default(40ms);
        double slotJitter @unit(s) = default(2ms);
        double wakeAhead @unit(s) = default(5ms);   // a member wakes this long before its slot (TDMA guard time)
        @display("i=block/cogwheel");
}
