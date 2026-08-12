/**
 * @file temporal_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Index
 *
 * An in-memory interval-tree index for fast temporal range look-ups.
 * Supports both point-in-time (AS OF) and range queries.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * A single entry stored inside the temporal index.
 */
struct TemporalIndexEntry {
    std::string key;
    TimeRange range;
    nlohmann::json payload; ///< Optional denormalized columns for covering index
};

/**
 * Statistics for a TemporalIndex instance.
 */
struct TemporalIndexStats {
    size_t total_entries{0};
    Timestamp min_timestamp{kMaxTimestamp};
    Timestamp max_timestamp{kMinTimestamp};
    double avg_duration_ms{0.0};
    size_t point_queries{0};
    size_t range_queries{0};

    nlohmann::json toJson() const {
        return {{"total_entries", total_entries},
                {"min_timestamp", min_timestamp},
                {"max_timestamp", max_timestamp},
                {"avg_duration_ms", avg_duration_ms},
                {"point_queries", point_queries},
                {"range_queries", range_queries}};
    }
};

/**
 * TemporalIndex
 *
 * A sorted, interval-based secondary index that allows efficient answering of:
 *
 *   • Point query: "which entries are valid at timestamp T?"
 *   • Range query: "which entries overlap [from, to)?"
 *
 * Implementation uses a std::map keyed by range.start for fast lower-bound
 * queries, and linear filtering within candidate sets.  For production use
 * an augmented interval tree (max-end tracking) is recommended; this
 * implementation provides the correct API and semantics.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class TemporalIndex {
public:
    explicit TemporalIndex(std::string name);

    // ── Mutation ─────────────────────────────────────────────────────────────

    /** Insert a new entry. */
    void insert(const TemporalIndexEntry& entry);

    /**
     * Remove entries matching the given key and range.
     * Returns the number of entries removed.
     */
    size_t remove(const std::string& key, const TimeRange& range);

    /** Remove all entries for the given key. */
    size_t removeKey(const std::string& key);

    // ── Queries ───────────────────────────────────────────────────────────────

    /**
     * Return all entries that are valid at timestamp t
     * (i.e., entry.range.contains(t) == true).
     */
    std::vector<TemporalIndexEntry> queryPoint(Timestamp t) const;

    /**
     * Return all entries whose range overlaps [from, to).
     */
    std::vector<TemporalIndexEntry> queryRange(Timestamp from, Timestamp to) const;

    /**
     * Return all entries for a specific key, optionally filtered by range.
     */
    std::vector<TemporalIndexEntry> queryKey(
        const std::string& key,
        std::optional<TimeRange> range = std::nullopt) const;

    // ── Metadata ─────────────────────────────────────────────────────────────

    const std::string& name() const noexcept { return name_; }
    size_t size() const;
    TemporalIndexStats stats() const;

private:
    std::string name_;

    // Primary store: (range.start → list of entries with that start time)
    // A multimap allows multiple entries at the same start timestamp.
    std::multimap<Timestamp, TemporalIndexEntry> entries_by_start_;

    mutable std::mutex mutex_;
    mutable TemporalIndexStats stats_;
};

} // namespace temporal
} // namespace themisdb
