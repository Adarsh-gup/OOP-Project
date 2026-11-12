// ALF-EWG: Autonomous Landslide & Flash-Flood Early Warning Grid  (CIC-211 project, Option 1)
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include "alerts.hpp"
#include "nodes.hpp"
#include "persistence.hpp"
#include "routing.hpp"
#include "telemetry.hpp"

using namespace std;
using namespace DisasterCore;
using namespace DisasterCore::Sensors;
using namespace DisasterCore::Alerts;
using namespace DisasterCore::Routing;
using namespace DisasterCore::Persistence;

static void section(const string& t) { cout << "\n=== " << t << " ===\n"; }

int main() {
    cout << fixed << setprecision(2);

    // ---------------------------------------------------------------- UNIT I
    section("UNIT I: namespaces, inline, overloading, references, friend");
    cout << "hazard(rain=60, slope=40)          = " << computeHazardIndex(60.0, 40.0) << "\n";
    cout << "hazard(saturation=85%, vib=12Hz)   = " << computeHazardIndex(85, 12.0) << "\n";
    cout << "hazard(60, 40, 85, 12)             = " << computeHazardIndex(60.0, 40.0, 85.0, 12.0) << "\n";

    GeoCoord c{30.7352, 79.0669};
    shiftCoord(shiftCoord(c, 0.0100, 0.0200), -0.0050, 0.0);       // chained by-reference transforms
    cout << "shifted coord: " << c.lat << ", " << c.lon << "\n";
    GeoCoord far{95.0, 190.0}; wrapCoord(far);
    cout << "wrapped coord: " << far.lat << ", " << far.lon << "\n";
    double total = 0; accumulateInto(accumulateInto(total, 12.5), 7.5);
    cout << "accumulated telemetry total: " << total << "\n";

    // ---------------------------------------------------------------- UNIT II
    section("UNIT II: stations, static members, deep copy, operators, streams");
    struct Site { const char* zone; const char* id; double lat, lon, slope; };
    const vector<Site> sites = {
        {"Kedarnath", "ST-01", 30.7352, 79.0669, 42}, {"Kedarnath", "ST-02", 30.7410, 79.0710, 38},
        {"Chamoli",   "ST-03", 30.4100, 79.3200, 35}, {"Chamoli",   "ST-04", 30.4200, 79.3300, 28},
        {"Uttarkashi","ST-05", 30.7300, 78.4500, 22}, {"Uttarkashi","ST-06", 30.7350, 78.4600, 18}};
    const double baseRain[] = {55, 48, 40, 32, 20, 15}, baseSat[] = {80, 76, 70, 60, 50, 45}, baseVib[] = {10, 9, 7, 6, 3, 2};

    vector<SensorStation> stations; stations.reserve(sites.size());
    vector<string> zoneOf;
    for (const auto& s : sites) { stations.emplace_back(s.id, GeoCoord{s.lat, s.lon}, s.slope); zoneOf.push_back(s.zone); }
    for (double f : {1.0, 1.3, 1.6})                               // three telemetry cycles, storm intensifying
        for (size_t i = 0; i < stations.size(); ++i)
            stations[i].update(baseRain[i] * f, min(100.0, baseSat[i] * f), baseVib[i] * f);
    cout << "active stations (static): " << SensorStation::activeStations() << "\n";

    SensorStation clone = stations[0];                             // deep copy
    clone.update(0, 0, 0);
    cout << pretty << "original: " << stations[0] << "\nclone   : " << clone << "\n";
    cout << "(clone changed independently; active stations now " << SensorStation::activeStations() << ")\n";
    cout << TelemetryAuditor::audit(stations[0]) << "\n";          // friend class access

    {   // file round-trip with << and >>
        ofstream out("stations.txt");
        for (const auto& s : stations) out << raw << s << '\n';
    }
    vector<SensorStation> loaded; SensorStation tmp;
    ifstream in("stations.txt");
    while (in >> tmp) loaded.push_back(tmp);
    cout << "reloaded " << loaded.size() << " stations from file; ST-01 hazard " << stations[0].hazard()
         << " vs " << loaded[0].hazard() << "\n";

    ZoneVector chain = stations[0] + stations[1] + stations[2];    // operator+ (station+station, then +station)
    cout << "chained aggregate: " << chain << "\n";
    map<string, ZoneVector> zoneAgg;
    for (size_t i = 0; i < stations.size(); ++i) zoneAgg[zoneOf[i]] = zoneAgg[zoneOf[i]] + stations[i];
    auto worst = max_element(zoneAgg.begin(), zoneAgg.end(),
                             [](const auto& a, const auto& b) { return b.second > a.second; });  // uses operator>
    cout << "highest-hazard zone: " << worst->first << " " << worst->second << "\n";

    vector<SensorStation> ranked = stations;
    sort(ranked.begin(), ranked.end(), [](const SensorStation& a, const SensorStation& b) { return a > b; });
    cout << "stations by hazard:\n";
    for (const auto& s : ranked) cout << "  " << pretty << s << "\n";

    // ---------------------------------------------------------------- UNIT III
    section("UNIT III: diamond problem, containership, runtime polymorphism");
    cout << "constructing AutonomousHybridProbe:\n";
    traceConstruction() = true;
    auto probe = make_unique<AutonomousHybridProbe>(NodeConfig{"HYB-PROBE-1", 60, 1.0, 70}, 1.5, 9);
    traceConstruction() = false;

    vector<unique_ptr<BaseSensorNode>> owner;
    owner.push_back(move(probe));
    owner.push_back(make_unique<SeismicGeophone>(NodeConfig{"GEO-02", 12, 1.0, 45}));
    owner.push_back(make_unique<RiverSonarGauge>(NodeConfig{"SONAR-03", 80, 1.0, 60}));
    vector<BaseSensorNode*> sweepList;                             // base-class pointers
    for (auto& p : owner) sweepList.push_back(p.get());

    vector<IncidentRecord> incidents; uint64_t nextId = 1;
    for (int cycle = 0; cycle < 5; ++cycle) {
        stormIntensity() = 0.5 * cycle;
        cout << "-- sweep cycle " << cycle << " (storm x" << 1.0 + stormIntensity() << ")\n";
        for (BaseSensorNode* n : sweepList) {
            try {
                double v = n->readTelemetry();                     // dynamic dispatch via vtable/vptr
                bool breach = n->evaluateThreshold();
                cout << "   " << n->kind() << " " << n->id() << ": reading=" << v
                     << (breach ? "  ** BREACH **" : "") << "  battery=" << n->battery() << "%\n";
                if (breach) incidents.push_back(makeRecord(nextId++, n->id(), v, static_cast<int>(classify(v))));
            }
            catch (const ThresholdOverflowException& e) { cout << "   !! " << e.what() << "\n"; }
            catch (const SensorFailureException& e)     { cout << "   !! " << e.what() << "\n"; }
        }
    }
    if (auto* hp = dynamic_cast<AutonomousHybridProbe*>(sweepList[0]))   // RTTI downcast
        cout << "probe LoRa packets sent: " << hp->packetsSent() << "\n";

    // ---------------------------------------------------------------- UNIT IV
    section("UNIT IV: persistence, rollback, exceptions");
    IncidentLog log("incidents.bin");
    log.appendBatch(incidents);
    cout << "committed " << log.recordCount() << " incident records\n";

    vector<IncidentRecord> bad = {makeRecord(900, "TEST-A", 50, 2), makeRecord(901, "TEST-B", 60, 2),
                                  makeRecord(902, "TEST-C", 175, 3)};   // last one overflows
    try { log.appendBatch(bad); }
    catch (const ThresholdOverflowException& e) { cout << "   caught: " << e.what() << "\n"; }
    cout << "records after failed batch: " << log.recordCount() << " (unchanged)\n";

    auto back = log.readAll();
    cout << "read back " << back.size() << " records; first: id=" << back[0].id << " station=" << back[0].station
         << " hazard=" << back[0].hazard << "\n";
    log.corruptByte(offsetof(IncidentRecord, hazard));
    try { log.readAll(); }
    catch (const TelemetryChecksumMismatch& e) { cout << "   caught: " << e.what() << "\n"; }

    // ---- STL: map, sort, for_each, functors, manipulators ----
    section("UNIT IV: STL evacuation routing + alert table");
    const map<string, int> population = {{"Kedarnath", 3200}, {"Chamoli", 12000}, {"Uttarkashi", 8500}};
    map<string, EarlyWarningSiren> sirens;
    vector<Zone> zones;
    for (const auto& [zn, zv] : zoneAgg) {
        sirens.try_emplace(zn, zn).first->second.sound(classify(zv.mean()));
        zones.push_back({zn, zv.mean(), population.at(zn)});
    }
    SensorStation::setGlobalAlertLevel(static_cast<int>(classify(worst->second.mean())));

    sort(zones.begin(), zones.end(), EvacuationPriority());
    EvacuationRouter router = for_each(zones.begin(), zones.end(),
        EvacuationRouter({{"Dehradun Stadium", 9000}, {"Rishikesh School", 5000}, {"Haridwar Camp", 12000}}));

    cout << alert_header;
    for (const auto& z : zones) cout << alert_row{z.name, z.hazard, classify(z.hazard), sirens.at(z.name).isActive()};
    cout << alert_rule;
    cout << "sirens active: " << EarlyWarningSiren::totalActive() << "   global alert level: "
         << SensorStation::globalAlertLevel() << "\n";
    for (const auto& [zn, shelter] : router.routes()) cout << "evacuate " << zn << " -> " << shelter << "\n";
    return 0;
}
