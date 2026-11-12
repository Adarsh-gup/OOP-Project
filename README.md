# ALF-EWG: Autonomous Landslide & Flash-Flood Early Warning Grid (CIC-211, Option 1)

Build & run: `make run`  (C++17, `g++ -std=c++17 -Wall -Wextra -pedantic`, no warnings)

| File | Contents | Unit |
|---|---|---|
| `hazard.hpp` | namespace `DisasterCore::Sensors`, inline risk functions, overloaded `computeHazardIndex()` (3 overloads), call/return by reference (`shiftCoord`, `wrapCoord`, `accumulateInto`) | I |
| `telemetry.hpp` | `SensorStation`, `TelemetryBuffer` (dynamic ctor, deep copy, destructor), static members, const-correctness, `operator+ > << >>`, friend `TelemetryAuditor` | I, II |
| `nodes.hpp` | abstract `BaseSensorNode`, contained `PowerSubsystem`, diamond: `DisasterTelemetryDevice` -> `SolarPoweredNode` / `WirelessLoRaTransceiver` (virtual) -> `AutonomousHybridProbe` | III |
| `alerts.hpp` | namespace `DisasterCore::Alerts`, `EarlyWarningSiren` (static count), table manipulators `alert_header`, `alert_rule`, `alert_row` | I, II, IV |
| `exceptions.hpp` | `SensorFailureException`, `TelemetryChecksumMismatch`, `ThresholdOverflowException` (all from `std::exception`) | IV |
| `persistence.hpp` | binary `std::fstream` incident log, RAII transaction rollback with `seekp/seekg` | IV |
| `routing.hpp` | functors `EvacuationPriority` (for `std::sort`) and `EvacuationRouter` (for `std::for_each`) | IV |
| `main.cpp` | end-to-end demo of all four units (uses `std::vector`, `std::map`) | all |
