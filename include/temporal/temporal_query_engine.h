/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_query_engine.h                            ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     151                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include "temporal/bi_temporal.h"
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
 * Temporal query semantics (SQL:2011 §4.16.5).
 *
 * SEQUENCED:     Each row is evaluated independently per temporal period.
 *                Temporal predicates are applied relative to each row's
 *                sys_time period; results preserve temporal consistency.
 *
 * NON_SEQUENCED: The time dimension is ignored; rows are treated as an
 *                atemporal relation.  All versions across all time are
 *                returned without temporal filtering.
 */
enum class TemporalSemantics {
    SEQUENCED,
    NON_SEQUENCED
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
     * Bi-temporal join between two BiTemporalTables.
     *
     * Returns pairs (left_row, right_row) where:
     *   - Both rows have sys_time.contains(sys_as_of)
     *   - Both rows have valid_time.contains(valid_at)
     *   - predicate(left_row, right_row) returns true
     *
     * This implements the combined transaction-time + valid-time join
     * semantics defined in SQL:2011.
     *
     * @param left       Left-hand bi-temporal table.
     * @param right      Right-hand bi-temporal table.
     * @param sys_as_of  System-time point applied to both tables.
     * @param valid_at   Valid-time point applied to both tables.
     * @param predicate  Join condition evaluated on every (left, right) pair.
     */
    static std::vector<std::pair<VersionedDocument, VersionedDocument>>
    joinBiTemporal(
        const BiTemporalTable& left,
        const BiTemporalTable& right,
        Timestamp sys_as_of,
        Timestamp valid_at,
        const std::function<bool(const VersionedDocument&,
                                 const VersionedDocument&)>& predicate);

    /**
     * Query a SystemVersionedTable with explicit SEQUENCED or NON-SEQUENCED
     * semantics (SQL:2011 §4.16.5).
     *
     * SEQUENCED:     Returns rows whose sys_time overlaps the given period.
     *                Temporal predicates are respected per individual period.
     * NON_SEQUENCED: Returns all row versions regardless of sys_time,
     *                treating the table as an atemporal relation.
     *
     * @param table     Source table.
     * @param semantics SEQUENCED or NON_SEQUENCED.
     * @param period    Reference period (used for SEQUENCED filtering only).
     * @param filters   Optional field-level row filters.
     */
    static std::vector<VersionedDocument> queryWithSemantics(
        const SystemVersionedTable& table,
        TemporalSemantics semantics,
        const TimeRange& period,
        const std::vector<RowFilter>& filters = {});

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
