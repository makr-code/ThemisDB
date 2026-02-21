/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_query_engine.h                            ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     158                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Query Engine
 *
 * SQL:2011-style time-travel queries over SystemVersionedTable collections.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * Temporal predicate operators (SQL:2011 §4.16)
 */
enum class TemporalOperator {
    CONTAINS,   ///< period contains a point or another period
    OVERLAPS,   ///< periods overlap
    PRECEDES,   ///< left period ends before right period starts
    SUCCEEDS,   ///< left period starts after right period ends
    MEETS,      ///< left period ends exactly where right period starts
    EQUALS      ///< periods are identical
};

/**
 * A simple field-level filter applied to Document rows.
 */
struct RowFilter {
    std::string field;
    nlohmann::json value;

    bool matches(const Document& doc) const {
        auto it = doc.find(field);
        return it != doc.end() && *it == value;
    }
};

/**
 * TemporalQueryEngine
 *
 * Provides time-travel and range queries over in-memory
 * SystemVersionedTable instances.
 *
 * Thread-safety: const methods are thread-safe; the table data they read
 *               must remain stable for the duration of the call.
 */
class TemporalQueryEngine {
public:
    /**
     * Query all rows that were current at a specific system time.
     *
     * @param table  Source table
     * @param as_of  Point-in-time (ms since epoch)
     * @param filters  Optional field-level row filters
     */
    static std::vector<VersionedDocument> queryAsOf(
        const SystemVersionedTable& table,
        Timestamp as_of,
        const std::vector<RowFilter>& filters = {});

    /**
     * Query all row versions whose sys_time overlaps [from, to).
     */
    static std::vector<VersionedDocument> queryFromTo(
        const SystemVersionedTable& table,
        Timestamp from,
        Timestamp to,
        const std::vector<RowFilter>& filters = {});

    /**
     * Query all row versions for a specific key whose sys_time overlaps
     * [from, to).
     */
    static std::vector<VersionedDocument> queryKeyFromTo(
        const SystemVersionedTable& table,
        const std::string& key,
        Timestamp from,
        Timestamp to);

    /**
     * Apply a temporal predicate between two time ranges.
     *
     * @param op  The temporal operator to evaluate
     * @param lhs Left-hand period
     * @param rhs Right-hand period
     * @return    true if the predicate holds
     */
    static bool evaluatePredicate(TemporalOperator op,
                                  const TimeRange& lhs,
                                  const TimeRange& rhs) noexcept;

    /**
     * Temporal AS-OF join between two tables.
     *
     * Returns pairs (left_row, right_row) where both rows were current at
     * the given system time and the join predicate returns true.
     *
     * @param left       Left-hand table.
     * @param right      Right-hand table.
     * @param as_of      Point-in-time for both tables.
     * @param predicate  Join condition evaluated on every (left, right) pair.
     *                   Return true to include the pair in the result.
     */
    static std::vector<std::pair<VersionedDocument, VersionedDocument>> joinAsOf(
        const SystemVersionedTable& left,
        const SystemVersionedTable& right,
        Timestamp as_of,
        const std::function<bool(const VersionedDocument&,
                                 const VersionedDocument&)>& predicate);

    /**
     * Compute the overlap intersection of two time ranges.
     * Returns an empty range (start==end) when there is no overlap.
     */
    static TimeRange intersect(const TimeRange& a, const TimeRange& b) noexcept;

private:
    static bool matchesFilters(const VersionedDocument& doc,
                               const std::vector<RowFilter>& filters);
};

} // namespace temporal
} // namespace themisdb
