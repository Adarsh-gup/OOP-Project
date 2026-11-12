// hazard.hpp  -- UNIT I: namespaces, inline functions, overloading, reference semantics
#pragma once
#include <cmath>

namespace DisasterCore::Sensors {

struct GeoCoord { double lat = 0.0, lon = 0.0; };

// ---- inline functions for real-time risk indexing ----
inline double clamp01(double v)               { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline double rainfallFactor(double mmPerHr)  { return clamp01(mmPerHr / 100.0); }
inline double slopeFactor(double deg)         { return clamp01(deg / 60.0); }
inline double saturationFactor(double pct)    { return clamp01(pct / 100.0); }
inline double vibrationFactor(double hz)      { return clamp01(hz / 20.0); }

// ---- overloaded computeHazardIndex(): different telemetry parameter combinations ----
// (1) rainfall rate + slope angle
inline double computeHazardIndex(double rainfallMmHr, double slopeDeg) {
    return 100.0 * (0.6 * rainfallFactor(rainfallMmHr) + 0.4 * slopeFactor(slopeDeg));
}
// (2) soil saturation (integer %) + vibration frequency
inline double computeHazardIndex(int soilSaturationPct, double vibrationHz) {
    return 100.0 * (0.7 * saturationFactor(soilSaturationPct) + 0.3 * vibrationFactor(vibrationHz));
}
// (3) full four-parameter fusion
inline double computeHazardIndex(double rainfallMmHr, double slopeDeg, double saturationPct, double vibrationHz) {
    return 0.5 * computeHazardIndex(rainfallMmHr, slopeDeg) +
           0.5 * computeHazardIndex(static_cast<int>(saturationPct), vibrationHz);
}

// ---- call by reference + return by reference (chainable, zero copies) ----
inline GeoCoord& shiftCoord(GeoCoord& c, double dLat, double dLon) {
    c.lat += dLat; c.lon += dLon;
    return c;
}
inline GeoCoord& wrapCoord(GeoCoord& c) {                 // normalise to valid lat/lon range
    if (c.lat >  90.0) c.lat =  90.0;
    if (c.lat < -90.0) c.lat = -90.0;
    c.lon = std::fmod(c.lon + 180.0, 360.0);
    if (c.lon < 0) c.lon += 360.0;
    c.lon -= 180.0;
    return c;
}
inline double& accumulateInto(double& total, double v) {  // telemetry state accumulation
    total += v;
    return total;
}

} // namespace DisasterCore::Sensors
