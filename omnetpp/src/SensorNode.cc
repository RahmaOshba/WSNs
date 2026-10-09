#include "SensorNode.h"

using namespace omnetpp;

namespace wsn {

Define_Module(SensorNode);

void SensorNode::initialize()
{
    WATCH(energy);
    WATCH(alive);
    WATCH(roundsAsCH);
    WATCH(deathRound);
}

void SensorNode::update(const NodeView& n, uint32_t round, const char* clusterColor)
{
    if (initialEnergy < 0) initialEnergy = n.energy;
    energy = n.energy;
    energyVec.record(n.energy);

    if (alive && !n.alive) deathRound = round;
    alive = n.alive;
    if (n.isCH) { ++roundsAsCH; if (!wasCH) ++timesElectedCH; }
    wasCH = n.isCH;

    if (!getEnvir()->isGUI()) return;
    cDisplayString& ds = getDisplayString();
    if (!alive) {
        ds.setTagArg("i", 0, "misc/node_vs");
        ds.setTagArg("i", 1, "black");
        ds.setTagArg("i", 2, "80");
        ds.setTagArg("t", 0, "");
        ds.setTagArg("tt", 0, "dead");
        return;
    }
    ds.setTagArg("i", 0, n.isCH ? "misc/node_s" : "misc/node_vs");
    ds.setTagArg("i", 1, clusterColor && *clusterColor ? clusterColor : "grey");
    ds.setTagArg("i", 2, clusterColor && *clusterColor ? "70" : "30");
    // only CHs carry a text label (100 labels would cover the field); every node shows its energy as a tooltip
    char buf[48];
    snprintf(buf, sizeof(buf), "%s%.3f J", n.isCH ? "CH " : "", n.energy);
    ds.setTagArg("t", 0, n.isCH ? buf : "");
    ds.setTagArg("tt", 0, buf);
}

void SensorNode::finish()
{
    recordScalar("deathRound", deathRound);   // 0 = still alive at the end
    recordScalar("roundsAsCH", roundsAsCH);
    recordScalar("timesElectedCH", timesElectedCH);
    recordScalar("residualEnergy", energy < 0 ? 0 : energy, "J");
}

} // namespace wsn
