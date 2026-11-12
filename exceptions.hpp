// exceptions.hpp  -- UNIT IV: custom exception hierarchy (derived from std::exception)
#pragma once
#include <cstdint>
#include <exception>
#include <string>
#include <utility>

namespace DisasterCore {

inline constexpr double kHardMaxReading = 150.0;   // physical sensor limit

class DisasterException : public std::exception {
protected:
    std::string msg;
public:
    explicit DisasterException(std::string m) : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }
};

class SensorFailureException : public DisasterException {
public:
    SensorFailureException(const std::string& node, const std::string& reason)
        : DisasterException("SensorFailure[node=" + node + "]: " + reason +
                            " | action: dispatch maintenance crew, mark node OFFLINE") {}
};

class TelemetryChecksumMismatch : public DisasterException {
public:
    TelemetryChecksumMismatch(std::uint64_t recordId, std::uint32_t expected, std::uint32_t actual)
        : DisasterException("TelemetryChecksumMismatch[incident=" + std::to_string(recordId) +
                            "]: stored=" + std::to_string(expected) + " computed=" + std::to_string(actual) +
                            " | action: quarantine record, restore from replica") {}
};

class ThresholdOverflowException : public DisasterException {
public:
    ThresholdOverflowException(const std::string& src, double value, double limit)
        : DisasterException("ThresholdOverflow[src=" + src + "]: value=" + std::to_string(value) +
                            " exceeds hard limit " + std::to_string(limit) +
                            " | action: recalibrate sensor, discard reading") {}
};

} // namespace DisasterCore
