/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_query_engine.cpp                          ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

    std::vector<VersionedDocument> result;
    // Use getAllKeys() so that fully-deleted keys are also reachable.
    auto keys = table.getAllKeys();
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

std::vector<std::pair<VersionedDocument, VersionedDocument>>
TemporalQueryEngine::joinAsOf(
    const SystemVersionedTable& left,
    const SystemVersionedTable& right,
    Timestamp as_of,
    const std::function<bool(const VersionedDocument&,
                             const VersionedDocument&)>& predicate) {

    auto left_rows  = left.scan(as_of);
    auto right_rows = right.scan(as_of);

    std::vector<std::pair<VersionedDocument, VersionedDocument>> result;
    result.reserve(std::min(left_rows.size(), right_rows.size()));

    for (const auto& l : left_rows) {
        for (const auto& r : right_rows) {
            if (predicate(l, r)) {
                result.emplace_back(l, r);
            }
        }
    }
    return result;
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
