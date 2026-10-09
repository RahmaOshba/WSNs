#include "NodeAgent.h"

#include "Brain.h"
#include "WsnCommon.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ieee802154/Ieee802154MacHeader_m.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/ITransmission.h"

using namespace omnetpp;
using namespace inet;

namespace pktwsn {

Define_Module(NodeAgent);

// UDP (8 B) + IPv4 (20 B) + 802.15.4 MAC header and FCS (9 B, INET default 72 bits)
static const int STACK_OVERHEAD_BYTES = 8 + 20 + 9;
// 802.15.4 PHY: preamble 4 B + SFD 1 B + PHR 1 B, on the air with every frame
static const int PHY_OVERHEAD_BITS = 48;

NodeAgent::~NodeAgent()
{
    cancelAndDelete(sleepRetry);
}

void NodeAgent::unsubscribeMac()
{
    if (mac) {
        mac->unsubscribe(packetReceivedFromLowerSignal, this);
        mac = nullptr;
    }
    if (radioModule) {
        radioModule->unsubscribe(transmissionStartedSignal, this);
        radioModule = nullptr;
    }
}

static physicallayer::IRadio* radioOf(cModule* host)
{
    return dynamic_cast<physicallayer::IRadio*>(host->getSubmodule("wlan", 0)->getSubmodule("radio"));
}

void NodeAgent::setAwake(bool awake)
{
    Enter_Method("setAwake");
    wantAwake = awake;
    applyRadioMode();
}

void NodeAgent::applyRadioMode()
{
    if (!isAlive || isSink()) return;
    auto* radio = radioOf(getContainingNode(this));
    if (!radio) return;
    using physicallayer::IRadio;
    if (wantAwake) {
        if (radio->getRadioMode() == IRadio::RADIO_MODE_SLEEP) radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        return;
    }
    if (radio->getRadioMode() == IRadio::RADIO_MODE_SLEEP) return;
    // never cut a frame: wait until the radio is idle
    // (in receiver mode the transmission state is "undefined"; transmitting means transmitter mode)
    if (radio->getRadioMode() != IRadio::RADIO_MODE_RECEIVER || radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING) {
        if (!sleepRetry->isScheduled()) scheduleAfter(0.002, sleepRetry);
        return;
    }
    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
}

void NodeAgent::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        port = par("port");
        brain = check_and_cast<Brain*>(getModuleByPath(par("brainModule").stringValue()));
        cModule* host = getContainingNode(this);
        nodeIndex = host->isVector() ? host->getIndex() : -1;
        sleepRetry = new cMessage("sleepRetry");
        WATCH(energy);
        WATCH(isAlive);
        WATCH(isCH);
        WATCH(macTx);
        WATCH(macRx);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        cModule* host = getContainingNode(this);
        cModule* wlan = host->getSubmodule("wlan", 0);
        myMac = check_and_cast<NetworkInterface*>(wlan)->getMacAddress();
        brain->registerNode(this, myMac);
        // every frame on the air (data, fragments, retransmissions, ACKs) starts at the radio;
        // every frame received comes up to the MAC
        mac = wlan->getSubmodule("mac");
        mac->subscribe(packetReceivedFromLowerSignal, this);
        radioModule = wlan->getSubmodule("radio");
        radioModule->subscribe(transmissionStartedSignal, this);
    }
}

void NodeAgent::handleStartOperation(LifecycleOperation*)
{
    socket.setOutputGate(gate("socketOut"));
    socket.setCallback(this);
    socket.bind(port);
    socket.setBroadcast(true);
}

int NodeAgent::frameBits(int type) const
{
    // one-frame estimate (control frames); used for broadcast receptions and decisions
    const int netBytes = brain->compressHeaders() ? brain->compressedHeaderBytes() : 28;
    return brain->payloadBits(type) + 8 * (9 + netBytes) + PHY_OVERHEAD_BITS;
}

bool NodeAgent::sendFrame(int type, int dest, double range, int sources, double value, int roundNo)
{
    Enter_Method("sendFrame");
    if (!isAlive) return false;
    auto frame = makeShared<WsnFrame>();
    frame->setType(type);
    frame->setSrc(nodeIndex);
    frame->setRound(roundNo);
    frame->setSources(sources);
    frame->setEnergy(energy);
    frame->setValue(value);
    frame->setChunkLength(b(brain->payloadBits(type)));
    static const char* names[] = {"", "HELLO", "BEACON", "ADV", "JOIN", "SCHED", "HANDOVER", "DATA", "AGG"};
    auto* packet = new Packet(names[type], frame);
    L3Address to;
    if (dest == Brain::BROADCAST) {
        broadcastRange = range;
        to = Ipv4Address::ALLONES_ADDRESS;
    }
    else {
        to = brain->ipOf(dest);
    }
    socket.sendTo(packet, to, port);
    ++appSent;
    return true;
}

void NodeAgent::handleMessageWhenUp(cMessage* msg)
{
    if (msg == sleepRetry) applyRadioMode();
    else if (msg->arrivedOn("socketIn")) socket.processMessage(msg);
    else delete msg;
}

void NodeAgent::socketDataArrived(UdpSocket*, Packet* packet)
{
    if (isAlive) {
        auto frame = packet->peekAtFront<WsnFrame>();
        brain->onFrame(nodeIndex, *frame);
    }
    delete packet;
}

// Energy of every frame the MAC really sends / receives.
void NodeAgent::receiveSignal(cComponent*, simsignal_t signal, cObject* obj, cObject*)
{
    if (isSink() || !isAlive) return;
    const Packet* packet = nullptr;
    if (signal == transmissionStartedSignal) packet = check_and_cast<const physicallayer::ITransmission*>(obj)->getPacket();
    else packet = dynamic_cast<Packet*>(obj);
    if (!packet) return;
    auto header = packet->peekAtFront<Ieee802154MacHeader>();
    const MacAddress dest = header->getDestAddr();
    const double bits = chargedBits(packet, header->getChunkLength().get<b>());
    if (signal == transmissionStartedSignal) {
        ++macTx;
        double d;
        if (dest.isBroadcast() || dest.isMulticast()) d = broadcastRange;
        else d = brain->distance(nodeIndex, brain->indexOfMac(dest));
        spend(RadioModel::tx(bits, d));
    }
    else if (signal == packetReceivedFromLowerSignal) {
        // unicast frames for this node (data, join, fused packets, ACKs);
        // broadcasts are charged by the Brain for the nodes that listen to them
        if (dest == myMac) {
            ++macRx;
            spend(RadioModel::rx(bits));
        }
    }
}

// Bits of a frame on the air. With headerModel = "6lowpan" the IPv4 (20 B) and UDP (8 B) headers
// that INET puts in the frame are counted as a 6LoWPAN compressed header (compressedHeaderBytes),
// as real 802.15.4 sensor networks do; "ipv4" counts them in full.
double NodeAgent::chargedBits(const Packet* packet, int64_t macHeaderBits) const
{
    double bits = packet->getBitLength() + PHY_OVERHEAD_BITS;
    if (!brain->compressHeaders() || packet->getBitLength() <= macHeaderBits + 160) return bits;   // ACK: no network header
    try {
        auto ip = packet->peekAt<Ipv4Header>(b(macHeaderBits));
        const bool firstFragment = ip->getFragmentOffset() == 0;
        bits -= 8 * (20 + (firstFragment ? 8 : 0));
        bits += 8 * brain->compressedHeaderBytes();
    }
    catch (std::exception&) {}
    return bits;
}

void NodeAgent::spend(double joules)
{
    if (isSink() || !isAlive) return;
    energy -= joules;
    if (energy <= 0) {
        energy = 0;
        die();
    }
}

void NodeAgent::die()
{
    isAlive = false;
    isCH = false;
    // the radio is switched off: a dead node neither sends nor receives
    if (sleepRetry) cancelEvent(sleepRetry);
    if (auto* r = radioOf(getContainingNode(this)))
        r->setRadioMode(physicallayer::IRadio::RADIO_MODE_OFF);
    brain->onDeath(nodeIndex);
}

void NodeAgent::refreshDisplay() const
{
    if (isSink()) return;
    cDisplayString& ds = getContainingNode(this)->getDisplayString();
    char buf[48];
    snprintf(buf, sizeof(buf), "%s%.3f J", isCH ? "CH " : "", energy);
    if (!isAlive) {
        ds.setTagArg("i", 1, "black");
        ds.setTagArg("i", 2, "90");
        ds.setTagArg("t", 0, "");
        ds.setTagArg("tt", 0, "dead");
        return;
    }
    ds.setTagArg("i", 1, isCH ? "red" : (clusterColor.empty() ? "grey" : clusterColor.c_str()));
    ds.setTagArg("i", 2, isCH ? "90" : (clusterColor.empty() ? "20" : "60"));
    ds.setTagArg("is", 0, isCH ? "s" : "vs");
    ds.setTagArg("t", 0, isCH ? buf : "");
    ds.setTagArg("tt", 0, buf);
}

void NodeAgent::finish()
{
    unsubscribeMac();
    if (!isSink()) {
        recordScalar("residualEnergy", energy, "J");
        recordScalar("macFramesSent", macTx);
        recordScalar("macFramesReceived", macRx);
    }
    ApplicationBase::finish();
}

} // namespace pktwsn
