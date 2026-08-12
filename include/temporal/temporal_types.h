/**
 * @file temporal_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Types
 *
 * Common types and utilities for all temporal module components.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <limits>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace temporal {

/// Milliseconds since Unix epoch
using Timestamp = int64_t;

/// Sentinel value representing "end of time" (open upper bound)
static constexpr Timestamp kMaxTimestamp = std::numeric_limits<int64_t>::max();

/// Sentinel value representing "beginning of time" (open lower bound).
/// Defined as -max() to avoid sign-overflow in arithmetic operations.
static constexpr Timestamp kMinTimestamp = -std::numeric_limits<int64_t>::max();

/// Return the current wall-clock time in milliseconds since epoch
inline Timestamp now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/**
 * Half-open time range [start, end).
 * end == kMaxTimestamp means "still current".
 */
struct TimeRange {
    Timestamp start{kMinTimestamp};
    Timestamp end{kMaxTimestamp};

    bool contains(Timestamp t) const noexcept {
        return t >= start && t < end;
    }

    bool overlaps(const TimeRange& other) const noexcept {
        return start < other.end && other.start < end;
    }

    bool precedes(const TimeRange& other) const noexcept {
        return end <= other.start;
    }

    bool succeeds(const TimeRange& other) const noexcept {
        return start >= other.end;
    }

    bool meets(const TimeRange& other) const noexcept {
        return end == other.start;
    }

    bool operator==(const TimeRange& other) const noexcept {
        return start == other.start && end == other.end;
    }

    bool operator!=(const TimeRange& other) const noexcept {
        return !(*this == other);
    }

    nlohmann::json toJson() const {
        return {{"start", start}, {"end", end}};
    }

    static TimeRange fromJson(const nlohmann::json& j) {
        return {j.at("start").get<Timestamp>(), j.at("end").get<Timestamp>()};
    }
};

/**
 * A document is an arbitrary JSON object.
 */
using Document = nlohmann::json;

/**
 * A document annotated with system-time versioning information.
 */
struct VersionedDocument {
    std::string key;
    Document data;
    TimeRange sys_time;          ///< System (transaction) time period
    TimeRange valid_time;        ///< Application (valid) time period
    std::string modified_by;     ///< Source node or user

    bool isCurrent() const noexcept {
        return sys_time.end == kMaxTimestamp;
    }

    nlohmann::json toJson() const {
        return {
            {"key", key},
            {"data", data},
            {"sys_time", sys_time.toJson()},
            {"valid_time", valid_time.toJson()},
            {"modified_by", modified_by}};
    }
};

} // namespace temporal
} // namespace themisdb
