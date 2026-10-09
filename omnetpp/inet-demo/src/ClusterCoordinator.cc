#include "ClusterCoordinator.h"

#include "ClusterApp.h"

using namespace omnetpp;

namespace inetdemo {

Define_Module(ClusterCoordinator);

void ClusterCoordinator::initialize(int stage)
{
    if (stage != 0) return;
    rounds = par("rounds");
    roundTime = par("roundTime");
    slotTime = par("slotTime");
    numClusters = par("numClusters");
    cModule* net = getSystemModule();
    const int n = net->getSubmoduleVectorSize("sensor");
    sensorsPerCluster = n / numClusters;
    for (int i = 0; i < n; ++i)
        apps.push_back(check_and_cast<ClusterApp*>(net->getSubmodule("sensor", i)->getSubmodule("app", 0)));
    servedThisEpoch.assign(n, false);
    WATCH(round);
    WATCH(generated);
    WATCH(delivered);
    roundTimer = new cMessage("round");
    scheduleAt(0, roundTimer);
}

void ClusterCoordinator::startRound()
{
    if (++round > rounds) return;
    const simtime_t t0 = simTime();
    const int n = apps.size();
    std::vector<int> ch(numClusters, -1);
    for (int c = 0; c < numClusters; ++c) {
        bool anyEligible = false;
        for (int i = 0; i < n; ++i)
            if (clusterOf(i) == c && !servedThisEpoch[i]) anyEligible = true;
        if (!anyEligible)
            for (int i = 0; i < n; ++i)
                if (clusterOf(i) == c) servedThisEpoch[i] = false;
        double best = -1;
        for (int i = 0; i < n; ++i)
            if (clusterOf(i) == c && !servedThisEpoch[i] && apps[i]->residualEnergy() > best) {
                best = apps[i]->residualEnergy();
                ch[c] = i;
            }
        if (ch[c] >= 0) servedThisEpoch[ch[c]] = true;
    }
    EV_INFO << "=== Round " << round << " (t = " << t0 << ")  CHs:";
    for (int c = 0; c < numClusters; ++c)
        EV_INFO << " cluster" << c << "->sensor[" << ch[c] << "]";
    EV_INFO << " ===\n";

    std::vector<int> slotInCluster(numClusters, 0);
    for (int i = 0; i < n; ++i) {
        const int c = clusterOf(i);
        const bool isCH = (i == ch[c]);
        const simtime_t sendAt = t0 + 1.0 + (slotInCluster[c] * numClusters + c) * slotTime;
        if (!isCH) ++slotInCluster[c];
        apps[i]->startRound(round, isCH, ch[c], sendAt, t0 + 7.0 + c * slotTime);
    }
    scheduleAt(t0 + roundTime, roundTimer);
}

void ClusterCoordinator::handleMessage(cMessage* msg)
{
    if (msg == roundTimer) startRound();
    else delete msg;
}

void ClusterCoordinator::refreshDisplay() const
{
    char buf[80];
    snprintf(buf, sizeof(buf), "round %d | readings at sink: %d / %d", round > rounds ? rounds : round, delivered, generated);
    getDisplayString().setTagArg("t", 0, buf);
}

void ClusterCoordinator::finish()
{
    recordScalar("readingsGenerated", generated);
    recordScalar("readingsDelivered", delivered);
    recordScalar("packetsAtSink", packetsAtSink);
    recordScalar("PDR", generated ? (double)delivered / generated : 0.0);
    EV_INFO << "\n========== DEMO RESULTS (INET, IEEE 802.15.4) ==========\n"
            << "Rounds                = " << rounds << "\n"
            << "Readings generated    = " << generated << "\n"
            << "Readings at the sink  = " << delivered << "\n"
            << "PDR (readings)        = " << (generated ? 100.0 * delivered / generated : 0.0) << " %\n"
            << "Packets to the sink   = " << packetsAtSink << "  (instead of " << generated << " without clustering)\n";
}

} // namespace inetdemo
