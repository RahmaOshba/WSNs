#pragma once

#include "inet/applications/base/ApplicationBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace inetdemo {

class ClusterCoordinator;

// Application of a sensor (or of the sink, parameter isSink) in the INET demo.
// Frames go over UDP/IPv4 on the IEEE 802.15.4 interface of the node,
// so they use the real 802.15.4 CSMA/CA MAC with ACKs and the radio channel.
class ClusterApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  public:
    // the coordinator tells every sensor its role for this round
    void startRound(int round, bool isCH, int chIndex, omnetpp::simtime_t sendAt, omnetpp::simtime_t fuseAt);
    double residualEnergy() const;
    int sent = 0, received = 0;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessageWhenUp(omnetpp::cMessage* msg) override;
    void finish() override;
    void refreshDisplay() const override;

    void handleStartOperation(inet::LifecycleOperation*) override;
    void handleStopOperation(inet::LifecycleOperation*) override;
    void handleCrashOperation(inet::LifecycleOperation*) override;

    void socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet) override;
    void socketErrorArrived(inet::UdpSocket* socket, inet::Indication* indication) override;
    void socketClosed(inet::UdpSocket* socket) override {}

  private:
    void sendMsg(int type, const inet::L3Address& dest, int readings);
    inet::L3Address addressOfSensor(int index) const;

    inet::UdpSocket socket;
    int port = 5000;
    bool isSink = false;
    int index = -1, cluster = -1, round = 0;
    bool isCH = false;
    int chIndex = -1, readingsHere = 0;
    ClusterCoordinator* coordinator = nullptr;
    omnetpp::cMessage* advTimer = nullptr;
    omnetpp::cMessage* sendTimer = nullptr;
    omnetpp::cMessage* fuseTimer = nullptr;
};

} // namespace inetdemo
