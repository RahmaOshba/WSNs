#pragma once

#include <omnetpp.h>

#include "WsnRuntime.h"

namespace wsn {

// A sensor node. Its state (energy, role, cluster) is set by the protocol
// engine through the Controller after every round; the module shows it on
// the GUI and records per-node statistics.
class SensorNode : public omnetpp::cSimpleModule
{
  public:
    void update(const NodeView& n, uint32_t round, const char* clusterColor);

  protected:
    void initialize() override;
    void handleMessage(omnetpp::cMessage* msg) override { delete msg; }
    void finish() override;

  private:
    omnetpp::cOutVector energyVec{"energy"};
    double initialEnergy = -1.0;
    double energy = 0.0;
    bool alive = true;
    bool wasCH = false;
    uint32_t deathRound = 0;
    uint32_t roundsAsCH = 0;
    uint32_t timesElectedCH = 0;
};

} // namespace wsn
