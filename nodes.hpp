// nodes.hpp -- UNIT III: abstract base, containership, diamond problem (virtual bases), polymorphism
#pragma once
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include "exceptions.hpp"

namespace DisasterCore::Sensors {

// ---- independent class used for CONTAINERSHIP (has-a) ----
class PowerSubsystem {
    double pct;
public:
    explicit PowerSubsystem(double level = 100.0) : pct(level) {}
    void draw(double d)   { pct = std::max(0.0, pct - d); }
    void charge(double c) { pct = std::min(100.0, pct + c); }
    double level() const  { return pct; }
};

struct NodeConfig { std::string id; double battery = 100.0; double calibration = 1.0; double threshold = 60.0; };

// ---- simulation helpers (return-by-reference accessor) ----
inline double& stormIntensity() { static double s = 0.0; return s; }
inline double jitter(double amp) {
    static std::mt19937 gen(2026);
    std::uniform_real_distribution<double> d(-amp, amp);
    return d(gen);
}
inline bool& traceConstruction() { static bool t = false; return t; }

// ---- abstract base: classification root (is-a) ----
class BaseSensorNode {
protected:
    std::string nodeId;
    PowerSubsystem power;                       // containership: node HAS-A power subsystem
public:
    BaseSensorNode(const std::string& id, double battery) : nodeId(id), power(battery) {
        if (traceConstruction()) std::cout << "    ctor BaseSensorNode(" << id << ")\n";
    }
    virtual ~BaseSensorNode() = default;
    virtual double readTelemetry() = 0;         // pure virtual
    virtual bool evaluateThreshold() = 0;       // pure virtual
    virtual std::string kind() const = 0;
    const std::string& id() const { return nodeId; }
    double battery() const { return power.level(); }
};

// ---- root of the diamond ----
class DisasterTelemetryDevice : public BaseSensorNode {
protected:
    double calibration, threshold, lastReading = 0.0;

    double finish(double rawValue) {            // calibrate + validate against hard limit
        double v = rawValue * calibration;
        if (v > kHardMaxReading) throw ThresholdOverflowException(nodeId, v, kHardMaxReading);
        return lastReading = v;
    }
    void requirePower() const {
        if (power.level() < 5.0) throw SensorFailureException(nodeId, "battery below 5% - brownout");
    }
public:
    explicit DisasterTelemetryDevice(const NodeConfig& c)
        : BaseSensorNode(c.id, c.battery), calibration(c.calibration), threshold(c.threshold) {
        if (traceConstruction()) std::cout << "    ctor DisasterTelemetryDevice  <-- shared virtual base, built ONCE\n";
    }
    bool evaluateThreshold() override { return lastReading >= threshold; }
};

// ---- ordinary (non-diamond) concrete nodes ----
class SeismicGeophone : public DisasterTelemetryDevice {
public:
    explicit SeismicGeophone(const NodeConfig& c) : DisasterTelemetryDevice(c) {}
    double readTelemetry() override { requirePower(); power.draw(2.0); return finish(30.0 * (1.0 + stormIntensity()) + jitter(3.0)); }
    std::string kind() const override { return "Geophone"; }
};
class RiverSonarGauge : public DisasterTelemetryDevice {
public:
    explicit RiverSonarGauge(const NodeConfig& c) : DisasterTelemetryDevice(c) {}
    double readTelemetry() override { requirePower(); power.draw(1.0); return finish(55.0 * (1.0 + stormIntensity()) + jitter(3.0)); }
    std::string kind() const override { return "SonarGauge"; }
};

// ---- intermediate derivations: VIRTUAL inheritance ----
class SolarPoweredNode : virtual public DisasterTelemetryDevice {
    double panelGain;
public:
    SolarPoweredNode(const NodeConfig& c, double gain) : DisasterTelemetryDevice(c), panelGain(gain) {
        if (traceConstruction()) std::cout << "    ctor SolarPoweredNode\n";
    }
protected:
    void harvest() { power.charge(panelGain); }
};

class WirelessLoRaTransceiver : virtual public DisasterTelemetryDevice {
    int spreadingFactor; int packets = 0;
public:
    WirelessLoRaTransceiver(const NodeConfig& c, int sf) : DisasterTelemetryDevice(c), spreadingFactor(sf) {
        if (traceConstruction()) std::cout << "    ctor WirelessLoRaTransceiver\n";
    }
    int packetsSent() const { return packets; }
    int sf() const { return spreadingFactor; }
protected:
    void transmit() { ++packets; power.draw(0.5); }
};

// ---- derived leaf ----
class AutonomousHybridProbe : public SolarPoweredNode, public WirelessLoRaTransceiver {
public:
    // most-derived class constructs the virtual base directly
    AutonomousHybridProbe(const NodeConfig& c, double gain, int sf)
        : DisasterTelemetryDevice(c), SolarPoweredNode(c, gain), WirelessLoRaTransceiver(c, sf) {
        if (traceConstruction()) std::cout << "    ctor AutonomousHybridProbe\n";
    }
    double readTelemetry() override {
        requirePower();
        power.draw(2.0);
        harvest();
        double v = finish(40.0 * (1.0 + stormIntensity()) + jitter(3.0));
        transmit();
        return v;
    }
    std::string kind() const override { return "HybridProbe"; }
};

} // namespace DisasterCore::Sensors
