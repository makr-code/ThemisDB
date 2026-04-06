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
 * @brief UUID v4 generation utility
 *
 * @details Provides a self-contained, dependency-free UUID v4 generator
 *          using the C++17 <random> facilities. Each call produces a
 *          statistically random 128-bit UUID formatted as the canonical
 *          lowercase hyphenated string
 *          "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".
 *
 *          This implementation is intended for identifier/uniqueness use
 *          cases and does not use a cryptographically secure PRNG; it
 *          MUST NOT be used for secrets, authentication tokens, or other
 *          security-sensitive values.
 *
 * @copyright MIT License
 */

#pragma once

#include <iomanip>
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

} // namespace utils
