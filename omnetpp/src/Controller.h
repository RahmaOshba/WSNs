#pragma once

#include <omnetpp.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "WsnRuntime.h"

namespace wsn {

class SensorNode;

// Runs one protocol engine as an OMNeT++ process (activity()):
// every round of the protocol is one step of simulation time
// (parameter roundDuration). After each round the Controller updates the
// SensorNode modules, the canvas (CH links) and records the statistics.
class Controller : public omnetpp::cSimpleModule, public Hook
{
  public:
    Controller() : omnetpp::cSimpleModule(4 * 1024 * 1024) {}

    // Hook
    double Define(const char* name, double defaultValue) override;
    void SetBaseStation(double x, double y) override;
    void EndRound(const RoundView& round, const std::vector<NodeView>& nodes) override;
    void Finish(const FinalView& result) override;
    void Scalar(const char* name, double value) override;

    const std::string& csvDir() const { return csvDirectory; }

  protected:
    void activity() override;
    void finish() override;

  private:
    void parseDefines(const char* text);
    void createNodes(const std::vector<NodeView>& nodes);
    void updateCanvas(const std::vector<NodeView>& nodes, const std::vector<const char*>& colors);
    double px(double x) const { return (x + offsetX) * scale; }
    double py(double y) const { return (y + offsetY) * scale; }

    std::string protocolName;
    std::map<std::string, double> defines;
    std::set<std::string> definesUsed;
    omnetpp::simtime_t roundDuration;
    std::string csvDirectory;
    double scale = 4.0, offsetX = 0.0, offsetY = 0.0;
    double bsX = 0.0, bsY = 0.0;
    bool drawLinks = true;
    bool finished = false;
    uint32_t roundsRun = 0;

    std::vector<SensorNode*> nodeModules;
    omnetpp::cModule* bsModule = nullptr;
    omnetpp::cGroupFigure* links = nullptr;
    std::vector<omnetpp::cLineFigure*> linkFigures;

    omnetpp::cOutVector aliveVec{"alive"}, deadVec{"dead"}, chVec{"chCount"};
    omnetpp::cOutVector residualVec{"residualEnergy"}, usedVec{"energyUsed"};
    omnetpp::cOutVector generatedVec{"generated"}, deliveredVec{"delivered"}, pdrVec{"pdr"};
};

} // namespace wsn
