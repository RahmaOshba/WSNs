#include "Brain.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "NodeAgent.h"
#include "WsnCommon.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;
using namespace inet;

namespace pktwsn {

Define_Module(Brain);

static const char* const kPalette[] = {"#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd", "#8c564b", "#e377c2",
                                       "#17becf", "#bcbd22", "#7f7f7f", "#393b79", "#637939", "#8c6d31"};

Brain::~Brain()
{
}

void Brain::setAwake(int i, bool awake)
{
    if (alive(i)) agents[i]->setAwake(awake);
}

// ---------------------------------------------------------------- set-up

void Brain::initialize(int stage)
{
    if (stage == 0) {
        isV8 = std::string(par("protocol").stringValue()) == "v8";
        N = par("numNodes");
        area = par("area").doubleValueInUnit("m");
        bsX = par("bsX").doubleValueInUnit("m");
        bsY = par("bsY").doubleValueInUnit("m");
        E0 = par("initialEnergy").doubleValueInUnit("J");
        pCH = par("pCH");
        epoch = par("epoch");
        setupInterval = par("setupInterval");
        nbRange = par("neighbourRange").doubleValueInUnit("m");
        chainMode = par("chainMode");
        I1 = par("I1"); I2 = par("I2"); I3 = par("I3"); I4 = par("I4"); I5 = par("I5");
        chargeHello = par("chargeHello");
        chargeBeacon = par("chargeBeacon");
        dataBits = par("dataBits");
        controlBits = par("controlBits");
        maxRounds = par("maxRounds");
        perClusterSlots = std::string(par("slotMode").stringValue()) == "perCluster";
        sixLowpan = std::string(par("headerModel").stringValue()) == "6lowpan";
        compressedBytes = par("compressedHeaderBytes");
        advWindow = par("advWindow").doubleValueInUnit("s");
        joinWindow = par("joinWindow").doubleValueInUnit("s");
        schedWindow = par("schedWindow").doubleValueInUnit("s");
        guard = par("guard").doubleValueInUnit("s");
        handoverWindow = par("handoverWindow").doubleValueInUnit("s");
        dataSlot = par("dataSlot").doubleValueInUnit("s");
        aggSlot = par("aggSlot").doubleValueInUnit("s");
        slotJitter = par("slotJitter").doubleValueInUnit("s");
        wakeAhead = par("wakeAhead").doubleValueInUnit("s");
        advRange = area * std::sqrt(2.0);
        if (!isV8) { I1 = I2 = I3 = I4 = I5 = false; chargeHello = chargeBeacon = false; chainMode = 0; setupInterval = 1; }

        // the same deployment as the analytical code: mt19937(seed), x then y per node
        const int seed = par("seed");
        std::mt19937 topologyRng(seed);
        electionRng.seed(seed + 1);
        jitterRng.seed(seed + 7);
        std::uniform_real_distribution<double> pos(0.0, area);
        nodes.resize(N);
        clusters.resize(N);
        agents.assign(N, nullptr);
        ipCache.resize(N);
        cModule* net = getSystemModule();
        if (net->getSubmoduleVectorSize("sensor") != N)
            throw cRuntimeError("numNodes of the brain and of the network differ");
        for (int i = 0; i < N; ++i) {
            nodes[i].x = pos(topologyRng);
            nodes[i].y = pos(topologyRng);
        }
        net->subscribe(packetDroppedSignal, this);
        WATCH(round);
        WATCH(lastAlive);
        WATCH(lastCH);
        WATCH(totalDelivered);
    }
    else if (stage == 1) {
        // energy per sensor; the first round starts after the HELLOs (v8) or at once (LEACH)
        simtime_t start = 0.01;
        if (chargeHello) {
            for (int i = 0; i < N; ++i) {
                std::uniform_real_distribution<double> j(0.0, 0.2);
                at(j(jitterRng), [this, i]() { agents[i]->sendFrame(WSN_HELLO, BROADCAST, nbRange, 0, 0, 0); });
            }
            start = 0.3;
        }
        at(start, [this]() { startRound(); });
    }
}

void Brain::registerNode(NodeAgent* agent, const MacAddress& mac)
{
    if (agent->isSink()) { sinkAgent = agent; macIndex[mac] = BS; return; }
    agents[agent->index()] = agent;
    agent->setInitialEnergy(E0);
    macIndex[mac] = agent->index();
}

int Brain::payloadBits(int type) const
{
    if (type == WSN_DATA || type == WSN_AGG) return dataBits + (isV8 && chargeBeacon ? 16 : 0);   // + 16-bit energy field (v8)
    return controlBits;
}

double Brain::distance(int a, int b) const
{
    auto px = [&](int i) { return i < 0 ? bsX : nodes[i].x; };
    auto py = [&](int i) { return i < 0 ? bsY : nodes[i].y; };
    return std::hypot(px(a) - px(b), py(a) - py(b));
}

int Brain::indexOfMac(const MacAddress& mac) const
{
    auto it = macIndex.find(mac);
    return it == macIndex.end() ? BS : it->second;
}

L3Address Brain::ipOf(int node)
{
    if (node < 0) {
        if (sinkIp.isUnspecified()) sinkIp = L3AddressResolver().resolve("sink");
        return sinkIp;
    }
    if (ipCache[node].isUnspecified())
        ipCache[node] = L3AddressResolver().resolve(getSystemModule()->getSubmodule("sensor", node)->getFullPath().c_str());
    return ipCache[node];
}

void Brain::at(simtime_t t, std::function<void()> fn)
{
    auto* a = new Action();
    a->fn = std::move(fn);
    scheduleAt(std::max(t, simTime()), a);
}

void Brain::handleMessage(cMessage* msg)
{
    auto* a = check_and_cast<Action*>(msg);
    auto fn = std::move(a->fn);
    delete a;
    fn();
}

bool Brain::alive(int i) const { return i >= 0 && agents[i] && agents[i]->alive(); }
double Brain::residual(int i) const { return alive(i) ? agents[i]->residual() : 0.0; }

// ---------------------------------------------------------------- rounds

void Brain::checkPositions()
{
    // the sensors are placed by omnetpp.ini (wsnPos); they must be where the protocol thinks they are
    cModule* net = getSystemModule();
    auto check = [&](cModule* host, double x, double y) {
        auto* mob = dynamic_cast<IMobility*>(host->getSubmodule("mobility"));
        if (!mob) return;
        const Coord p = mob->getCurrentPosition();
        if (std::fabs(p.x - x) > 1e-6 || std::fabs(p.y - y) > 1e-6)
            throw cRuntimeError("%s is at (%g, %g) but the protocol expects (%g, %g): check seed / bsX / bsY in omnetpp.ini",
                                host->getFullPath().c_str(), p.x, p.y, x, y);
    };
    for (int i = 0; i < N; ++i) check(net->getSubmodule("sensor", i), nodes[i].x, nodes[i].y);
    check(net->getSubmodule("sink"), bsX, bsY);
}

void Brain::startRound()
{
    if (round == 0) checkPositions();
    ++round;
    roundGenerated = roundDelivered = 0;
    for (auto& c : clusters) { c.recv = c.inPackets = c.inSources = 0; c.sent = false; }
    const bool setup = !isV8 || ((round - 1) % setupInterval == 0);
    const simtime_t t = simTime();
    const simtime_t dataStart = setup ? setupPhase(t) : t;
    at(dataStart, [this]() { dataPhase(); });
}

simtime_t Brain::setupPhase(simtime_t t)
{
    if (isV8) {
        // the BS knows the residual energies (piggy-backed on the data) and broadcasts their average
        double sum = 0; int n = 0;
        for (int i = 0; i < N; ++i) if (alive(i)) { sum += residual(i); ++n; }
        avgEnergy = n ? sum / n : E0;
        if (chargeBeacon) sinkAgent->sendFrame(WSN_BEACON, BROADCAST, 0, 0, avgEnergy, round);
        electV8();
    }
    else {
        electLeach();
    }
    for (int i = 0; i < N; ++i) {
        nodes[i].head = nodes[i].ch ? i : -1;
        nodes[i].heardAdv.clear();
        nodes[i].scheduled = false;
        clusters[i].joined.clear();
        clusters[i].memberEnergy.clear();
        clusters[i].backup = -1;
        clusters[i].failedOver = false;
    }
    std::uniform_real_distribution<double> U(0.0, 1.0);
    const simtime_t advAt = t + 0.01, joinAt = advAt + advWindow + guard, schedAt = joinAt + joinWindow + guard;
    for (int i = 0; i < N; ++i) setAwake(i, true);   // everybody listens to the advertisements
    // 1. CH advertisement, heard by the whole field
    for (int c = 0; c < N; ++c)
        if (alive(c) && nodes[c].ch)
            at(advAt + U(jitterRng) * advWindow, [this, c]() { agents[c]->sendFrame(WSN_ADV, BROADCAST, advRange, 0, 0, round); });
    // 2. JOIN: every other node joins the nearest CH it heard (only the CHs listen)
    at(joinAt, [this]() { for (int i = 0; i < N; ++i) if (!nodes[i].ch) setAwake(i, false); });
    for (int i = 0; i < N; ++i)
        if (alive(i) && !nodes[i].ch)
            at(joinAt + U(jitterRng) * joinWindow, [this, i]() {
                if (!alive(i) || nodes[i].ch) return;
                nodes[i].head = nearestHeardCH(i);
                if (nodes[i].head >= 0) agents[i]->sendFrame(WSN_JOIN, nodes[i].head, 0, 0, 0, round);
            });
    // 3. TDMA schedule: the CH answers the members whose JOIN arrived (everybody listens)
    at(schedAt, [this]() { for (int i = 0; i < N; ++i) setAwake(i, true); });
    for (int c = 0; c < N; ++c)
        if (alive(c) && nodes[c].ch)
            at(schedAt + U(jitterRng) * schedWindow, [this, c]() {
                if (!alive(c) || !nodes[c].ch) return;
                Cluster& cl = clusters[c];
                double farthest = 0, bestE = -1;
                for (int m : cl.joined) {
                    farthest = std::max(farthest, distance(c, m));
                    if (cl.memberEnergy[m] > bestE) { bestE = cl.memberEnergy[m]; cl.backup = m; }   // v8 backup CH
                }
                if (!cl.joined.empty()) agents[c]->sendFrame(WSN_SCHED, BROADCAST, farthest, 0, 0, round);
            });
    return schedAt + schedWindow + guard;
}

int Brain::nearestHeardCH(int i) const
{
    int best = -1;
    double bestD = std::numeric_limits<double>::infinity();
    for (int c : nodes[i].heardAdv) {
        if (!alive(c) || !nodes[c].ch) continue;
        const double d = distance(i, c);
        if (d < bestD - 1e-12) { bestD = d; best = c; }
    }
    return best;
}

void Brain::dataPhase()
{
    simtime_t t = simTime();
    if (isV8) {
        for (int i = 0; i < N; ++i) {
            if (!alive(i) || nodes[i].ch) continue;
            int& h = nodes[i].head;
            if (h >= 0 && (!alive(h) || !nodes[h].ch)) h = -1;
            if (I3 && h < 0) { h = nearestHeardCH(i); nodes[i].scheduled = h >= 0; }   // orphan re-join: the CH is known from its ADV
        }
        if (I2) {
            bool any = false;
            nextHop = computeNextHop();
            std::vector<int> members(N, 0), incoming(N, 0);
            for (int i = 0; i < N; ++i) if (alive(i) && !nodes[i].ch && nodes[i].head >= 0) ++members[nodes[i].head];
            for (int c = 0; c < N; ++c) if (nextHop[c] >= 0) ++incoming[nextHop[c]];
            for (int c = 0; c < N; ++c) {
                if (!alive(c) || !nodes[c].ch) continue;
                double cost = expectedCHCost(c, members[c] + incoming[c]);
                const double pb = payloadBits(WSN_AGG);
                if (nextHop[c] >= 0) cost += RadioModel::tx(pb, distance(c, nextHop[c])) - RadioModel::tx(pb, distance(c, BS));
                if (residual(c) < cost && proactiveHandover(c, t)) any = true;
            }
            if (any) t += handoverWindow + guard;
        }
    }

    // members sleep except in their own slot; CHs listen
    for (int i = 0; i < N; ++i) setAwake(i, alive(i) && nodes[i].ch);

    // member slots
    for (int i = 0; i < N; ++i) if (alive(i)) ++roundGenerated;
    std::map<int, int> slotOfGroup;
    int maxSlot = -1, globalSlot = 0;
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (int i = 0; i < N; ++i) {
        if (!alive(i) || nodes[i].ch) continue;
        const int slot = perClusterSlots ? slotOfGroup[nodes[i].head]++ : globalSlot++;
        maxSlot = std::max(maxSlot, slot);
        // wake a little before the slot, so that the channel check (CCA) knows the frames already on the air
        at(t + slot * dataSlot - wakeAhead, [this, i]() { setAwake(i, true); });
        at(t + slot * dataSlot + U(jitterRng) * slotJitter, [this, i]() { memberSend(i); });
        at(t + (slot + 1) * dataSlot, [this, i]() { if (!nodes[i].ch) setAwake(i, false); });
    }
    const simtime_t chStart = t + (maxSlot + 1) * dataSlot + guard;
    at(chStart, [this, chStart]() { chPhase(chStart); });
}

void Brain::memberSend(int i)
{
    if (!alive(i) || nodes[i].ch) return;
    const int c = nodes[i].head;
    // a member sends to its CH only if it is in the CH's TDMA schedule; otherwise it has no CH this round
    const bool hasCH = c >= 0 && alive(c) && nodes[c].ch && nodes[i].scheduled;
    const double pb = agents[i]->frameBits(WSN_DATA);
    bool direct;
    if (isV8) direct = I4 && (!hasCH || RadioModel::tx(pb, distance(i, BS)) <= RadioModel::tx(pb, distance(i, c)));
    else direct = !hasCH;   // unified rule: a node without a CH sends straight to the BS
    if (!direct && !hasCH) return;   // unclustered (v8 without I4)
    const int dest = direct ? BS : c;
    if (RadioModel::tx(pb, distance(i, dest)) > residual(i)) return;   // cannot afford it: the reading is lost
    if (direct) ++directToBS;
    agents[i]->sendFrame(WSN_DATA, dest, 0, 1, 0, round);
}

void Brain::chPhase(simtime_t t)
{
    nextHop = computeNextHop();
    std::vector<int> order;
    for (int c = 0; c < N; ++c) if (alive(c) && nodes[c].ch) order.push_back(c);
    if (isV8 && chainMode != 0)   // relays must send after the CHs that forward to them
        std::sort(order.begin(), order.end(), [&](int a, int b) { return distance(a, BS) > distance(b, BS); });
    int k = 0;
    for (int c : order) at(t + (k++) * aggSlot, [this, c]() { chSend(c); });
    // backups promoted during the round send last
    const simtime_t late = t + k * aggSlot;
    at(late, [this, late]() {
        int extra = 0;
        for (int c = 0; c < N; ++c)
            if (alive(c) && nodes[c].ch && !clusters[c].sent) at(late + (extra++) * aggSlot, [this, c]() { chSend(c); });
        at(late + (extra + 1) * aggSlot + guard, [this]() { endRound(); });
    });
}

void Brain::chSend(int c)
{
    if (!alive(c) || !nodes[c].ch || clusters[c].sent) return;
    Cluster& cl = clusters[c];
    cl.sent = true;
    // fuse members + own reading + relayed packets
    agents[c]->spend((cl.recv + 1 + cl.inPackets) * RadioModel::E_DA * payloadBits(WSN_AGG));
    if (!alive(c)) return;
    int dst = (c < (int)nextHop.size()) ? nextHop[c] : -1;
    if (dst >= 0 && (!alive(dst) || !nodes[dst].ch || clusters[dst].sent)) dst = BS;   // relay gone: go straight to the BS
    if (dst >= 0) ++relayed;
    agents[c]->sendFrame(WSN_AGG, dst < 0 ? BS : dst, 0, cl.recv + 1 + cl.inSources, 0, round);
}

void Brain::endRound()
{
    const int delivered = std::min(roundDelivered, roundGenerated);
    totalGenerated += roundGenerated;
    totalDelivered += delivered;
    int aliveNow = 0, chNow = 0;
    double res = 0;
    for (int i = 0; i < N; ++i) if (alive(i)) { ++aliveNow; res += residual(i); if (nodes[i].ch) ++chNow; }
    lastAlive = aliveNow;
    lastCH = chNow;
    aliveVec.record(aliveNow);
    chVec.record(chNow);
    residualVec.record(res);
    pdrVec.record(roundGenerated ? (double)delivered / roundGenerated : 0.0);
    if (!FND && aliveNow < N) FND = round;
    if (!HND && aliveNow <= N / 2) HND = round;
    colorClusters();
    if (aliveNow == 0) { LND = round; endSimulation(); }
    if (round >= maxRounds) endSimulation();
    startRound();
}

// ---------------------------------------------------------------- frames

void Brain::onFrame(int at, const WsnFrame& f)
{
    const int type = f.getType(), src = f.getSrc();
    static const char* tn[] = {"", "HELLO", "BEACON", "ADV", "JOIN", "SCHED", "HANDOVER", "DATA", "AGG"};
    frameCount[std::string("rx:") + tn[type]]++;
    const double bcastBits = agents.empty() || at < 0 ? 0 : agents[at]->frameBits(type);
    switch (type) {
        case WSN_HELLO:
            if (at >= 0 && distance(at, src) <= nbRange) agents[at]->spend(RadioModel::rx(bcastBits));
            break;
        case WSN_BEACON:
            if (at >= 0) agents[at]->spend(RadioModel::rx(bcastBits));
            break;
        case WSN_ADV:
            if (at >= 0 && f.getRound() == round) {
                agents[at]->spend(RadioModel::rx(bcastBits));
                nodes[at].heardAdv.insert(src);
            }
            break;
        case WSN_JOIN:
            if (at >= 0 && f.getRound() == round && nodes[at].ch) {
                clusters[at].joined.push_back(src);
                clusters[at].memberEnergy[src] = f.getEnergy();
            }
            break;
        case WSN_SCHED:
            if (at >= 0 && nodes[at].head == src) {   // only its members listen
                agents[at]->spend(RadioModel::rx(bcastBits));
                const auto& j = clusters[src].joined;   // the schedule lists the members whose JOIN arrived
                nodes[at].scheduled = std::find(j.begin(), j.end(), at) != j.end();
            }
            break;
        case WSN_HANDOVER:
            if (at >= 0 && at != src && nodes[at].head == (int)f.getValue()) {
                agents[at]->spend(RadioModel::rx(bcastBits));
                nodes[at].head = src;
            }
            break;
        case WSN_DATA:
            if (f.getRound() != round) break;
            if (at == BS) ++roundDelivered;
            else if (nodes[at].ch && !clusters[at].sent) ++clusters[at].recv;
            break;
        case WSN_AGG:
            if (f.getRound() != round) break;
            if (at == BS) roundDelivered += f.getSources();
            else if (nodes[at].ch && !clusters[at].sent) { ++clusters[at].inPackets; clusters[at].inSources += f.getSources(); }
            break;
    }
}

void Brain::onDeath(int node)
{
    if (node < 0) return;
    const bool wasCH = nodes[node].ch;
    nodes[node].ch = false;
    if (isV8 && wasCH) tryFailover(node);
}

void Brain::receiveSignal(cComponent* src, simsignal_t, cObject* obj, cObject* details)
{
    auto* d = dynamic_cast<PacketDropDetails*>(details);
    if (!d) return;
    if (d->getReason() == RETRY_LIMIT_REACHED || d->getReason() == CONGESTION) {
        std::string name = obj ? obj->getName() : "?";
        dropNames[std::to_string((int)d->getReason()) + ":" + name.substr(0, name.find('-'))]++;
    }
    switch (d->getReason()) {
        case RETRY_LIMIT_REACHED: ++retryDrops; break;
        case CONGESTION: ++backoffDrops; break;
        case INCORRECTLY_RECEIVED: ++corrupted; break;
        default: break;
    }
}

// ---------------------------------------------------------------- election (thesis code)

void Brain::electLeach()
{
    // code/2_EDITED/leach_EDITED.cc, RunLEACH
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (auto& n : nodes) n.ch = false;
    if ((round - 1) % epoch == 0) for (auto& n : nodes) n.selectedThisEpoch = false;
    const int r = (round - 1) % epoch;
    const double denom = 1.0 - pCH * r;
    const double t = denom > 0 ? pCH / denom : 1.0;
    std::vector<int> candidates;
    for (int i = 0; i < N; ++i)
        if (alive(i) && !nodes[i].selectedThisEpoch && U(electionRng) <= t) candidates.push_back(i);
    if (candidates.empty()) {
        std::vector<int> eligible;
        for (int i = 0; i < N; ++i) if (alive(i) && !nodes[i].selectedThisEpoch) eligible.push_back(i);
        if (!eligible.empty()) {
            std::uniform_int_distribution<int> pick(0, eligible.size() - 1);
            candidates.push_back(eligible[pick(electionRng)]);
        }
    }
    for (int id : candidates) { nodes[id].ch = true; nodes[id].selectedThisEpoch = true; }
}

void Brain::electV8()
{
    // code/4_PROPOSED/v8_chain_center.cc, RunSelection
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (auto& n : nodes) n.ch = false;
    if (setupIndex % epoch == 0) for (auto& n : nodes) n.selectedThisEpoch = false;
    if (I1) {   // epoch-exhaustion fix
        bool anyEligible = false;
        for (int i = 0; i < N; ++i) if (alive(i) && !nodes[i].selectedThisEpoch) { anyEligible = true; break; }
        if (!anyEligible) for (auto& n : nodes) n.selectedThisEpoch = false;
    }
    double avg = 0; int aliveCount = 0;
    for (int i = 0; i < N; ++i) if (alive(i)) { avg += residual(i); ++aliveCount; }
    avg = aliveCount ? avg / aliveCount : 0;
    int maxDegree = 1;
    for (int i = 0; i < N; ++i) {
        nodes[i].degree = 0;
        if (!alive(i)) continue;
        for (int j = 0; j < N; ++j) if (j != i && alive(j) && distance(i, j) <= nbRange) ++nodes[i].degree;
        maxDegree = std::max(maxDegree, nodes[i].degree);
    }
    const int r = setupIndex % epoch;
    const double denom = 1.0 - pCH * r;
    const double t = denom > 0 ? pCH / denom : 1.0;
    std::vector<int> candidates;
    for (int i = 0; i < N; ++i) {
        if (!alive(i) || nodes[i].selectedThisEpoch) continue;
        if (I5 && residual(i) < avg) continue;   // energy-gated election
        const double score = (residual(i) / E0) * (1.0 + (double)nodes[i].degree / maxDegree);
        if (U(electionRng) <= std::min(1.0, t * score)) candidates.push_back(i);
    }
    if (candidates.empty()) {
        int best = -1; double bestScore = -1;
        for (int i = 0; i < N; ++i) {
            if (!alive(i) || nodes[i].selectedThisEpoch) continue;
            const double score = (residual(i) / E0) * (1.0 + (double)nodes[i].degree / maxDegree);
            if (score > bestScore) { bestScore = score; best = i; }
        }
        if (best >= 0) candidates.push_back(best);
    }
    for (int id : candidates) { nodes[id].ch = true; nodes[id].selectedThisEpoch = true; }
    ++setupIndex;
}

std::vector<int> Brain::computeNextHop() const
{
    std::vector<int> next(N, -1);
    if (!isV8 || chainMode == 0) return next;
    std::vector<int> chs;
    for (int c = 0; c < N; ++c) if (alive(c) && nodes[c].ch) chs.push_back(c);
    const double pb = payloadBits(WSN_AGG);
    if (chainMode == 1) {
        std::sort(chs.begin(), chs.end(), [&](int a, int b) { return distance(a, BS) > distance(b, BS); });
        for (size_t k = 0; k + 1 < chs.size(); ++k) next[chs[k]] = chs[k + 1];
        return next;
    }
    const double relayCost = RadioModel::rx(pb) + RadioModel::E_DA * pb;
    for (int c : chs) {
        double best = RadioModel::tx(pb, distance(c, BS));
        for (int j : chs) {
            if (j == c || distance(j, BS) >= distance(c, BS)) continue;
            const double viaJ = RadioModel::tx(pb, distance(c, j)) + relayCost;
            if (viaJ < best) { best = viaJ; next[c] = j; }
        }
    }
    return next;
}

double Brain::expectedCHCost(int ch, int members) const
{
    const double pb = payloadBits(WSN_AGG);
    return members * RadioModel::rx(pb) + (members + 1) * RadioModel::E_DA * pb + RadioModel::tx(pb, distance(ch, BS));
}

bool Brain::proactiveHandover(int ch, simtime_t t)
{
    // code/4_PROPOSED/v8_chain_center.cc, ProactiveHandover: the strongest member takes over
    int b = -1, members = 0;
    double bestE = residual(ch);
    for (int m = 0; m < N; ++m) {
        if (!alive(m) || nodes[m].ch || nodes[m].head != ch) continue;
        ++members;
        if (residual(m) > bestE && !lowEnergy(residual(m))) { bestE = residual(m); b = m; }
    }
    if (b < 0) return false;
    double farthest = 0;
    for (int m = 0; m < N; ++m)
        if (alive(m) && nodes[m].head == ch && m != b) farthest = std::max(farthest, distance(m, b));
    const double announce = RadioModel::tx(agents[b]->frameBits(WSN_HANDOVER), farthest);
    if (announce + expectedCHCost(b, members) > residual(b)) return false;
    nodes[b].ch = true;
    nodes[b].head = b;
    nodes[ch].ch = false;
    nodes[ch].head = b;          // the old CH becomes a member; the others switch when they hear the announcement
    clusters[b] = Cluster();
    clusters[b].backup = clusters[ch].backup == b ? -1 : clusters[ch].backup;
    ++handovers;
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (int m = 0; m < N; ++m) if (alive(m) && nodes[m].head == ch) setAwake(m, true);
    at(t + U(jitterRng) * handoverWindow, [this, b, ch, farthest]() { agents[b]->sendFrame(WSN_HANDOVER, BROADCAST, farthest, 0, ch, round); });
    return true;
}

void Brain::tryFailover(int dead)
{
    // code/4_PROPOSED/v8_chain_center.cc, TryFailover: the backup chosen at set-up takes over
    Cluster& cl = clusters[dead];
    if (cl.failedOver) return;
    cl.failedOver = true;
    const int b = cl.backup;
    if (b < 0 || !alive(b) || nodes[b].ch || lowEnergy(residual(b))) return;
    nodes[b].ch = true;
    nodes[b].head = b;
    clusters[b] = Cluster();
    for (int i = 0; i < N; ++i)
        if (alive(i) && !nodes[i].ch && nodes[i].head == dead && !lowEnergy(residual(i))) nodes[i].head = b;
    ++failovers;
}

// ---------------------------------------------------------------- display + results

void Brain::colorClusters()
{
    if (!getEnvir()->isGUI()) return;
    std::map<int, int> colorOf;
    for (int c = 0; c < N; ++c) if (alive(c) && nodes[c].ch) colorOf.emplace(c, (int)colorOf.size());
    for (int i = 0; i < N; ++i) {
        auto it = colorOf.find(nodes[i].ch ? i : nodes[i].head);
        agents[i]->setRole(alive(i) && nodes[i].ch, it == colorOf.end() ? "" : kPalette[it->second % 12]);
    }
}

void Brain::refreshDisplay() const
{
    char buf[96];
    snprintf(buf, sizeof(buf), "%s | round %d | %d alive | %d CHs", isV8 ? "v8" : "LEACH", round, lastAlive, lastCH);
    getDisplayString().setTagArg("t", 0, buf);
}

void Brain::finish()
{
    getSystemModule()->unsubscribe(packetDroppedSignal, this);
    recordScalar("FND", FND);
    recordScalar("HND", HND);
    recordScalar("LND", LND);
    recordScalar("PDR", totalGenerated ? (double)totalDelivered / totalGenerated : 0.0);
    recordScalar("totalGenerated", totalGenerated);
    recordScalar("totalDelivered", totalDelivered);
    recordScalar("rounds", round);
    recordScalar("macRetryLimitDrops", retryDrops);
    recordScalar("macBackoffDrops", backoffDrops);
    recordScalar("corruptedFrames", corrupted);
    recordScalar("handovers", handovers);
    recordScalar("failovers", failovers);
    recordScalar("directToBS", directToBS);
    recordScalar("relayed", relayed);
    for (auto& kv : frameCount) recordScalar(("frames:" + kv.first).c_str(), kv.second);
    for (auto& kv : dropNames)   // MAC drops by reason (10 = retry limit, 12 = backoff limit) and frame type
        recordScalar(("macDrops:" + kv.first).c_str(), kv.second);
    EV_INFO << (isV8 ? "v8" : "LEACH") << " (packet level): FND=" << FND << " HND=" << HND << " LND=" << LND
            << " PDR=" << (totalGenerated ? (double)totalDelivered / totalGenerated : 0.0) << "\n";
}

} // namespace pktwsn
