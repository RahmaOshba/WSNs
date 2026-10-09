#include "ClusterApp.h"

#include "ClusterCoordinator.h"
#include "ClusterMsg_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/power/contract/IEpEnergyStorage.h"

using namespace omnetpp;
using namespace inet;

namespace inetdemo {

Define_Module(ClusterApp);

static const char* const kColor[] = {"#3c8cff", "#28be6e", "#aa5ae6"};

void ClusterApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        port = par("port");
        isSink = par("isSink");
        coordinator = check_and_cast<ClusterCoordinator*>(getModuleByPath(par("coordinatorModule").stringValue()));
        cModule* host = getContainingNode(this);
        index = isSink ? -1 : host->getIndex();
        cluster = isSink ? -1 : coordinator->clusterOf(index);
        advTimer = new cMessage("advertise");
        sendTimer = new cMessage("sendReading");
        fuseTimer = new cMessage("fuseAndSend");
        WATCH(sent);
        WATCH(received);
        WATCH(isCH);
    }
}

double ClusterApp::residualEnergy() const
{
    auto* storage = dynamic_cast<power::IEpEnergyStorage*>(getContainingNode(this)->getSubmodule("energyStorage"));
    return storage ? storage->getResidualEnergyCapacity().get<J>() : 0.0;
}

void ClusterApp::handleStartOperation(LifecycleOperation*)
{
    socket.setOutputGate(gate("socketOut"));
    socket.setCallback(this);
    socket.bind(port);
    socket.setBroadcast(true);
}

void ClusterApp::handleStopOperation(LifecycleOperation*) { socket.close(); }
void ClusterApp::handleCrashOperation(LifecycleOperation*) { socket.destroy(); }

L3Address ClusterApp::addressOfSensor(int i) const
{
    return L3AddressResolver().resolve(getSimulation()->getSystemModule()->getSubmodule("sensor", i)->getFullPath().c_str());
}

void ClusterApp::startRound(int r, bool ch, int head, simtime_t sendAt, simtime_t fuseAt)
{
    Enter_Method("startRound");
    round = r;
    isCH = ch;
    chIndex = head;
    readingsHere = 0;
    cancelEvent(advTimer);
    cancelEvent(sendTimer);
    cancelEvent(fuseTimer);
    if (isCH) {
        scheduleAt(simTime() + 0.2 + 0.2 * cluster, advTimer);
        scheduleAt(fuseAt, fuseTimer);
    }
    else if (head >= 0) {
        scheduleAt(sendAt, sendTimer);
    }
}

void ClusterApp::sendMsg(int type, const L3Address& dest, int readings)
{
    auto payload = makeShared<ClusterMsg>();
    payload->setType(type);
    payload->setSource(index);
    payload->setRound(round);
    payload->setReadings(readings);
    const char* names[] = {"", "ADV", "reading", "fused"};
    auto* packet = new Packet(names[type], payload);
    socket.sendTo(packet, dest, port);
    ++sent;
}

void ClusterApp::handleMessageWhenUp(cMessage* msg)
{
    if (msg == advTimer) {
        EV_INFO << "sensor[" << index << "] (CH of cluster " << cluster << ") broadcasts ADV\n";
        sendMsg(CLUSTER_ADV, Ipv4Address::ALLONES_ADDRESS, 0);
    }
    else if (msg == sendTimer) {
        coordinator->readingGenerated();
        EV_INFO << "sensor[" << index << "] -> CH sensor[" << chIndex << "] : reading (40 bytes, TDMA slot)\n";
        sendMsg(CLUSTER_READING, addressOfSensor(chIndex), 1);
    }
    else if (msg == fuseTimer) {
        coordinator->readingGenerated();   // the CH's own reading
        const int readings = readingsHere + 1;
        EV_INFO << "sensor[" << index << "] (CH) fuses " << readings << " readings -> sink\n";
        sendMsg(CLUSTER_FUSED, L3AddressResolver().resolve("sink"), readings);
    }
    else if (msg->arrivedOn("socketIn")) {
        socket.processMessage(msg);
    }
    else {
        throw cRuntimeError("Unknown message %s", msg->getName());
    }
}

void ClusterApp::socketDataArrived(UdpSocket*, Packet* packet)
{
    auto msg = packet->peekAtFront<ClusterMsg>();
    if (msg->getRound() == round || isSink) {
        if (isSink && msg->getType() == CLUSTER_FUSED) {
            ++received;
            coordinator->readingsDelivered(msg->getReadings());
            EV_INFO << "SINK <- sensor[" << msg->getSource() << "] : fused packet with " << msg->getReadings() << " readings\n";
        }
        else if (!isSink && msg->getType() == CLUSTER_READING && isCH) {
            ++received;
            ++readingsHere;
            EV_INFO << "sensor[" << index << "] (CH) <- sensor[" << msg->getSource() << "] : reading received ("
                    << readingsHere << " so far)\n";
        }
        else if (!isSink && msg->getType() == CLUSTER_ADV && coordinator->clusterOf(msg->getSource()) == cluster) {
            ++received;
            EV_INFO << "sensor[" << index << "] hears ADV: my CH is sensor[" << msg->getSource() << "]\n";
        }
    }
    delete packet;
}

void ClusterApp::socketErrorArrived(UdpSocket*, Indication* indication)
{
    EV_WARN << "socket error\n";
    delete indication;
}

void ClusterApp::refreshDisplay() const
{
    if (isSink) return;
    cDisplayString& ds = getContainingNode(this)->getDisplayString();
    ds.setTagArg("i", 1, isCH ? "red" : kColor[cluster % 3]);
    ds.setTagArg("i", 2, isCH ? "80" : "50");
    ds.setTagArg("is", 0, isCH ? "l" : "");
    char label[48];
    snprintf(label, sizeof(label), "%s%d (%.2f J)", isCH ? "CH" : "S", index, residualEnergy());
    ds.setTagArg("t", 0, label);
}

void ClusterApp::finish()
{
    recordScalar("sent", sent);
    recordScalar("received", received);
    if (!isSink) {
        recordScalar("residualEnergy", residualEnergy(), "J");
        EV_INFO << "Sensor " << index << " (cluster " << cluster << "): sent " << sent << ", received " << received
                << ", energy left " << residualEnergy() << " J\n";
    }
    ApplicationBase::finish();
}

ClusterApp::~ClusterApp()
{
    cancelAndDelete(advTimer);
    cancelAndDelete(sendTimer);
    cancelAndDelete(fuseTimer);
}

} // namespace inetdemo
