/**
 * @file temporal_query_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// ============================================================================
// TemporalQuerySpec — structured SQL:2011 FOR SYSTEM_TIME / FOR APPLICATION_TIME
// ============================================================================

/**
 * SQL:2011 temporal clause type (§7.6).
 *
 * AS_OF          — FOR SYSTEM_TIME AS OF <timestamp>
 *                  Returns all rows current at the given instant.
 *
 * FROM_TO        — FOR SYSTEM_TIME FROM <start> TO <end>
 *                  Returns all row versions whose sys_time overlaps [start, end).
 *
 * BETWEEN_AND    — FOR SYSTEM_TIME BETWEEN <start> AND <end>
 *                  Closed-interval variant: overlaps [start, end].
 *
 * CONTAINED_IN   — FOR SYSTEM_TIME CONTAINED IN PERIOD (<start>, <end>)
 *                  Returns only rows whose entire sys_time lies within [start, end).
 *
 * ALL            — FOR SYSTEM_TIME ALL
 *                  Returns every stored version (equivalent to NON_SEQUENCED).
 */
enum class TemporalClause {
    AS_OF,
    FROM_TO,
    BETWEEN_AND,
    CONTAINED_IN,
    ALL
};

/**
 * Structured specification for a SQL:2011 temporal query.
 *
 * For AS_OF queries only start_time is used.
 * For FROM_TO / BETWEEN_AND / CONTAINED_IN both start_time and end_time are used.
 * For ALL neither start_time nor end_time is relevant.
 *
 * include_deleted: when true, rows that have been logically deleted (i.e. their
 *   most-recent version carries an explicit "deleted" flag in their data) are
 *   included in the result.  When false (default) such rows are excluded.
 */
struct TemporalQuerySpec {
    TemporalClause clause{TemporalClause::AS_OF};
    Timestamp start_time{0};
    Timestamp end_time{kMaxTimestamp};
    bool include_deleted{false};

    /// Convenience factory — FOR SYSTEM_TIME AS OF <t>
    static TemporalQuerySpec asOf(Timestamp t) noexcept {
        return {TemporalClause::AS_OF, t, kMaxTimestamp, false};
    }
    /// Convenience factory — FOR SYSTEM_TIME FROM <s> TO <e>
    static TemporalQuerySpec fromTo(Timestamp s, Timestamp e) noexcept {
        return {TemporalClause::FROM_TO, s, e, false};
    }
    /// Convenience factory — FOR SYSTEM_TIME BETWEEN <s> AND <e>
    static TemporalQuerySpec betweenAnd(Timestamp s, Timestamp e) noexcept {
        return {TemporalClause::BETWEEN_AND, s, e, false};
    }
    /// Convenience factory — FOR SYSTEM_TIME CONTAINED IN PERIOD (<s>, <e>)
    static TemporalQuerySpec containedIn(Timestamp s, Timestamp e) noexcept {
        return {TemporalClause::CONTAINED_IN, s, e, false};
    }
    /// Convenience factory — FOR SYSTEM_TIME ALL
    static TemporalQuerySpec all() noexcept {
        return {TemporalClause::ALL, kMinTimestamp, kMaxTimestamp, true};
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

    // =========================================================================
    // executeTemporalQuery — SQL:2011 FOR SYSTEM_TIME clause dispatcher
    // =========================================================================

    /**
     * Execute a SQL:2011 FOR SYSTEM_TIME query over a SystemVersionedTable.
     *
     * Dispatches to the appropriate lower-level method based on spec.clause:
     *   AS_OF         → queryAsOf(table, spec.start_time, filters)
     *   FROM_TO       → queryFromTo(table, spec.start_time, spec.end_time, filters)
     *   BETWEEN_AND   → queryBetween(table, spec.start_time, spec.end_time, filters)
     *   CONTAINED_IN  → versions whose entire sys_time ⊆ [start, end)
     *   ALL           → all stored versions (NON_SEQUENCED over full time range)
     *
     * When spec.include_deleted is false (default) rows whose data contains the
     * field "deleted" with value true are excluded from the result.
     *
     * @param table    Source system-versioned table.
     * @param spec     Temporal query specification (clause type + timestamps).
     * @param filters  Optional field-level row filters applied after the
     *                 temporal predicate.
     */
    static std::vector<VersionedDocument> executeTemporalQuery(
        const SystemVersionedTable& table,
        const TemporalQuerySpec& spec,
        const std::vector<RowFilter>& filters = {});

    /**
     * Execute a SQL:2011 FOR APPLICATION_TIME query over a BiTemporalTable.
     *
     * Dispatches based on spec.clause:
     *   AS_OF         → queryApplicationTime(table, spec.start_time, filters)
     *   FROM_TO       → queryApplicationTimeRange(table, spec.start_time,
     *                                              spec.end_time, filters)
     *   BETWEEN_AND   → queryApplicationTimeRange with closed upper bound
     *   CONTAINED_IN  → current rows whose valid_time ⊆ [start, end)
     *   ALL           → all current rows (all valid-time periods)
     *
     * @param table    Source bi-temporal table.
     * @param spec     Temporal query specification (clause type + timestamps).
     * @param filters  Optional field-level row filters.
     */
    static std::vector<VersionedDocument> executeTemporalQuery(
        const BiTemporalTable& table,
        const TemporalQuerySpec& spec,
        const std::vector<RowFilter>& filters = {});

    // ── SEQUENCED DISTINCT ────────────────────────────────────────────────────

    /**
     * @brief SQL:2011 §13.4 SEQUENCED DISTINCT — remove temporally redundant rows.
     *
     * Returns the minimal set of `VersionedDocument` rows that captures the
     * complete version history of each logical key, eliminating rows whose
     * non-temporal data is identical to an adjacent version for the same key.
     * Adjacent periods with identical data are merged into a single, longer
     * interval.
     *
     * ### Definition (SQL:2011 §13.4)
     * A row R is temporally redundant when there exists another row R' for the
     * same key such that:
     *   - R'.sys_time overlaps or is immediately adjacent to R.sys_time, AND
     *   - the compared fields of R' are identical to those of R.
     * The SEQUENCED DISTINCT result coalesces all such adjacent equal-data
     * intervals into a single row whose sys_time spans the merged range.
     *
     * ### Example
     * ```
     * key="x",  data={"v":1},  sys_time=[0,  10)
     * key="x",  data={"v":1},  sys_time=[10, 20)   ← same data, adjacent → merge
     * key="x",  data={"v":2},  sys_time=[20, 30)   ← different data → keep separate
     * ```
     * Result:
     * ```
     * key="x",  data={"v":1},  sys_time=[0,  20)
     * key="x",  data={"v":2},  sys_time=[20, 30)
     * ```
     *
     * @param table          Source table (all historical versions are scanned).
     * @param compare_fields JSON field names used for equality comparison.
     *                       Pass an empty vector to compare the entire `data`
     *                       document (all fields must match for merging).
     * @return               Coalesced rows, sorted by key then sys_start.
     */
    static std::vector<VersionedDocument> sequencedDistinct(
        const SystemVersionedTable& table,
        const std::vector<std::string>& compare_fields = {});

    /**
     * @brief SEQUENCED DISTINCT restricted to a single key.
     *
     * Same semantics as the table-wide overload, applied only to versions
     * of the given @p key.  Useful when the caller already knows the key
     * and wants to avoid scanning the full table.
     *
     * @param table          Source table.
     * @param key            Key whose versions should be coalesced.
     * @param compare_fields Fields used for equality comparison (empty = all fields).
     * @return               Coalesced rows for @p key, sorted by sys_start.
     */
    static std::vector<VersionedDocument> sequencedDistinctForKey(
        const SystemVersionedTable& table,
        const std::string& key,
        const std::vector<std::string>& compare_fields = {});
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

