// persistence.hpp -- UNIT IV: binary fstream logging with transactional rollback via seekp/seekg
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include "exceptions.hpp"

namespace DisasterCore::Persistence {

struct IncidentRecord {
    std::uint64_t id;
    char          station[16];
    double        hazard;
    std::int32_t  level;
    std::uint32_t checksum;
};
static_assert(std::is_trivially_copyable<IncidentRecord>::value, "record must be binary-serialisable");

inline std::uint32_t checksumOf(const IncidentRecord& r) {         // FNV-1a over all payload fields
    std::uint32_t h = 2166136261u;
    auto mix = [&h](const void* p, std::size_t n) {
        auto* b = static_cast<const unsigned char*>(p);
        for (std::size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 16777619u; }
    };
    mix(&r.id, sizeof r.id); mix(r.station, sizeof r.station); mix(&r.hazard, sizeof r.hazard); mix(&r.level, sizeof r.level);
    return h;
}
inline IncidentRecord makeRecord(std::uint64_t id, const std::string& station, double hazard, int level) {
    IncidentRecord r{};
    r.id = id; r.hazard = hazard; r.level = level;
    std::strncpy(r.station, station.c_str(), sizeof r.station - 1);
    r.checksum = checksumOf(r);
    return r;
}

class IncidentLog {
    std::string path;
    std::fstream io;

    // RAII transaction: unless commit() is called, destructor rolls the file back
    class Transaction {
        IncidentLog& log; std::streampos start; bool committed = false;
    public:
        explicit Transaction(IncidentLog& l) : log(l) {
            log.io.clear();
            log.io.seekp(0, std::ios::end);
            start = log.io.tellp();                                // remember where the batch begins
        }
        void commit() { log.io.flush(); committed = true; }
        ~Transaction() {
            if (committed) return;
            log.io.clear();                                        // reset fail/bad bits
            log.io.flush();
            log.io.seekp(start);                                   // rewind write pointer
            log.io.seekg(start);                                   // rewind read pointer
            std::error_code ec;
            std::filesystem::resize_file(log.path, static_cast<std::uintmax_t>(start), ec);  // drop partial batch
            std::cout << "   [ROLLBACK] log restored to offset " << static_cast<long long>(start) << "\n";
        }
    };

    static void validate(const IncidentRecord& r) {
        if (r.hazard < 0.0) throw SensorFailureException(r.station, "negative hazard value");
        if (r.hazard > kHardMaxReading) throw ThresholdOverflowException(r.station, r.hazard, kHardMaxReading);
    }
public:
    explicit IncidentLog(const std::string& p) : path(p), io(p, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc) {
        if (!io) throw SensorFailureException("incident-log", "cannot open " + p);
    }

    void appendBatch(const std::vector<IncidentRecord>& batch) {   // all-or-nothing
        Transaction tx(*this);
        for (IncidentRecord rec : batch) {
            validate(rec);
            rec.checksum = checksumOf(rec);
            io.write(reinterpret_cast<const char*>(&rec), sizeof rec);
            if (!io) throw SensorFailureException("incident-log", "disk write failed");
        }
        tx.commit();
    }

    std::size_t recordCount() {
        io.flush();
        return static_cast<std::size_t>(std::filesystem::file_size(path) / sizeof(IncidentRecord));
    }

    std::vector<IncidentRecord> readAll() {
        io.clear();
        io.seekg(0, std::ios::beg);
        std::vector<IncidentRecord> out;
        IncidentRecord r;
        while (io.read(reinterpret_cast<char*>(&r), sizeof r)) {
            std::uint32_t actual = checksumOf(r);
            if (actual != r.checksum) throw TelemetryChecksumMismatch(r.id, r.checksum, actual);
            out.push_back(r);
        }
        io.clear();
        return out;
    }

    void corruptByte(std::streamoff offset) {                      // test helper: simulate disk bit-rot
        io.clear(); io.seekp(offset);
        char c = 0x55; io.write(&c, 1); io.flush();
    }
};

} // namespace DisasterCore::Persistence
