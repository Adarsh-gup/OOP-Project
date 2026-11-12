// telemetry.hpp -- UNIT II: classes, static members, dynamic memory, deep copy, operator overloading
#pragma once
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "hazard.hpp"

namespace DisasterCore::Sensors {

// ===================== TelemetryBuffer: dynamic memory (Rule of Three) =====================
class TelemetryBuffer {
    double* data;
    std::size_t cap, len;
public:
    explicit TelemetryBuffer(std::size_t capacity = 8)             // parameterised dynamic constructor
        : data(new double[capacity ? capacity : 1]), cap(capacity ? capacity : 1), len(0) {}
    TelemetryBuffer(const TelemetryBuffer& o)                      // deep copy constructor
        : data(new double[o.cap]), cap(o.cap), len(o.len) { std::copy(o.data, o.data + o.len, data); }
    TelemetryBuffer& operator=(const TelemetryBuffer& o) {         // deep copy assignment (exception safe)
        if (this != &o) {
            double* fresh = new double[o.cap];
            std::copy(o.data, o.data + o.len, fresh);
            delete[] data;
            data = fresh; cap = o.cap; len = o.len;
        }
        return *this;
    }
    ~TelemetryBuffer() { delete[] data; }                          // explicit destructor: no leaks

    TelemetryBuffer& push(double v) {                              // returns *this by reference
        if (len == cap) {                                          // grow x2
            double* bigger = new double[cap * 2];
            std::copy(data, data + len, bigger);
            delete[] data; data = bigger; cap *= 2;
        }
        data[len++] = v;
        return *this;
    }
    double&       operator[](std::size_t i)       { return data[i]; }
    const double& operator[](std::size_t i) const { return data[i]; }
    std::size_t size() const     { return len; }
    std::size_t capacity() const { return cap; }
    double peak() const    { return len ? *std::max_element(data, data + len) : 0.0; }
    double average() const { double s = 0; for (std::size_t i = 0; i < len; ++i) s += data[i]; return len ? s / len : 0.0; }
};

// ===================== ZoneVector: result of operator+ aggregation =====================
class ZoneVector {
    int n; double total, peak;
public:
    explicit ZoneVector(int n = 0, double total = 0.0, double peak = 0.0) : n(n), total(total), peak(peak) {}
    ZoneVector operator+(const ZoneVector& o) const { return ZoneVector(n + o.n, total + o.total, std::max(peak, o.peak)); }
    bool operator>(const ZoneVector& o) const { return mean() > o.mean(); }   // prioritise higher hazard
    double mean() const { return n ? total / n : 0.0; }
    int stations() const { return n; }
    double peakHazard() const { return peak; }
};
inline std::ostream& operator<<(std::ostream& os, const ZoneVector& z) {
    return os << "ZoneVector{stations=" << z.stations() << ", mean=" << z.mean() << ", peak=" << z.peakHazard() << "}";
}

// ===================== stream-format switch (pretty console vs raw file) =====================
inline int prettyIndex() { static const int i = std::ios_base::xalloc(); return i; }
inline std::ostream& pretty(std::ostream& os) { os.iword(prettyIndex()) = 1; return os; }
inline std::ostream& raw(std::ostream& os)    { os.iword(prettyIndex()) = 0; return os; }

// ===================== SensorStation =====================
class SensorStation {
    std::string id;
    GeoCoord pos;
    double slope = 0, rain = 0, saturation = 0, vibration = 0;     // raw private metrics
    TelemetryBuffer history{4};

    inline static int activeCount = 0;                             // static data members
    inline static int globalAlert = 0;

    friend class TelemetryAuditor;                                 // friend class: raw access
    friend std::ostream& operator<<(std::ostream&, const SensorStation&);
    friend std::istream& operator>>(std::istream&, SensorStation&);
public:
    explicit SensorStation(std::string id = "UNSET", GeoCoord p = {}, double slopeDeg = 0.0)
        : id(std::move(id)), pos(p), slope(slopeDeg) { ++activeCount; }
    SensorStation(const SensorStation& o)                          // deep copy (history deep-copied)
        : id(o.id), pos(o.pos), slope(o.slope), rain(o.rain), saturation(o.saturation),
          vibration(o.vibration), history(o.history) { ++activeCount; }
    SensorStation& operator=(const SensorStation&) = default;      // uses TelemetryBuffer deep assignment
    ~SensorStation() { --activeCount; }

    void update(double rainfallMmHr, double saturationPct, double vibrationHz) {
        rain = rainfallMmHr; saturation = saturationPct; vibration = vibrationHz;
        history.push(hazard());
    }
    // const-correct accessors
    double hazard() const { return computeHazardIndex(rain, slope, saturation, vibration); }
    const std::string& stationId() const { return id; }
    const GeoCoord& position() const { return pos; }
    GeoCoord& position() { return pos; }                           // return by reference for transforms
    const TelemetryBuffer& log() const { return history; }

    // static member functions
    static int activeStations() { return activeCount; }
    static int globalAlertLevel() { return globalAlert; }
    static void setGlobalAlertLevel(int l) { globalAlert = l; }

    // operators
    bool operator>(const SensorStation& o) const { return hazard() > o.hazard(); }
    ZoneVector operator+(const SensorStation& o) const {
        return ZoneVector(1, hazard(), hazard()) + ZoneVector(1, o.hazard(), o.hazard());
    }
};
inline ZoneVector operator+(const ZoneVector& z, const SensorStation& s) {
    return z + ZoneVector(1, s.hazard(), s.hazard());
}

inline std::ostream& operator<<(std::ostream& os, const SensorStation& s) {
    auto f = os.flags(); auto p = os.precision();
    if (os.iword(prettyIndex())) {
        os << std::fixed << std::setprecision(2) << "[" << s.id << "] (" << s.pos.lat << "N, " << s.pos.lon
           << "E) hazard=" << s.hazard() << " samples=" << s.history.size();
    } else {                                                       // machine-readable line for files
        os << std::setprecision(10) << s.id << ' ' << s.pos.lat << ' ' << s.pos.lon << ' ' << s.slope << ' '
           << s.rain << ' ' << s.saturation << ' ' << s.vibration;
    }
    os.flags(f); os.precision(p);
    return os;
}
inline std::istream& operator>>(std::istream& is, SensorStation& s) {
    std::string id; GeoCoord p; double slope, rain, sat, vib;
    if (is >> id >> p.lat >> p.lon >> slope >> rain >> sat >> vib) {
        s.id = id; s.pos = p; s.slope = slope;
        s.update(rain, sat, vib);
    }
    return is;
}

// ===================== friend class: inspects private metrics without getters =====================
class TelemetryAuditor {
public:
    static std::string audit(const SensorStation& s) {
        std::ostringstream o;
        o << "AUDIT " << s.id << ": rain=" << s.rain << "mm/h slope=" << s.slope << "deg sat=" << s.saturation
          << "% vib=" << s.vibration << "Hz history=" << s.history.size() << " samples";
        return o.str();
    }
};

} // namespace DisasterCore::Sensors
