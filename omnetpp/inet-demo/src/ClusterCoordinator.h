#pragma once

#include <omnetpp.h>

#include <vector>

namespace inetdemo {

class ClusterApp;

// Runs the rounds of the demo, exactly like the ns-3 demo (a global set-up step):
// in each cluster, the sensor with the most residual energy that has not been
// CH in this epoch (LEACH's G set) becomes CH. Members get a TDMA slot each,
// the CHs fuse and send to the sink at the end of the round.
class ClusterCoordinator : public omnetpp::cSimpleModule
{
  public:
    void readingGenerated() { ++generated; }
    void readingsDelivered(int n) { delivered += n; ++packetsAtSink; deliveredVec.record(delivered); }
    int clusterOf(int sensor) const { return sensor / sensorsPerCluster; }

  protected:
    int numInitStages() const override { return 20; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage* msg) override;
    void finish() override;
    void refreshDisplay() const override;

  private:
    void startRound();

    int rounds = 0, round = 0, numClusters = 3, sensorsPerCluster = 4;
    omnetpp::simtime_t roundTime, slotTime;
    std::vector<ClusterApp*> apps;
    std::vector<bool> servedThisEpoch;
    int generated = 0, delivered = 0, packetsAtSink = 0;
    omnetpp::cMessage* roundTimer = nullptr;
    omnetpp::cOutVector deliveredVec{"readingsDelivered"};
};

} // namespace inetdemo
