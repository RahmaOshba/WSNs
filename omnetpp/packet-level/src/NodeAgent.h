#pragma once

#include "inet/applications/base/ApplicationBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "WsnFrame_m.h"

namespace pktwsn {

class Brain;

// The node side of the packet-level simulation (one per sensor, one on the sink).
// - sends the frames the protocol asks for over UDP/IPv4/IEEE 802.15.4;
// - hands every received frame to the Brain (the protocol logic);
// - keeps the battery: every frame the 802.15.4 MAC really transmits
//   (data, retransmissions, ACKs) or receives for this node is charged with
//   the first-order radio model; broadcast frames are charged by the Brain
//   only for the nodes that are meant to listen (TDMA: the others sleep).
class NodeAgent : public inet::ApplicationBase, public inet::UdpSocket::ICallback, public omnetpp::cListener
{
  public:
    ~NodeAgent() override;
    int index() const { return nodeIndex; }   // -1 = sink
    bool isSink() const { return nodeIndex < 0; }
    bool alive() const { return isAlive; }
    double residual() const { return energy; }
    void setInitialEnergy(double e) { energy = initialEnergy = e; }

    // protocol commands (from the Brain)
    bool sendFrame(int type, int dest, double broadcastRange, int sources, double value, int roundNo);
    void spend(double joules);                // aggregation, broadcast reception, ...
    // TDMA: the radio sleeps when the node has nothing to send or receive
    // (the 802.15.4 MAC wakes it by itself to send; it is put back to sleep when idle)
    void setAwake(bool awake);
    void setRole(bool ch, const char* color) { isCH = ch; clusterColor = color ? color : ""; }

    int frameBits(int type) const;            // on-air size of a frame of this type (for estimates)

    // counters
    long macTx = 0, macRx = 0, appSent = 0;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessageWhenUp(omnetpp::cMessage* msg) override;
    void finish() override;
    void refreshDisplay() const override;

    void handleStartOperation(inet::LifecycleOperation*) override;
    void handleStopOperation(inet::LifecycleOperation*) override { socket.close(); }
    void handleCrashOperation(inet::LifecycleOperation*) override { socket.destroy(); }

    void socketDataArrived(inet::UdpSocket*, inet::Packet* packet) override;
    void socketErrorArrived(inet::UdpSocket*, inet::Indication* indication) override { delete indication; }
    void socketClosed(inet::UdpSocket*) override {}

    void receiveSignal(omnetpp::cComponent* source, omnetpp::simsignal_t signal, omnetpp::cObject* obj, omnetpp::cObject* details) override;

  private:
    void die();
    double chargedBits(const inet::Packet* packet, int64_t macHeaderBits) const;
    void applyRadioMode();
    bool wantAwake = true;
    omnetpp::cMessage* sleepRetry = nullptr;
    void unsubscribeMac();

    Brain* brain = nullptr;
    inet::UdpSocket socket;
    int port = 6000;
    int nodeIndex = -1;
    double energy = 0, initialEnergy = 0;
    bool isAlive = true, isCH = false;
    std::string clusterColor;
    double broadcastRange = 0;      // TX distance of this node's current broadcast
    inet::MacAddress myMac;
    omnetpp::cModule* mac = nullptr;
    omnetpp::cModule* radioModule = nullptr;
    int deathRound = 0;
};

} // namespace pktwsn
