/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_query_engine.h                            ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:10:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     414                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fe76ac476  2026-03-12  fix(temporal): address PR review comments on QueryCache a... ║
    • bce530ee4  2026-03-12  feat(temporal): implement Time-Travel Query Engine (v1.2.0) ║
    • 6e8942ed4  2026-03-09  feat(temporal): implement bitemporal joins and SEQUENCED/... ║
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
#include "temporal/temporal_index.h"
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
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
     * FOR SYSTEM_TIME BETWEEN start AND end (SQL:2011 §7.6)
     *
     * Returns all row versions whose sys_time overlaps the closed interval
     * [start, end].  This differs from queryFromTo() which uses the
     * half-open interval [from, to).
     *
     * @param table    Source table.
     * @param start    Inclusive range start.
     * @param end      Inclusive range end.
     * @param filters  Optional field-level row filters.
     */
    static std::vector<VersionedDocument> queryBetween(
        const SystemVersionedTable& table,
        Timestamp start,
        Timestamp end,
        const std::vector<RowFilter>& filters = {});

    /**
     * FOR APPLICATION_TIME AS OF valid_at (SQL:2011 §7.6)
     *
     * Returns all rows from a BiTemporalTable that are current in
     * system-time (latest versions, i.e., sys_time end == kMaxTimestamp)
     * and whose valid-time period contains valid_at.
     * This is the application-time counterpart of queryAsOf().
     *
     * @param table    Source bi-temporal table.
     * @param valid_at Point-in-application-time to query.
     * @param filters  Optional field-level row filters.
     */
    static std::vector<VersionedDocument> queryApplicationTime(
        const BiTemporalTable& table,
        Timestamp valid_at,
        const std::vector<RowFilter>& filters = {});

    /**
     * FOR APPLICATION_TIME FROM valid_from TO valid_to (SQL:2011 §7.6)
     *
     * Returns all rows from a BiTemporalTable whose valid-time period
     * overlaps the half-open interval [valid_from, valid_to).
     * Only rows that are currently active (sys_time end == kMaxTimestamp)
     * are returned.
     *
     * @param table       Source bi-temporal table.
     * @param valid_from  Start of the valid-time range (inclusive).
     * @param valid_to    End of the valid-time range (exclusive).
     * @param filters     Optional field-level row filters.
     */
    static std::vector<VersionedDocument> queryApplicationTimeRange(
        const BiTemporalTable& table,
        Timestamp valid_from,
        Timestamp valid_to,
        const std::vector<RowFilter>& filters = {});

    /**
     * Index-accelerated AS-OF query (query optimization).
     *
     * Uses an externally managed TemporalIndex to identify candidate keys
     * before consulting the table, reducing the scan space for large
     * history tables.  Falls back to a full scan when the index returns
     * no candidates.
     *
     * @param table   Source table.
     * @param index   Temporal index built over the same table.
     * @param as_of   Point-in-time to query.
     * @param filters Optional field-level row filters.
     */
    static std::vector<VersionedDocument> queryAsOfWithIndex(
        const SystemVersionedTable& table,
        const TemporalIndex& index,
        Timestamp as_of,
        const std::vector<RowFilter>& filters = {});

    /**
     * Compute the overlap intersection of two time ranges.
     * Returns an empty range (start==end) when there is no overlap.
     */
    static TimeRange intersect(const TimeRange& a, const TimeRange& b) noexcept;

    /**
     * Apply a list of field-level row filters to a document.
     * Returns true only when the document satisfies every filter.
     * Accessible as a public helper so that code outside the class (e.g.
     * detail::queryAsOfCached) can reuse the same filter logic.
     */
    static bool matchesFilters(const VersionedDocument& doc,
                               const std::vector<RowFilter>& filters);

};

// ============================================================================
// QueryCache — result caching for frequently accessed historical data
// ============================================================================

/**
 * QueryCache
 *
 * A simple thread-safe LRU cache for AS-OF query results.  Each entry maps a
 * (table_name, as_of) key to the list of VersionedDocument rows returned by a
 * previous queryAsOf() call.
 *
 * Intended usage pattern:
 * @code
 *   QueryCache cache(128);  // keep up to 128 distinct (table, time) entries
 *   auto rows = detail::queryAsOfCached(table, as_of, cache);
 * @endcode
 *
 * Thread-safety: all public methods are thread-safe.
 */
class QueryCache {
public:
    /**
     * Construct a cache with the given maximum number of entries.
     * When the cache is full the least-recently-used entry is evicted.
     *
     * @param max_entries  Maximum number of (table, time) entries to retain.
     *                     Must be > 0.
     */
    explicit QueryCache(size_t max_entries = 256);

    /**
     * Look up a cached result.
     * Returns a copy of the cached vector, or std::nullopt on cache miss.
     * Returning by value avoids returning a pointer into internal storage
     * that can be invalidated by a concurrent put/invalidate/clear call.
     */
    std::optional<std::vector<VersionedDocument>> get(const std::string& table_name,
                                                      Timestamp as_of) const;

    /** Store a result in the cache, evicting LRU entry if necessary. */
    void put(const std::string& table_name,
             Timestamp as_of,
             std::vector<VersionedDocument> result);

    /** Invalidate all entries for the given table (e.g. after a write). */
    void invalidate(const std::string& table_name);

    /** Discard all cached entries. */
    void clear();

    size_t size() const;

private:
    struct CacheKey {
        std::string table_name;
        Timestamp   as_of;
        bool operator==(const CacheKey& o) const noexcept {
            return as_of == o.as_of && table_name == o.table_name;
        }
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& k) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(k.table_name);
            std::size_t h2 = std::hash<int64_t>{}(k.as_of);
            // Portable hash-combine (based on boost::hash_combine).
            // Avoids shifting by a fixed 32 bits which is UB on 32-bit platforms.
            h1 ^= h2 + 0x9e3779b9u + (h1 << 6u) + (h1 >> 2u);
            return h1;
        }
    };

    struct Entry {
        CacheKey                     key;
        std::vector<VersionedDocument> value;
        // insertion-order position in lru_order_; updated on every access
        size_t lru_seq{0};
    };

    size_t max_entries_;
    mutable size_t lru_counter_{0};

    mutable std::unordered_map<CacheKey, Entry, CacheKeyHash> store_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Cached query helpers (free functions — use QueryCache)
// ============================================================================

namespace detail {

/**
 * Cached AS-OF query.
 *
 * Returns the cached result if available; otherwise executes
 * TemporalQueryEngine::queryAsOf(), caches the result, and returns it.
 * The cache is keyed on (table.tableName(), as_of); field-level filters
 * are applied *after* cache lookup so that the cache stores unfiltered
 * result sets and multiple filter combinations can reuse the same entry.
 *
 * @param table    Source table.
 * @param as_of    Point-in-time (ms since epoch).
 * @param cache    Shared QueryCache instance.
 * @param filters  Optional field-level row filters (applied post-cache).
 */
std::vector<VersionedDocument> queryAsOfCached(
    const SystemVersionedTable& table,
    Timestamp as_of,
    QueryCache& cache,
    const std::vector<RowFilter>& filters = {});

} // namespace detail

} // namespace temporal
} // namespace themisdb
