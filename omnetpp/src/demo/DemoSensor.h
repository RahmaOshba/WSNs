#pragma once

#include <omnetpp.h>

#include <vector>

namespace wsn {

// A frame of the demo. kind = what it carries.
enum DemoFrameKind { FRAME_ADV = 1, FRAME_READING = 2, FRAME_FUSED = 3 };

// Energy: the thesis' first-order radio model.
double DemoTxEnergy(double bits, double distance);
double DemoRxEnergy(double bits);
extern const double DEMO_BITRATE;   // 250 kb/s, IEEE 802.15.4
extern const double DEMO_E_DA;      // data aggregation energy per bit

class DemoSink;

class DemoSensor : public omnetpp::cSimpleModule
{
  public:
    double x() const { return px; }
    double y() const { return py; }
    int cluster() const { return clusterId; }
    double energy() const { return residual; }
    bool alive() const { return residual > 0; }

    // called by the sink at the start of a round
    void startRound(int round, bool isCH, DemoSensor* ch, int slotIndex, omnetpp::simtime_t slotTime,
                    omnetpp::simtime_t fuseTime, const std::vector<DemoSensor*>& members);

    // radio
    void transmit(omnetpp::cPacket* frame, omnetpp::cModule* to, double distance);
    void spend(double joules);

    int sent = 0, received = 0;

  protected:
    void initialize() override;
    void handleMessage(omnetpp::cMessage* msg) override;
    void finish() override;

  private:
    void refreshDisplay() const override;
    void sendReading();
    void sendFused();

    double px = 0, py = 0, residual = 0, initial = 0;
    int clusterId = 0, round = 0;
    bool isCH = false;
    DemoSensor* myCH = nullptr;
    DemoSink* sink = nullptr;
    std::vector<DemoSensor*> members;
    int readingsHere = 0;
    omnetpp::cMessage* sendTimer = nullptr;
    omnetpp::cMessage* fuseTimer = nullptr;
    omnetpp::cMessage* advTimer = nullptr;
};

} // namespace wsn
