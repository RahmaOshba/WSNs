#include "DemoSensor.h"

#include <cmath>

#include "DemoSink.h"

using namespace omnetpp;

namespace wsn {

Define_Module(DemoSensor);

// First-order radio model of the thesis (Heinzelman et al.)
static const double E_ELEC = 50e-9, EPS_FS = 10e-12, EPS_MP = 0.0013e-12;
const double DEMO_BITRATE = 250e3;
const double DEMO_E_DA = 5e-9;
static const int PAYLOAD_BYTES = 40;   // as in the ns-3 demo
static const int HEADER_BYTES = 11;    // 802.15.4 MAC header (short addresses) + FCS
static const int ADV_BYTES = 25;       // CH advertisement (control frame)

double DemoTxEnergy(double bits, double d)
{
    const double d0 = std::sqrt(EPS_FS / EPS_MP);
    return d < d0 ? bits * (E_ELEC + EPS_FS * d * d) : bits * (E_ELEC + EPS_MP * d * d * d * d);
}

double DemoRxEnergy(double bits) { return bits * E_ELEC; }

static const char* const kColor[] = {"#3c8cff", "#28be6e", "#aa5ae6"};

void DemoSensor::initialize()
{
    px = par("x").doubleValueInUnit("m");
    py = par("y").doubleValueInUnit("m");
    clusterId = par("cluster");
    initial = residual = par("initialEnergy").doubleValueInUnit("J");
    sink = check_and_cast<DemoSink*>(getParentModule()->getSubmodule("sink"));
    const double s = getParentModule()->par("scale").doubleValue();
    getDisplayString().setTagArg("p", 0, px * s);
    getDisplayString().setTagArg("p", 1, py * s);
    sendTimer = new cMessage("sendReading");
    fuseTimer = new cMessage("fuseAndSend");
    advTimer = new cMessage("advertise");
    WATCH(residual);
    WATCH(sent);
    WATCH(received);
    WATCH(isCH);
}

void DemoSensor::spend(double joules)
{
    residual = std::max(0.0, residual - joules);
}

void DemoSensor::startRound(int r, bool ch, DemoSensor* head, int, simtime_t sendAt, simtime_t fuseAt,
                            const std::vector<DemoSensor*>& clusterMembers)
{
    round = r;
    isCH = ch;
    myCH = head;
    members = clusterMembers;
    readingsHere = 0;
    cancelEvent(sendTimer);
    cancelEvent(fuseTimer);
    cancelEvent(advTimer);
    if (!alive()) return;
    if (isCH) {
        scheduleAt(simTime() + 0.2 + 0.2 * clusterId, advTimer);
        scheduleAt(fuseAt, fuseTimer);
    }
    else {
        scheduleAt(sendAt, sendTimer);
    }
}

void DemoSensor::transmit(cPacket* frame, cModule* to, double distance)
{
    spend(DemoTxEnergy(frame->getBitLength(), distance));
    ++sent;
    sendDirect(frame, distance / 3e8, frame->getBitLength() / DEMO_BITRATE, to, "radioIn");
}

void DemoSensor::handleMessage(cMessage* msg)
{
    if (msg == advTimer) {
        // one broadcast: paid once at the range of the farthest member, heard by every member
        double range = 0;
        for (DemoSensor* m : members)
            range = std::max(range, std::hypot(m->x() - px, m->y() - py));
        auto* adv = new cPacket("ADV", FRAME_ADV);
        adv->setByteLength(ADV_BYTES);
        spend(DemoTxEnergy(adv->getBitLength(), range));
        ++sent;
        for (DemoSensor* m : members)
            sendDirect(adv->dup(), std::hypot(m->x() - px, m->y() - py) / 3e8,
                       adv->getBitLength() / DEMO_BITRATE, m, "radioIn");
        EV_INFO << getFullName() << " (CH of cluster " << clusterId << ") broadcasts ADV to "
                << members.size() << " members\n";
        delete adv;
        return;
    }
    if (msg == sendTimer) { sendReading(); return; }
    if (msg == fuseTimer) { sendFused(); return; }

    // a frame arrived
    auto* frame = check_and_cast<cPacket*>(msg);
    if (alive()) {
        spend(DemoRxEnergy(frame->getBitLength()));
        ++received;
        if (frame->getKind() == FRAME_READING && isCH) {
            ++readingsHere;
            EV_INFO << getFullName() << " (CH) <- " << frame->getSenderModule()->getFullName()
                    << " : reading received (" << readingsHere << " so far)\n";
        }
        else if (frame->getKind() == FRAME_ADV) {
            EV_INFO << getFullName() << " hears ADV: my CH is " << frame->getSenderModule()->getFullName() << "\n";
        }
    }
    delete frame;
}

void DemoSensor::sendReading()
{
    if (!alive() || !myCH) return;
    sink->readingGenerated();
    auto* f = new cPacket("reading", FRAME_READING);
    f->setByteLength(PAYLOAD_BYTES + HEADER_BYTES);
    EV_INFO << getFullName() << " -> " << myCH->getFullName() << " : reading (" << PAYLOAD_BYTES
            << " bytes, TDMA slot)\n";
    transmit(f, myCH, std::hypot(myCH->x() - px, myCH->y() - py));
}

void DemoSensor::sendFused()
{
    if (!alive()) return;
    sink->readingGenerated();   // the CH's own reading
    const int readings = readingsHere + 1;
    spend(DEMO_E_DA * readings * PAYLOAD_BYTES * 8);   // data fusion
    auto* f = new cPacket("fused", FRAME_FUSED);
    f->setByteLength(PAYLOAD_BYTES + HEADER_BYTES);
    f->addPar("readings") = readings;
    EV_INFO << getFullName() << " (CH) fuses " << readings << " readings -> sink\n";
    transmit(f, sink, std::hypot(sink->x() - px, sink->y() - py));
}

void DemoSensor::refreshDisplay() const
{
    cDisplayString& ds = getDisplayString();
    char label[64];
    if (!alive()) {
        ds.setTagArg("i", 0, "misc/node_vs");
        ds.setTagArg("i", 1, "black");
        ds.setTagArg("t", 0, "dead");
        return;
    }
    ds.setTagArg("i", 0, isCH ? "misc/node_s" : "misc/node_vs");
    ds.setTagArg("i", 1, isCH ? "red" : kColor[clusterId % 3]);
    ds.setTagArg("i", 2, isCH ? "80" : "60");
    snprintf(label, sizeof(label), "%s%d (%.4f J)", isCH ? "CH" : "S", getIndex(), residual);
    ds.setTagArg("t", 0, label);
}

void DemoSensor::finish()
{
    recordScalar("sent", sent);
    recordScalar("received", received);
    recordScalar("residualEnergy", residual, "J");
    recordScalar("energyUsed", initial - residual, "J");
    EV_INFO << "Sensor " << getIndex() << " (cluster " << clusterId << "): sent " << sent << ", received "
            << received << ", energy left " << residual << " J\n";
    cancelAndDelete(sendTimer);
    cancelAndDelete(fuseTimer);
    cancelAndDelete(advTimer);
    sendTimer = fuseTimer = advTimer = nullptr;
}

} // namespace wsn
