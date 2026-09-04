/**
 * @file temporal_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Index Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_index.h"
#include <algorithm>
#include <numeric>

namespace themisdb {
namespace temporal {

// ============================================================================
// Construction
// ============================================================================

TemporalIndex::TemporalIndex(std::string name) : name_(std::move(name)) {}

// ============================================================================
// Mutation
// ============================================================================

void TemporalIndex::insert(const TemporalIndexEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    entries_by_start_.emplace(entry.range.start, entry);

    // Update stats
    ++stats_.total_entries;
    if (entry.range.start < stats_.min_timestamp) {
        stats_.min_timestamp = entry.range.start;
    }
    Timestamp effective_end =
        (entry.range.end == kMaxTimestamp) ? entry.range.start : entry.range.end;
    if (effective_end > stats_.max_timestamp) {
        stats_.max_timestamp = effective_end;
    }
}

size_t TemporalIndex::remove(const std::string& key, const TimeRange& range) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t removed = 0;
    for (auto it = entries_by_start_.begin(); it != entries_by_start_.end();) {
        if (it->second.key == key && it->second.range.start == range.start &&
            it->second.range.end == range.end) {
            it = entries_by_start_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    stats_.total_entries -= removed;
    return removed;
}

size_t TemporalIndex::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t removed = 0;
    for (auto it = entries_by_start_.begin(); it != entries_by_start_.end();) {
        if (it->second.key == key) {
            it = entries_by_start_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    stats_.total_entries -= removed;
    return removed;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<TemporalIndexEntry> TemporalIndex::queryPoint(Timestamp t) const {
    std::lock_guard<std::mutex> lock(mutex_);

    ++stats_.point_queries;

    std::vector<TemporalIndexEntry> result = {};

    for (const auto& [start, entry] : entries_by_start_) {
        // All entries starting after t cannot contain t
        if (start > t) {
            break;
        }
        if (entry.range.contains(t)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<TemporalIndexEntry> TemporalIndex::queryRange(Timestamp from,
                                                           Timestamp to) const {
    std::lock_guard<std::mutex> lock(mutex_);

    ++stats_.range_queries;

    TimeRange query{from, to};
    std::vector<TemporalIndexEntry> result;

    for (const auto& [start, entry] : entries_by_start_) {
        // All entries that start at or after 'to' cannot overlap
        if (start >= to) {
            break;
        }
        if (entry.range.overlaps(query)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<TemporalIndexEntry> TemporalIndex::queryKey(
    const std::string& key,
    std::optional<TimeRange> range) const {

    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<TemporalIndexEntry> result = {};

    for (const auto& [start, entry] : entries_by_start_) {
        if (entry.key != key) {
            continue;
        }
        if (!range.has_value() || entry.range.overlaps(*range)) {
            result.push_back(entry);
        }
    }
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t TemporalIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_by_start_.size();
}

TemporalIndexStats TemporalIndex::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Recompute average duration
    if (!entries_by_start_.empty()) {
        double total_duration = 0.0;
        for (const auto& [start, entry] : entries_by_start_) {
            double duration =
                (entry.range.end == kMaxTimestamp)
                    ? 0.0
                    : static_cast<double>(entry.range.end - entry.range.start);
            total_duration += duration;
        }
        stats_.avg_duration_ms =
            total_duration / static_cast<double>(entries_by_start_.size());
    }

    return stats_;
}

} // namespace temporal
} // namespace themisdb
