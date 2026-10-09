#pragma once

#include <cmath>

namespace pktwsn {

// First-order radio model of the thesis (Heinzelman et al.), applied to every
// frame that the 802.15.4 MAC really transmits or receives.
struct RadioModel {
    static constexpr double E_ELEC = 50e-9;      // J/bit
    static constexpr double E_FS = 10e-12;       // J/bit/m^2
    static constexpr double E_MP = 0.0013e-12;   // J/bit/m^4
    static constexpr double E_DA = 5e-9;         // J/bit (data fusion)
    static double d0() { return std::sqrt(E_FS / E_MP); }
    static double tx(double bits, double d)
    {
        if (d <= 0) return bits * E_ELEC;
        return d < d0() ? bits * (E_ELEC + E_FS * d * d) : bits * (E_ELEC + E_MP * d * d * d * d);
    }
    static double rx(double bits) { return bits * E_ELEC; }
};

} // namespace pktwsn
