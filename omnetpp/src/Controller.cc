#include "Controller.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>

#include "SensorNode.h"

using namespace omnetpp;

namespace wsn {

Define_Module(Controller);

namespace {

// Protocol console output: forwarded line by line to the event log when
// "verbose" is on, otherwise discarded (badbit makes every << a no-op).
class EvLineBuf : public std::streambuf
{
    std::string line;
  protected:
    int overflow(int c) override
    {
        if (c == '\n') { EV_INFO << line << "\n"; line.clear(); }
        else if (c != EOF) line.push_back(static_cast<char>(c));
        return c;
    }
};

EvLineBuf evBuf;
std::ostream evStream(&evBuf);
std::ostream nullStream(nullptr);
std::ostream* outStream = &nullStream;
Controller* current = nullptr;

const char* const kPalette[] = {"#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd", "#8c564b", "#e377c2",
                                "#17becf", "#bcbd22", "#7f7f7f", "#393b79", "#637939", "#8c6d31"};

} // namespace

std::ostream& Out() { return *outStream; }

std::string CsvPath(const std::string& fileName)
{
    if (!current || current->csvDir().empty())
        return "/dev/null";
    return current->csvDir() + "/" + fileName;
}

void Controller::parseDefines(const char* text)
{
    std::string s(text);
    std::replace(s.begin(), s.end(), ',', ' ');
    std::istringstream in(s);
    std::string tok;
    while (in >> tok) {
        if (tok.rfind("-D", 0) == 0) tok = tok.substr(2);
        const auto eq = tok.find('=');
        if (eq == std::string::npos)
            throw cRuntimeError("defines: expected NAME=value, got '%s'", tok.c_str());
        defines[tok.substr(0, eq)] = std::stod(tok.substr(eq + 1));
    }
}

double Controller::Define(const char* name, double defaultValue)
{
    definesUsed.insert(name);
    auto it = defines.find(name);
    return it == defines.end() ? defaultValue : it->second;
}

void Controller::SetBaseStation(double x, double y)
{
    bsX = x;
    bsY = y;
}

void Controller::activity()
{
    protocolName = par("protocol").stdstringValue();
    roundDuration = par("roundDuration");
    csvDirectory = par("csvDir").stdstringValue();
    scale = par("scale");
    drawLinks = par("drawLinks").boolValue() && getEnvir()->isGUI();
    parseDefines(par("defines").stringValue());
    parseDefines(par("extraDefines").stringValue());

    auto it = Registry().find(protocolName);
    if (it == Registry().end()) {
        std::string known;
        for (const auto& p : Registry()) known += " " + p.first;
        throw cRuntimeError("Unknown protocol '%s'. Available:%s", protocolName.c_str(), known.c_str());
    }
    const ProtocolInfo& info = it->second;

    for (const auto& d : defines)
        if (std::find(info.defines.begin(), info.defines.end(), d.first) == info.defines.end())
            throw cRuntimeError("Protocol '%s' has no switch '%s' (it has:%s)", protocolName.c_str(), d.first.c_str(),
                                [&] { std::string s; for (auto& x : info.defines) s += " " + x; return s; }().c_str());

    if (!csvDirectory.empty())
        std::filesystem::create_directories(csvDirectory);

    EV_INFO << "Protocol " << info.name << " (ported from " << info.source << ")\n";
    getDisplayString().setTagArg("t", 0, info.name.c_str());

    current = this;
    outStream = par("verbose").boolValue() ? &evStream : &nullStream;
    info.run(*this);
    outStream = &nullStream;
    current = nullptr;

    if (!finished)
        throw cRuntimeError("Protocol '%s' returned without reporting its results", protocolName.c_str());
}

void Controller::createNodes(const std::vector<NodeView>& nodes)
{
    // Shift the drawing so that every node and the BS are visible (far BS: y < 0).
    double minX = bsX, minY = bsY, maxX = bsX, maxY = bsY;
    for (const auto& n : nodes) {
        minX = std::min(minX, n.x); minY = std::min(minY, n.y);
        maxX = std::max(maxX, n.x); maxY = std::max(maxY, n.y);
    }
    const double margin = 10.0;
    offsetX = margin - minX;
    offsetY = margin - minY;

    cModule* network = getParentModule();
    network->getDisplayString().setTagArg("bgb", 0, (maxX - minX + 2 * margin) * scale);
    network->getDisplayString().setTagArg("bgb", 1, (maxY - minY + 2 * margin) * scale);

    bsModule = network->getSubmodule("bs");
    if (bsModule) {
        bsModule->getDisplayString().setTagArg("p", 0, px(bsX));
        bsModule->getDisplayString().setTagArg("p", 1, py(bsY));
    }

    cModuleType* type = cModuleType::get("wsn.SensorNode");
    network->addSubmoduleVector("node", nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        cModule* m = type->create("node", network, i);
        m->par("x") = nodes[i].x;
        m->par("y") = nodes[i].y;
        m->finalizeParameters();
        m->getDisplayString().setTagArg("p", 0, px(nodes[i].x));
        m->getDisplayString().setTagArg("p", 1, py(nodes[i].y));
        m->buildInside();
        m->callInitialize();
        nodeModules.push_back(check_and_cast<SensorNode*>(m));
    }

    if (drawLinks) {
        links = new cGroupFigure("links");
        network->getCanvas()->addFigure(links);
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto* line = new cLineFigure();
            line->setVisible(false);
            line->setLineWidth(1);
            links->addFigure(line);
            linkFigures.push_back(line);
        }
    }
}

void Controller::updateCanvas(const std::vector<NodeView>& nodes, const std::vector<const char*>& colors)
{
    for (size_t i = 0; i < nodes.size(); ++i) {
        const NodeView& n = nodes[i];
        cLineFigure* line = linkFigures[i];
        const bool hasCH = n.alive && !n.isCH && n.clusterHead >= 0 && static_cast<size_t>(n.clusterHead) < nodes.size()
                           && nodes[n.clusterHead].alive;
        double tx, ty;
        if (hasCH) { tx = nodes[n.clusterHead].x; ty = nodes[n.clusterHead].y; }
        else { tx = bsX; ty = bsY; }
        line->setVisible(n.alive && (hasCH || n.isCH));
        if (!line->isVisible()) continue;
        line->setStart(cFigure::Point(px(n.x), py(n.y)));
        line->setEnd(cFigure::Point(px(tx), py(ty)));
        line->setLineColor(cFigure::Color(*colors[i] ? colors[i] : "#999999"));
        line->setLineStyle(n.isCH ? cFigure::LINE_DASHED : cFigure::LINE_SOLID);
    }
}

void Controller::EndRound(const RoundView& r, const std::vector<NodeView>& nodes)
{
    // One protocol round = one step of simulation time.
    wait(roundDuration);
    ++roundsRun;

    if (nodeModules.empty())
        createNodes(nodes);
    if (nodes.size() != nodeModules.size())
        throw cRuntimeError("node count changed during the run");

    // one colour per cluster (CH and its members), "" = no cluster
    std::map<long, int> colorOf;
    for (const auto& n : nodes)
        if (n.isCH) colorOf.emplace(n.id, static_cast<int>(colorOf.size()));
    std::vector<const char*> colors(nodes.size(), "");
    for (size_t i = 0; i < nodes.size(); ++i) {
        const long key = nodes[i].isCH ? static_cast<long>(nodes[i].id) : nodes[i].clusterHead;
        auto it = colorOf.find(key);
        if (it != colorOf.end()) colors[i] = kPalette[it->second % 12];
        nodeModules[i]->update(nodes[i], r.round, colors[i]);
    }
    if (drawLinks)
        updateCanvas(nodes, colors);

    aliveVec.record(r.alive);
    deadVec.record(r.dead);
    chVec.record(r.chCount);
    residualVec.record(r.residualEnergy);
    usedVec.record(r.energyUsed);
    generatedVec.record(static_cast<double>(r.generated));
    deliveredVec.record(static_cast<double>(r.delivered));
    pdrVec.record(r.pdr);

    char buf[96];
    snprintf(buf, sizeof(buf), "round %u: %u alive, %u CHs", r.round, r.alive, r.chCount);
    getDisplayString().setTagArg("t", 0, buf);
}

void Controller::Finish(const FinalView& f)
{
    finished = true;
    recordScalar("FND", f.FND);
    recordScalar("HND", f.HND);
    recordScalar("LND", f.LND);
    recordScalar("PDR", f.pdr);
    recordScalar("totalEnergyUsed", f.energyUsed, "J");
    recordScalar("totalGenerated", static_cast<double>(f.generated));
    recordScalar("totalDelivered", static_cast<double>(f.delivered));
    recordScalar("rounds", roundsRun);
    EV_INFO << protocolName << ": FND=" << f.FND << " HND=" << f.HND << " LND=" << f.LND << " PDR=" << f.pdr << "\n";
}

void Controller::Scalar(const char* name, double value)
{
    recordScalar(name, value);
}

void Controller::finish()
{
    for (const auto& d : defines)
        if (!definesUsed.count(d.first))
            EV_WARN << "define " << d.first << " was never read by the protocol\n";
}

} // namespace wsn
