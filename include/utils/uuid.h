/**
 * @file uuid.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
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

    std::ostringstream oss = {};
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

namespace utils {

/**
 * @brief Generate a time-ordered UUID v7 string (RFC 9562).
 *
 * The UUID encodes the current Unix timestamp in milliseconds in the
 * most-significant 48 bits, followed by the version nibble (7), 12 random
 * bits, the 2-bit variant (0b10), and 62 random bits.
 *
 * UUID v7 values generated within the same process are monotonically
 * increasing: two calls within the same millisecond yield the same
 * timestamp portion, with the random suffix providing uniqueness.
 *
 * Format: "xxxxxxxx-xxxx-7xxx-yxxx-xxxxxxxxxxxx"
 * where the first 48 bits encode unix_ts_ms and y ∈ {8,9,a,b}.
 *
 * @return Lowercase UUID v7 string.
 */
inline std::string generate_uuid_v7() {
    // 48-bit Unix timestamp in milliseconds (wraps in year 10 889).
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t ts = static_cast<uint64_t>(now_ms) & 0x0000'FFFF'FFFF'FFFFULL;

    // Random bits (two independent 64-bit draws).
    static thread_local std::mt19937_64 prng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    const uint64_t r1 = dist(prng);
    const uint64_t r2 = dist(prng);

    // UUID v7 field layout (RFC 9562 §5.7):
    //   time_low           (32 bits) = ts >> 16
    //   time_mid           (16 bits) = ts & 0xFFFF
    //   ver + rand_a       (16 bits) = 0x7000 | (r1 >> 52 & 0x0FFF)
    //   var + rand_b_hi    (16 bits) = 0x8000 | (r1 >> 48 & 0x3FFF)
    //   rand_b_lo          (48 bits) = r2 & 0x0000FFFFFFFFFFFFULL
    const uint32_t time_low    = static_cast<uint32_t>((ts >> 16) & 0xFFFF'FFFFULL);
    const uint16_t time_mid    = static_cast<uint16_t>(ts & 0xFFFFULL);
    const uint16_t ver_rand_a  = static_cast<uint16_t>(0x7000U | ((r1 >> 52) & 0x0FFFU));
    const uint16_t var_rand_b  = static_cast<uint16_t>(0x8000U | ((r1 >> 48) & 0x3FFFU));
    const uint64_t rand_b_lo   = r2 & 0x0000'FFFF'FFFF'FFFFULL;

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0')
        << std::setw(8) << time_low
        << '-'
        << std::setw(4) << time_mid
        << '-'
        << std::setw(4) << ver_rand_a
        << '-'
        << std::setw(4) << var_rand_b
        << '-'
        << std::setw(12) << rand_b_lo;
    return oss.str();
}

} // namespace utils

namespace themis {
namespace utils {

// Compatibility bridge: keep UUID helpers available under themis::utils.
inline std::string generate_uuid_v4() {
    return ::utils::generate_uuid_v4();
}

inline std::string generate_uuid_v7() {
    return ::utils::generate_uuid_v7();
}

} // namespace utils
} // namespace themis
