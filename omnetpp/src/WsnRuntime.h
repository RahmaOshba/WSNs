// WsnRuntime.h -- the thin layer between the protocol engines and OMNeT++.
//
// Every protocol in src/protocols/ is the thesis' ns-3 program with its
// protocol logic unchanged. Instead of NetAnim/ns-3 it talks to this Hook,
// which the OMNeT++ Controller module implements: one call per round
// (the Controller advances simulation time by one round, updates the node
// modules and records the statistics) and one call at the end.

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace wsn {

// What the Controller needs to know about one node after a round.
struct NodeView {
    uint32_t id = 0;
    double x = 0.0, y = 0.0;
    double energy = 0.0;
    bool alive = false;
    bool isCH = false;
    long clusterHead = -1;   // -1 = none / sends to the BS
};

// What the Controller records after a round.
struct RoundView {
    uint32_t round = 0;
    uint32_t alive = 0, dead = 0, chCount = 0;
    uint64_t generated = 0, delivered = 0;
    double energyUsed = 0.0, residualEnergy = 0.0, pdr = 0.0;
};

// Final lifetime metrics, exactly as the protocol computed them.
struct FinalView {
    uint32_t FND = 0, HND = 0, LND = 0;   // 0 = not reached
    double pdr = 0.0, energyUsed = 0.0;
    uint64_t generated = 0, delivered = 0;
};

// --- member detection (the node / round structs differ a little per protocol)
template <class T, class = void> struct HasFinalCH : std::false_type {};
template <class T> struct HasFinalCH<T, std::void_t<decltype(std::declval<T>().finalCH)>> : std::true_type {};
template <class T, class = void> struct HasClusterHead : std::false_type {};
template <class T> struct HasClusterHead<T, std::void_t<decltype(std::declval<T>().clusterHead)>> : std::true_type {};
template <class T, class = void> struct HasChCount : std::false_type {};
template <class T> struct HasChCount<T, std::void_t<decltype(std::declval<T>().chCount)>> : std::true_type {};
template <class T, class = void> struct HasLeader : std::false_type {};
template <class T> struct HasLeader<T, std::void_t<decltype(std::declval<T>().leader)>> : std::true_type {};

class Hook
{
  public:
    virtual ~Hook() = default;

    // Value of a compile-time switch of the ns-3 code (-DNAME=value);
    // the ini parameter "defines" may override the default.
    virtual double Define(const char* name, double defaultValue) = 0;

    virtual void SetBaseStation(double x, double y) = 0;
    virtual void EndRound(const RoundView& round, const std::vector<NodeView>& nodes) = 0;
    virtual void Finish(const FinalView& result) = 0;
    virtual void Scalar(const char* name, double value) = 0;

    template <class R, class NV>
    void Round(uint32_t round, const R& r, const NV& nodes)
    {
        RoundView rv;
        rv.round = round;
        rv.alive = r.alive;
        rv.dead = r.dead;
        if constexpr (HasChCount<R>::value) rv.chCount = r.chCount;
        else if constexpr (HasLeader<R>::value) rv.chCount = (r.leader == std::numeric_limits<uint32_t>::max()) ? 0 : 1;
        rv.generated = r.generated;
        rv.delivered = r.delivered;
        rv.energyUsed = r.energyUsed;
        rv.residualEnergy = r.residualEnergy;
        rv.pdr = r.pdr;

        std::vector<NodeView> nv;
        nv.reserve(nodes.size());
        for (const auto& n : nodes) {
            NodeView v;
            v.id = n.id; v.x = n.x; v.y = n.y; v.energy = n.energy; v.alive = n.alive;
            if constexpr (HasFinalCH<typename NV::value_type>::value) v.isCH = n.alive && n.finalCH;
            else if constexpr (HasLeader<R>::value) v.isCH = n.alive && n.id == r.leader;
            if constexpr (HasClusterHead<typename NV::value_type>::value)
                v.clusterHead = (n.clusterHead < nodes.size() && n.clusterHead != n.id) ? static_cast<long>(n.clusterHead) : -1;
            nv.push_back(v);
        }
        EndRound(rv, nv);
    }
};

// Protocol registry: every engine registers itself under its thesis name.
using RunFn = int (*)(Hook&);
struct ProtocolInfo {
    std::string name;
    std::string source;                  // ns-3 file it was ported from
    RunFn run = nullptr;
    std::vector<std::string> defines;    // switches the engine understands
};
std::map<std::string, ProtocolInfo>& Registry();
struct Registrar {
    Registrar(const char* name, const char* source, RunFn fn, std::vector<std::string> defines);
};

std::ostream& Out();                               // protocol console output (EV or nothing)
std::string CsvPath(const std::string& fileName);  // per-run CSV file, or /dev/null
[[noreturn]] void Fatal(const std::string& message);

} // namespace wsn

// ns-3 macros used by the protocol code
#define NS_FATAL_ERROR(msg) do { std::ostringstream wsn_oss_; wsn_oss_ << msg; wsn::Fatal(wsn_oss_.str()); } while (0)
#define NS_ASSERT(cond) do { if (!(cond)) wsn::Fatal("assertion failed: " #cond); } while (0)
#define NS_ASSERT_MSG(cond, msg) do { if (!(cond)) NS_FATAL_ERROR(msg); } while (0)
