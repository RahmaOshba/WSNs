// NED function wsnPos(seed, index, axis, area): the position of node `index`
// in the thesis deployment, i.e. mt19937(seed) drawing x then y for every node
// in turn, uniform in [0, area). Used in omnetpp.ini for the mobility of the sensors.

#include <omnetpp.h>

#include <map>
#include <random>
#include <vector>

using namespace omnetpp;

namespace pktwsn {

static double deploymentCoordinate(long seed, int index, int axis, double area)
{
    static std::map<std::pair<long, double>, std::vector<double>> cache;
    auto& v = cache[{seed, area}];
    if ((int)v.size() <= 2 * index + axis) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> pos(0.0, area);
        v.clear();
        for (int k = 0; k <= 2 * index + axis + 2 * 1000; ++k) v.push_back(pos(rng));
    }
    return v[2 * index + axis];
}

static cValue wsnPos(cComponent*, cValue argv[], int)
{
    const long seed = argv[0].intValue();
    const int index = (int)argv[1].intValue();
    const int axis = (int)argv[2].intValue();
    const double area = argv[3].doubleValueInUnit("m");
    return cValue(deploymentCoordinate(seed, index, axis, area), "m");
}

Define_NED_Function2(wsnPos, "quantity wsnPos(int seed, int index, int axis, quantity area)", "misc",
                     "Position (x: axis 0, y: axis 1) of a node of the thesis deployment: mt19937(seed), x then y per node");

} // namespace pktwsn
