// alerts.hpp -- Alerts namespace: sirens, alert levels, tabular stream manipulators (UNIT I/II/IV)
#pragma once
#include <iomanip>
#include <iostream>
#include <string>

namespace DisasterCore::Alerts {

enum class Level { Normal = 0, Watch, Warning, Emergency };
inline Level classify(double h) { return h < 25 ? Level::Normal : h < 50 ? Level::Watch : h < 75 ? Level::Warning : Level::Emergency; }
inline const char* name(Level l) { static const char* n[] = {"NORMAL", "WATCH", "WARNING", "EMERGENCY"}; return n[static_cast<int>(l)]; }

class EarlyWarningSiren {
    std::string zone; bool active = false;
    inline static int activeSirens = 0;                            // static: network-wide siren count
public:
    explicit EarlyWarningSiren(std::string z) : zone(std::move(z)) {}
    EarlyWarningSiren(const EarlyWarningSiren&) = delete;          // a physical siren is not copyable
    EarlyWarningSiren& operator=(const EarlyWarningSiren&) = delete;
    ~EarlyWarningSiren() { if (active) --activeSirens; }
    void sound(Level l) {
        bool want = l >= Level::Warning;
        if (want && !active) ++activeSirens; else if (!want && active) --activeSirens;
        active = want;
    }
    bool isActive() const { return active; }
    const std::string& zoneName() const { return zone; }
    static int totalActive() { return activeSirens; }
};

// ---- custom stream manipulators for tabular alert dispatch ----
inline std::ostream& alert_rule(std::ostream& os) {
    return os << '+' << std::string(16, '-') << '+' << std::string(10, '-') << '+'
              << std::string(12, '-') << '+' << std::string(8, '-') << "+\n";
}
inline std::ostream& alert_header(std::ostream& os) {              // zero-argument manipulator
    alert_rule(os);
    auto f = os.flags();
    os << "| " << std::left << std::setw(14) << "ZONE" << " | " << std::setw(8) << "HAZARD" << " | "
       << std::setw(10) << "LEVEL" << " | " << std::setw(6) << "SIREN" << " |\n";
    os.flags(f);
    return alert_rule(os);
}
struct alert_row { std::string zone; double hazard; Level level; bool siren; };   // parameterised manipulator
inline std::ostream& operator<<(std::ostream& os, const alert_row& r) {
    auto f = os.flags(); auto p = os.precision();
    os << "| " << std::left << std::setw(14) << r.zone << " | " << std::right << std::fixed << std::setprecision(2)
       << std::setw(8) << r.hazard << " | " << std::left << std::setw(10) << name(r.level) << " | "
       << std::setw(6) << (r.siren ? "ON" : "off") << " |\n";
    os.flags(f); os.precision(p);
    return os;
}

} // namespace DisasterCore::Alerts
