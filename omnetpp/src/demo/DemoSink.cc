#include "DemoSink.h"

#include "DemoSensor.h"

using namespace omnetpp;

namespace wsn {

Define_Module(DemoSink);

void DemoSink::initialize()
{
    px = par("x").doubleValueInUnit("m");
    py = par("y").doubleValueInUnit("m");
    rounds = par("rounds");
    roundTime = par("roundTime");
    slotTime = par("slotTime");
    pxPerMetre = getParentModule()->par("scale").doubleValue();
    getDisplayString().setTagArg("p", 0, px * pxPerMetre);
    getDisplayString().setTagArg("p", 1, py * pxPerMetre);

    cModule* net = getParentModule();
    for (int i = 0; i < net->getSubmoduleVectorSize("sensor"); ++i)
        sensors.push_back(check_and_cast<DemoSensor*>(net->getSubmodule("sensor", i)));
    servedThisEpoch.assign(sensors.size(), false);
    WATCH(round);
    WATCH(generated);
    WATCH(delivered);

    roundTimer = new cMessage("round");
    scheduleAt(0, roundTimer);
}

void DemoSink::startRound()
{
    if (++round > rounds) return;
    const int nClusters = 3;
    const simtime_t t0 = simTime();

    // SET-UP: in each cluster the node with the most energy that has not been CH
    // in this epoch (LEACH's G set) becomes CH; when all have served, a new epoch starts.
    std::vector<DemoSensor*> ch(nClusters, nullptr);
    for (int c = 0; c < nClusters; ++c) {
        bool anyEligible = false;
        for (size_t i = 0; i < sensors.size(); ++i)
            if (sensors[i]->cluster() == c && !servedThisEpoch[i] && sensors[i]->alive()) anyEligible = true;
        if (!anyEligible)
            for (size_t i = 0; i < sensors.size(); ++i)
                if (sensors[i]->cluster() == c) servedThisEpoch[i] = false;
        double best = -1;
        int bestIdx = -1;
        for (size_t i = 0; i < sensors.size(); ++i)
            if (sensors[i]->cluster() == c && !servedThisEpoch[i] && sensors[i]->alive() && sensors[i]->energy() > best) {
                best = sensors[i]->energy();
                bestIdx = i;
            }
        if (bestIdx >= 0) { servedThisEpoch[bestIdx] = true; ch[c] = sensors[bestIdx]; }
    }

    EV_INFO << "=== Round " << round << " (t = " << t0 << ")  CHs:";
    for (int c = 0; c < nClusters; ++c)
        EV_INFO << " cluster" << c << "->" << (ch[c] ? ch[c]->getFullName() : "none");
    EV_INFO << " ===\n";

    // MEMBERS: one TDMA slot each (the 3 clusters use their slots in turn); CHs fuse at t0 + 7 s
    std::vector<int> slotInCluster(nClusters, 0);
    for (DemoSensor* s : sensors) {
        const int c = s->cluster();
        std::vector<DemoSensor*> members;
        for (DemoSensor* m : sensors)
            if (m->cluster() == c && m != ch[c]) members.push_back(m);
        const bool isCH = (s == ch[c]);
        const simtime_t sendAt = t0 + 1.0 + (slotInCluster[c] * nClusters + c) * slotTime;
        if (!isCH) ++slotInCluster[c];
        s->startRound(round, isCH, ch[c], 0, sendAt, t0 + 7.0 + c * slotTime, members);
    }
    scheduleAt(t0 + roundTime, roundTimer);
}

void DemoSink::handleMessage(cMessage* msg)
{
    if (msg == roundTimer) { startRound(); return; }
    auto* f = check_and_cast<cPacket*>(msg);
    if (f->getKind() == FRAME_FUSED) {
        const int readings = (int)f->par("readings").longValue();
        delivered += readings;
        ++packetsAtSink;
        deliveredVec.record(delivered);
        EV_INFO << "SINK <- " << f->getSenderModule()->getFullName() << " : fused packet with " << readings
                << " readings\n";
    }
    delete f;
}

void DemoSink::refreshDisplay() const
{
    char label[80];
    snprintf(label, sizeof(label), "SINK | round %d | readings: %d", round > rounds ? rounds : round, delivered);
    getDisplayString().setTagArg("t", 0, label);
}

void DemoSink::finish()
{
    recordScalar("readingsGenerated", generated);
    recordScalar("readingsDelivered", delivered);
    recordScalar("packetsAtSink", packetsAtSink);
    recordScalar("PDR", generated ? (double)delivered / generated : 0.0);
    EV_INFO << "\n========== DEMO RESULTS ==========\n"
            << "Rounds                = " << rounds << "\n"
            << "Readings generated    = " << generated << "\n"
            << "Readings at the sink  = " << delivered << "\n"
            << "PDR (readings)        = " << (generated ? 100.0 * delivered / generated : 0.0) << " %\n"
            << "Packets to the sink   = " << packetsAtSink << "  (instead of " << generated << " without clustering)\n";
}

} // namespace wsn
