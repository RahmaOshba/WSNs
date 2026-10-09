#pragma once

#include <omnetpp.h>

#include <functional>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/common/L3Address.h"

#include "WsnFrame_m.h"

namespace pktwsn {

class NodeAgent;

// Protocol logic of the packet-level simulation (LEACH or v8 / v8-Chain).
// The decisions are the ones of the thesis code (code/2_EDITED/leach_EDITED.cc,
// code/4_PROPOSED/v8_chain_center.cc); what changes is that every message is a
// real IEEE 802.15.4 frame, so a node only knows what it actually received:
// it joins a CH whose ADV it heard, a member is in the schedule only if its JOIN
// arrived, data count only when the frame reaches the CH / BS, and every frame
// (including MAC retransmissions and ACKs) costs energy.
class Brain : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  public:
    static const int BROADCAST = -2;
    static const int BS = -1;

    ~Brain() override;
    void registerNode(NodeAgent* agent, const inet::MacAddress& mac);
    void onFrame(int at, const WsnFrame& frame);
    void onDeath(int node);
    int payloadBits(int type) const;
    bool compressHeaders() const { return sixLowpan; }
    int compressedHeaderBytes() const { return compressedBytes; }
    double distance(int a, int b) const;
    int indexOfMac(const inet::MacAddress& mac) const;
    inet::L3Address ipOf(int node);

  protected:
    int numInitStages() const override { return 2; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage* msg) override;
    void finish() override;
    void refreshDisplay() const override;
    void receiveSignal(omnetpp::cComponent* source, omnetpp::simsignal_t signal, omnetpp::cObject* obj, omnetpp::cObject* details) override;

  private:
    struct Node {
        double x = 0, y = 0;
        bool ch = false;
        int head = -1;                 // current CH (-1 = none)
        bool selectedThisEpoch = false;
        int degree = 0;
        std::set<int> heardAdv;        // CHs whose ADV this node received in this set-up
        bool scheduled = false;        // in its CH's TDMA schedule (JOIN arrived and schedule received)
    };
    struct Cluster {                   // per CH, this interval / round
        std::vector<int> joined;       // members whose JOIN arrived
        std::map<int, double> memberEnergy;
        int backup = -1;
        bool failedOver = false;
        int recv = 0, inPackets = 0, inSources = 0;
        bool sent = false;
    };

    void at(omnetpp::simtime_t t, std::function<void()> fn);
    bool alive(int i) const;
    void setAwake(int i, bool awake);
    double residual(int i) const;
    void checkPositions();
    void startRound();
    omnetpp::simtime_t setupPhase(omnetpp::simtime_t t);
    void dataPhase();
    void memberSend(int i);
    void chPhase(omnetpp::simtime_t t);
    void chSend(int c);
    void endRound();

    void electV8();
    void electLeach();
    std::vector<int> computeNextHop() const;
    double expectedCHCost(int ch, int members) const;
    bool proactiveHandover(int ch, omnetpp::simtime_t t);
    void tryFailover(int deadCH);
    bool lowEnergy(double e) const { return e < 0.05 * avgEnergy; }
    int nearestHeardCH(int i) const;
    void colorClusters();

    // parameters
    bool isV8 = true;
    int N = 100, setupInterval = 5, epoch = 20, chainMode = 2, maxRounds = 5000;
    double area = 100, bsX = 50, bsY = 50, E0 = 0.5, pCH = 0.05, nbRange = 25, advRange = 0;
    bool I1 = true, I2 = true, I3 = true, I4 = true, I5 = true, chargeHello = true, chargeBeacon = true;
    int dataBits = 2000, controlBits = 200;
    bool perClusterSlots = true;
    bool sixLowpan = true;
    int compressedBytes = 6;
    double advWindow, joinWindow, schedWindow, guard, handoverWindow, dataSlot, aggSlot, slotJitter, wakeAhead;

    // state
    std::vector<Node> nodes;
    std::vector<Cluster> clusters;
    std::vector<NodeAgent*> agents;
    NodeAgent* sinkAgent = nullptr;
    std::map<inet::MacAddress, int> macIndex;
    std::vector<inet::L3Address> ipCache;
    inet::L3Address sinkIp;
    std::mt19937 electionRng, jitterRng;
    int round = 0, setupIndex = 0;
    double avgEnergy = 0.5;
    int roundGenerated = 0, roundDelivered = 0;
    std::vector<int> nextHop;

    // results
    int FND = 0, HND = 0, LND = 0, lastAlive = 0, lastCH = 0;
    long totalGenerated = 0, totalDelivered = 0;
    long retryDrops = 0, backoffDrops = 0, corrupted = 0, handovers = 0, failovers = 0, directToBS = 0, relayed = 0;
    std::map<std::string, long> dropNames;
    std::map<std::string, long> frameCount;   // frames sent / received per type (control traffic check)
    omnetpp::cOutVector aliveVec{"alive"}, chVec{"chCount"}, pdrVec{"pdr"}, residualVec{"residualEnergy"};

    struct Action : public omnetpp::cMessage { std::function<void()> fn; Action() : omnetpp::cMessage("action") {} };
};

} // namespace pktwsn
