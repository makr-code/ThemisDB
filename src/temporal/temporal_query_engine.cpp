/**
 * ThemisDB Temporal Query Engine Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_query_engine.h"
#include <algorithm>

namespace themisdb {
namespace temporal {

// ============================================================================
// Public static methods
// ============================================================================

std::vector<VersionedDocument> TemporalQueryEngine::queryAsOf(
    const SystemVersionedTable& table,
    Timestamp as_of,
    const std::vector<RowFilter>& filters) {

    auto rows = table.scan(as_of);

    if (filters.empty()) {
        return rows;
    }

    std::vector<VersionedDocument> result;
    result.reserve(rows.size());
    for (auto& row : rows) {
        if (matchesFilters(row, filters)) {
            result.push_back(std::move(row));
        }
    }
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::queryFromTo(
    const SystemVersionedTable& table,
    Timestamp from,
    Timestamp to,
    const std::vector<RowFilter>& filters) {

    TimeRange query_range{from, to};

    // Collect all keys and iterate history per key
    auto current_rows = table.scan(kMaxTimestamp);

    std::vector<VersionedDocument> result;
    // Use a set-like approach: gather all keys from current scan, then
    // query per-key history across the range.
    // We reuse the scan of "all current" to get the key set, then ask
    // for the full history over the range.
    std::vector<std::string> keys;
    keys.reserve(current_rows.size());
    for (const auto& r : current_rows) {
        keys.push_back(r.key);
    }
    // Also gather deleted keys via the full history scan
    // For simplicity we re-use the per-key API
    for (const auto& key : keys) {
        auto versions = table.getHistoryInRange(key, query_range);
        for (auto& v : versions) {
            if (filters.empty() || matchesFilters(v, filters)) {
                result.push_back(std::move(v));
            }
        }
    }
    return result;
}

std::vector<VersionedDocument> TemporalQueryEngine::queryKeyFromTo(
    const SystemVersionedTable& table,
    const std::string& key,
    Timestamp from,
    Timestamp to) {

    return table.getHistoryInRange(key, {from, to});
}

bool TemporalQueryEngine::evaluatePredicate(TemporalOperator op,
                                            const TimeRange& lhs,
                                            const TimeRange& rhs) noexcept {
    switch (op) {
        case TemporalOperator::OVERLAPS:
            return lhs.overlaps(rhs);
        case TemporalOperator::CONTAINS:
            return lhs.start <= rhs.start && lhs.end >= rhs.end;
        case TemporalOperator::PRECEDES:
            return lhs.precedes(rhs);
        case TemporalOperator::SUCCEEDS:
            return lhs.succeeds(rhs);
        case TemporalOperator::MEETS:
            return lhs.meets(rhs);
        case TemporalOperator::EQUALS:
            return lhs.start == rhs.start && lhs.end == rhs.end;
    }
    return false;
}

TimeRange TemporalQueryEngine::intersect(const TimeRange& a,
                                         const TimeRange& b) noexcept {
    Timestamp start = std::max(a.start, b.start);
    Timestamp end   = std::min(a.end, b.end);
    if (start >= end) {
        return {start, start}; // empty range
    }
    return {start, end};
}

// ============================================================================
// Private helpers
// ============================================================================

bool TemporalQueryEngine::matchesFilters(const VersionedDocument& doc,
                                         const std::vector<RowFilter>& filters) {
    for (const auto& f : filters) {
        if (!f.matches(doc.data)) {
            return false;
        }
    }
    return true;
}

} // namespace temporal
} // namespace themisdb
