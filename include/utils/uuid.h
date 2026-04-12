/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            uuid.h                                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:13:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c4c01c2428  2026-03-12  fix(chimera): address code review feedback on ThemisDB ad... ║
    • cadbebb7b8  2026-03-12  feat(chimera): Production ThemisDB Adapter Integration - ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file uuid.h
 * @brief UUID v4 and UUID v7 generation utilities
 *
 * @details Provides self-contained, dependency-free generators for:
 *
 *  - UUID v4 (random): statistically unique 128-bit IDs.
 *  - UUID v7 (time-ordered, RFC 9562): monotonically increasing IDs that
 *    embed a 48-bit Unix millisecond timestamp in the most-significant bits.
 *    This makes them lexicographically sortable by creation time, ideal for
 *    database primary keys (e.g., RocksDB range scans, B-tree locality).
 *
 * Neither generator uses a cryptographically secure PRNG; both MUST NOT be
 * used for secrets, authentication tokens, or other security-sensitive values.
 *
 * @copyright MIT License
 */

#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

namespace utils {

/**
 * @brief Generate a random UUID v4 string.
 *
 * Uses a thread-local Mersenne-Twister seeded from std::random_device so
 * that IDs are unique across threads without lock contention.
 *
 * @return Lowercase UUID v4 string of the form
 *         "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".
 */
inline std::string generate_uuid_v4() {
    // Thread-local PRNG: seeded once per thread from a real entropy source.
    static thread_local std::mt19937_64 prng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;

    const uint64_t hi = dist(prng);
    const uint64_t lo = dist(prng);

    // Set version bits to 0100 (version 4) in octet 6 (bits 76-79 of hi).
    const uint64_t ver_hi =
        (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set variant bits to 10xx (RFC 4122 variant) in octet 8 (top 2 bits of lo).
    const uint64_t var_lo =
        (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        // time_low  (32 bits)
        << std::setw(8) << static_cast<uint32_t>((ver_hi >> 32) & 0xFFFFFFFFULL)
        << '-'
        // time_mid  (16 bits)
        << std::setw(4) << static_cast<uint16_t>((ver_hi >> 16) & 0xFFFFULL)
        << '-'
        // time_hi_and_version (16 bits, version=4)
        << std::setw(4) << static_cast<uint16_t>(ver_hi & 0xFFFFULL)
        << '-'
        // clock_seq_hi_and_reserved + clock_seq_low (16 bits, variant=10xx)
        << std::setw(4) << static_cast<uint16_t>((var_lo >> 48) & 0xFFFFULL)
        << '-'
        // node (48 bits)
        << std::setw(12) << (var_lo & 0x0000FFFFFFFFFFFFULL);

    return oss.str();
}

/**
 * @brief Generate a time-ordered UUID v7 string (RFC 9562).
 *
 * UUID v7 layout (128 bits):
 * ```
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          unix_ts_ms[47:16]                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  unix_ts_ms[15:0]             | ver(0111) | seq_hi[11:0]      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |var| seq_lo[5:0] |           rand_b[55:0]                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        rand_b[cont.]                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ```
 *
 * - **Bits 0–47**: Unix epoch in milliseconds (lexicographic time ordering).
 * - **Bits 48–51**: Version field = 0111 (7).
 * - **Bits 52–63 + 66–71**: 18-bit monotonic per-ms sequence counter.
 *   Within the same millisecond the counter is incremented atomically so
 *   that IDs generated on the same thread or across threads within one
 *   millisecond remain strictly monotonic.  When the clock advances the
 *   counter resets to a small random seed (prevents counter-guessing).
 * - **Bits 64–65**: RFC 4122 variant = 10.
 * - **Bits 72–127**: 56 bits of random data (thread-local MT19937-64).
 *
 * The returned string has the canonical lowercase hyphenated form
 * "xxxxxxxx-xxxx-7xxx-yxxx-xxxxxxxxxxxx".
 *
 * @return UUID v7 string.
 */
inline std::string generate_uuid_v7() {
    using clock = std::chrono::system_clock;
    using ms    = std::chrono::milliseconds;

    // Thread-local PRNG (no lock for random bits).
    static thread_local std::mt19937_64 prng{std::random_device{}()};
    static thread_local std::uniform_int_distribution<uint64_t> dist;

    // Monotonic sequence state: shared across all threads so that two
    // concurrent callers in the same millisecond never produce the same ID.
    struct SeqState {
        std::mutex  mu;
        uint64_t    last_ms  = 0;
        uint32_t    seq      = 0;  // 18-bit; wraps at 0x3FFFF
    };
    static SeqState state;

    const uint64_t now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<ms>(clock::now().time_since_epoch()).count());

    uint32_t seq_val;
    {
        std::lock_guard<std::mutex> lg(state.mu);
        if (now_ms > state.last_ms) {
            // Clock advanced: reset seq to a small random seed (bits 0-5) so
            // that the seq field is not predictably zero at tick boundaries.
            state.last_ms = now_ms;
            state.seq     = static_cast<uint32_t>(dist(prng) & 0x3F);
        } else {
            // Same (or backward) ms: increment and clamp to 18 bits.
            state.seq = (state.seq + 1) & 0x3FFFFU;
        }
        seq_val = state.seq;
    }

    // 56 random bits for rand_b field.
    const uint64_t rand_b = dist(prng) & 0x00FFFFFFFFFFFFFFULL;

    // ---- Assemble the two 64-bit halves ----
    //
    // High 64 bits:
    //   [63:16]  unix_ts_ms (48 bits)
    //   [15:12]  version = 0111 (4 bits)
    //   [11:0]   seq_hi = seq_val[17:6]  (12 bits)
    const uint64_t seq_hi = (seq_val >> 6) & 0x0FFFU;
    const uint64_t hi =
        ((now_ms & 0x0000FFFFFFFFFFFFULL) << 16) |
        (0x7000ULL) |                       // version 7
        seq_hi;

    // Low 64 bits:
    //   [63:62]  variant = 10
    //   [61:56]  seq_lo = seq_val[5:0]  (6 bits)
    //   [55:0]   rand_b (56 bits)
    const uint64_t seq_lo = seq_val & 0x3FULL;
    const uint64_t lo =
        (0x8000000000000000ULL) |           // variant 10xx
        (seq_lo << 56) |
        rand_b;

    // ---- Format as canonical UUID string ----
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        // time_low (32 bits of hi[63:32])
        << std::setw(8) << static_cast<uint32_t>((hi >> 32) & 0xFFFFFFFFULL)
        << '-'
        // time_mid (16 bits hi[31:16])
        << std::setw(4) << static_cast<uint16_t>((hi >> 16) & 0xFFFFULL)
        << '-'
        // time_hi_and_version (16 bits hi[15:0], already has version=7)
        << std::setw(4) << static_cast<uint16_t>(hi & 0xFFFFULL)
        << '-'
        // clock_seq_hi_and_reserved (16 bits lo[63:48])
        << std::setw(4) << static_cast<uint16_t>((lo >> 48) & 0xFFFFULL)
        << '-'
        // node (48 bits lo[47:0])
        << std::setw(12) << (lo & 0x0000FFFFFFFFFFFFULL);

    return oss.str();
}

} // namespace utils
