#pragma once

#include <omnetpp.h>

#include <vector>

namespace wsn {

class DemoSensor;

class DemoSink : public omnetpp::cSimpleModule
{
  public:
    double x() const { return px; }
    double y() const { return py; }
    void readingGenerated() { ++generated; }
    double scale() const { return pxPerMetre; }

  protected:
    void initialize() override;
    void handleMessage(omnetpp::cMessage* msg) override;
    void finish() override;
    void refreshDisplay() const override;

  private:
    void startRound();

    double px = 0, py = 0, pxPerMetre = 8;
    int rounds = 0, round = 0;
    int generated = 0, delivered = 0, packetsAtSink = 0;
    omnetpp::simtime_t roundTime, slotTime;
    std::vector<DemoSensor*> sensors;
    std::vector<bool> servedThisEpoch;
    omnetpp::cMessage* roundTimer = nullptr;
    omnetpp::cOutVector deliveredVec{"readingsDelivered"};
};

} // namespace wsn
