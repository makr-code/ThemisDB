/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timestamp_utils.h                                  ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:06:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace utils {

/**
 * @brief ISO 8601 / RFC 3339 timestamp parsing and formatting utilities.
 *
 * All methods operate on UTC time points.  No external date/time library
 * dependencies — only the C++ standard library is used.
 *
 * Supported parse formats:
 *   "2026-03-09T21:28:24Z"
 *   "2026-03-09T21:28:24.912Z"
 *   "2026-03-09T21:28:24+05:30"
 *   "2026-03-09T21:28:24.123+05:30"
 */
class TimestampUtils {
public:
    TimestampUtils() = delete;

    /**
     * @brief Format a time_point as an ISO 8601 / RFC 3339 UTC string.
     *
     * Example output (include_ms=true): "2026-03-09T21:28:24.912Z"
     *
     * @param tp          The time point to format.
     * @param include_ms  Append milliseconds fraction when true.
     * @return UTF-8 formatted timestamp string.
     */
    static std::string format(std::chrono::system_clock::time_point tp,
                              bool include_ms = true);

    /**
     * @brief Parse an ISO 8601 / RFC 3339 string into a UTC time_point.
     *
     * Handles optional milliseconds and numeric timezone offsets.
     *
     * @param s  Input timestamp string.
     * @return   Parsed time point (UTC).
     * @throws   std::invalid_argument on malformed input.
     */
    static std::chrono::system_clock::time_point parse(const std::string& s);

    /**
     * @brief Return current UTC time formatted as ISO 8601 / RFC 3339.
     * @param include_ms  Append milliseconds fraction when true.
     */
    static std::string now(bool include_ms = true);

    /**
     * @brief Format a nanosecond duration as a human-readable string.
     *
     * Example: "1h 23m 45.678s"  (omits leading zero components)
     *
     * @param ns  Duration to format.
     */
    static std::string formatDuration(std::chrono::nanoseconds ns);

    /**
     * @brief Convert a time_point to Unix milliseconds since epoch.
     */
    static int64_t toUnixMs(std::chrono::system_clock::time_point tp);

    /**
     * @brief Reconstruct a time_point from Unix milliseconds since epoch.
     */
    static std::chrono::system_clock::time_point fromUnixMs(int64_t ms);
};

} // namespace utils
} // namespace themis
