/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hlc.h                                              ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:38:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <chrono>
#include <cstdint>
#include <atomic>
#include <string>

namespace themis {

/**
 * @brief Hybrid Logical Clock (HLC) Timestamp
 *
 * A compact 64-bit representation combining physical time and a logical counter:
 *   - Upper 44 bits: wall-clock milliseconds since Unix epoch (~557 years of range)
 *   - Lower 20 bits: logical counter (up to 1,048,575 events per millisecond)
 *
 * This layout guarantees:
 *   - Monotonically increasing values across all nodes that exchange messages
 *   - Total ordering of events on a single node
 *   - Efficient big-endian byte encoding for key-sorted MVCC version lookups
 *
 * Based on: Kulkarni et al. (2014) "Logical Physical Clocks and Consistent Snapshots
 * in Globally Distributed Databases" - HotDep '14.
 */
struct HLCTimestamp {
    uint64_t value{0};  // Encoded (physical_ms << 20) | logical

    static constexpr uint32_t LOGICAL_BITS = 20;
    static constexpr uint64_t LOGICAL_MASK = (1ULL << LOGICAL_BITS) - 1;
    static constexpr uint64_t MAX_LOGICAL  = LOGICAL_MASK;

    HLCTimestamp() = default;
    explicit HLCTimestamp(uint64_t v) : value(v) {}

    /** Physical component (wall-clock milliseconds since Unix epoch) */
    uint64_t physical() const { return value >> LOGICAL_BITS; }

    /** Logical component (tie-breaker within the same millisecond) */
    uint32_t logical() const { return static_cast<uint32_t>(value & LOGICAL_MASK); }

    /** Build an HLCTimestamp from physical and logical components */
    static HLCTimestamp from(uint64_t physical_ms, uint32_t logical_counter) {
        return HLCTimestamp{(physical_ms << LOGICAL_BITS) | (logical_counter & LOGICAL_MASK)};
    }

    bool operator<(const HLCTimestamp& o)  const { return value <  o.value; }
    bool operator<=(const HLCTimestamp& o) const { return value <= o.value; }
    bool operator>(const HLCTimestamp& o)  const { return value >  o.value; }
    bool operator>=(const HLCTimestamp& o) const { return value >= o.value; }
    bool operator==(const HLCTimestamp& o) const { return value == o.value; }
    bool operator!=(const HLCTimestamp& o) const { return value != o.value; }

    /**
     * @brief Encode timestamp as 8 big-endian bytes for use in RocksDB keys.
     *
     * Big-endian encoding ensures lexicographic key order matches chronological
     * order, which is required for correct MVCC range scans.
     */
    void encodeToBytes(uint8_t out[8]) const {
        uint64_t v = value;
        for (int i = 7; i >= 0; --i) {
            out[i] = static_cast<uint8_t>(v & 0xFF);
            v >>= 8;
        }
    }

    /** Decode 8 big-endian bytes back into an HLCTimestamp. */
    static HLCTimestamp decodeFromBytes(const uint8_t in[8]) {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | in[i];
        }
        return HLCTimestamp{v};
    }

    /**
     * @brief Encode to an 8-byte string (for key construction).
     *
     * Produces a fixed-width, big-endian byte string that compares
     * correctly with memcmp / RocksDB's default bytewise comparator.
     */
    std::string encodeToString() const {
        std::string s(8, '\0');
        uint64_t v = value;
        for (int i = 7; i >= 0; --i) {
            s[i] = static_cast<char>(v & 0xFF);
            v >>= 8;
        }
        return s;
    }

    /** Decode from an 8-byte string produced by encodeToString(). */
    static HLCTimestamp decodeFromString(const std::string& s) {
        if (s.size() < 8) return HLCTimestamp{};
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | static_cast<uint8_t>(s[i]);
        }
        return HLCTimestamp{v};
    }

    /** Human-readable representation: "<physical_ms>.<logical>" */
    std::string toString() const;
};

/**
 * @brief Hybrid Logical Clock
 *
 * Thread-safe monotonic clock that combines wall-clock time with a logical
 * counter to provide a total order of events, even across nodes that exchange
 * HLC-stamped messages.
 *
 * API:
 *   now()             - generate a new timestamp for a local event
 *   update(received)  - advance the clock on receiving a remote timestamp
 *   peek()            - read the current timestamp without advancing it
 */
class HybridLogicalClock {
public:
    HybridLogicalClock();

    /**
     * @brief Generate a new timestamp for a local event.
     *
     * Guarantees: returned value > every previous value returned by this instance.
     */
    HLCTimestamp now();

    /**
     * @brief Advance the clock after receiving a message with timestamp @p received.
     *
     * Updates the clock to be strictly greater than both the local clock and
     * the received timestamp, then returns the new local timestamp.
     */
    HLCTimestamp update(HLCTimestamp received);

    /**
     * @brief Read the current timestamp without advancing it.
     *
     * The returned value reflects the last call to now() or update().
     */
    HLCTimestamp peek() const;

private:
    // Single atomic state encodes the full HLCTimestamp value:
    //   bits 63..20 = physical milliseconds since Unix epoch
    //   bits 19.. 0 = logical counter
    // All three public methods (now/update/peek) operate with CAS loops,
    // eliminating the mutex and allowing lock-free reads via peek().
    std::atomic<uint64_t> state_{0};

    static uint64_t wallClockMs();
    HLCTimestamp advanceTo(uint64_t phys_ms);
};

} // namespace themis
