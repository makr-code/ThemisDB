/**
 * @file bi_temporal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Bi-Temporal Table
 *
 * SQL:2011 bi-temporal support: each row carries both a system-time period
 * (when it was stored) and a valid-time period (when it is true in the
 * modelled reality).
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

// Forward declaration for TemporalForeignKey
class BiTemporalTable;

/**
 * TemporalForeignKey
 *
 * Describes a period-aware referential integrity constraint between two
 * bi-temporal tables.  A child row is valid only when the referenced parent
 * table contains a current row for the same key whose valid-time period
 * *contains* the child row's valid-time period.
 *
 * `parent_table_name` is compared against `BiTemporalTable::tableName()` in
 * `validate()` to prevent accidentally passing the wrong table instance.
 *
 * Usage:
 * @code
 *   TemporalForeignKey fk{"employees"};
 *   bool ok = fk.validate(emp_table, "emp_42", {1000, 2000});
 * @endcode
 */
struct TemporalForeignKey {
    /// Name of the referenced (parent) table.  Must match
    /// `parent_table.tableName()` when `validate()` is called.
    std::string parent_table_name;

    /**
     * Validate that @p parent_table is the expected table and that it has at
     * least one *current* row for @p parent_key whose valid-time period
     * **contains** @p child_period.
     *
     * Returns true  → referential integrity satisfied.
     * Returns false → constraint violation: either @p parent_table is the
     *                 wrong table (name mismatch), or no parent row covers the
     *                 period.
     */
    bool validate(const BiTemporalTable& parent_table,
                  const std::string& parent_key,
                  const TimeRange& child_period) const;
};

/**
 * BiTemporalTable
 *
 * Rows are versioned along two independent time axes:
 *
 *   sys_time   – The system (transaction) time period tracked automatically.
 *   valid_time – The application-defined valid-time period supplied by the
 *                caller.
 *
 * Each INSERT creates a row with valid_time = [valid_from, valid_to) and
 * sys_time = [now, ∞).  Subsequent UPDATEs close the old row's sys_time and
 * open a new row, preserving the complete bi-temporal history.
 *
 * Overlap detection ensures that no two *current* rows for the same key have
 * overlapping valid-time periods.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class BiTemporalTable {
public:
    /**
     * @brief Construct a bi-temporal table.
     * @param table_name Logical table name.
     * @param source_node Source label recorded on newly written versions.
     */
    explicit BiTemporalTable(std::string table_name,
                             std::string source_node = "local");

    // Non-copyable; move explicitly deleted because std::mutex is not movable.
    // Use std::shared_ptr<BiTemporalTable> when shared ownership is needed,
    // or pass by reference for in-process transfers.
    //
    /// @note Move semantics: BiTemporalTable is intentionally non-movable. The
    ///   internal std::mutex cannot be moved, and moving a live table under
    ///   concurrent access would create data-race hazards (CWE-362). Wrap in
    ///   std::shared_ptr<BiTemporalTable> when shared/transferred ownership is required.
    BiTemporalTable(const BiTemporalTable&)            = delete;
    BiTemporalTable& operator=(const BiTemporalTable&) = delete;
    BiTemporalTable(BiTemporalTable&&)                 = delete;
    BiTemporalTable& operator=(BiTemporalTable&&)      = delete;

    // ── DML ──────────────────────────────────────────────────────────────────

    /**
     * Insert a row with an explicit valid-time period.
     * Returns false and leaves the table unchanged when the valid-time period
     * would overlap with an existing current row for the same key.
     */
    bool insertWithValidTime(const std::string& key,
                             const Document& doc,
                             const TimeRange& valid_time);

    /**
     * Update the payload of the current row whose valid-time period contains
     * the given timestamp.
     * The old row's sys_time is closed; a new row is created with the merged
     * data and the same valid-time period.
     * Returns false if no matching current row is found.
     */
    bool updateForValidTime(const std::string& key,
                            const Document& updates,
                            Timestamp valid_at);

    /**
     * Logically delete all current rows for a key whose valid-time period
     * contains the given timestamp.
     * Returns the number of rows closed.
     */
    size_t deleteForValidTime(const std::string& key, Timestamp valid_at);

    // ── Queries ───────────────────────────────────────────────────────────────

    /**
     * @brief Run a bi-temporal AS-OF query.
     * Returns rows that were current at sys_as_of and whose valid-time period
     * contains valid_at.
     * @param key Logical row key to query.
     * @param sys_as_of System-time instant at which the row must have been current.
     * @param valid_at Valid-time instant that the row's valid-time period must contain.
     * @return Matching versions, or an empty vector when no version matches.
     */
    std::vector<VersionedDocument> queryBiTemporal(const std::string& key,
                                                   Timestamp sys_as_of,
                                                   Timestamp valid_at) const;

    /**
     * @brief Return current rows for a key whose valid-time period contains
     * valid_at.
     * @param key Logical row key to query.
     * @param valid_at Valid-time instant to test.
     * @return Matching current versions, or an empty vector when none match.
     */
    std::vector<VersionedDocument> queryCurrentByValidTime(
        const std::string& key, Timestamp valid_at) const;

    /**
     * @brief Detect overlaps among current rows for a key.
     * @param key Logical row key to inspect.
     * @return Pairs of overlapping versions; empty when no overlaps exist.
     */
    std::vector<std::pair<VersionedDocument, VersionedDocument>> findOverlaps(
        const std::string& key) const;

    /**
     * @brief Find uncovered valid-time intervals for a key.
     * Detect gaps in the valid-time coverage of current rows for a key
     * within the half-open interval [@p from, @p to).
     *
     * A gap is a sub-interval within [@p from, @p to) not covered by any
     * current row's valid-time period.
     *
     * Returns an empty vector when the period [@p from, @p to) is fully
     * covered.  Returns `{{from, to}}` when the key has no current rows or
     * none of them overlap the query range (the entire interval is a gap).
     * Returns an empty vector when @p from >= @p to.
     * @param key Logical row key to inspect.
     * @param from Inclusive start of the query interval.
     * @param to Exclusive end of the query interval.
     * @return Uncovered sub-intervals, or an empty vector when none exist or
     *         the query interval is invalid.
     */
    std::vector<TimeRange> findGaps(const std::string& key,
                                    Timestamp from,
                                    Timestamp to) const;

    /**
     * @brief Check whether a valid-time period conflicts with current rows.
     * Check whether inserting a row with the given valid-time @p period for
     * @p key would violate the temporal uniqueness constraint (i.e., overlap
     * with an existing current row).
     *
     * Returns false immediately when @p period is empty or invalid
     * (i.e., `period.start >= period.end`).
     * Returns true when a conflict exists; false when the insert would succeed.
     * @param key Logical row key to check.
     * @param period Candidate half-open valid-time interval.
     * @return true if the period overlaps a current row; false otherwise.
     */
    bool hasUniquenessConflict(const std::string& key,
                                const TimeRange& period) const;

    /**
     * @brief Return all versions (history) for a key.
     * @param key Logical row key to query.
     * @return Versions for the key in stored order, or an empty vector if
     *         the key has no history.
     */
    std::vector<VersionedDocument> getHistory(const std::string& key) const;

    /**
     * @brief Scan rows matching both system and valid time.
     * Returns all rows where sys_time contains sys_as_of AND
     * valid_time contains valid_at.
     * Equivalent to a full-table AS-OF bi-temporal query.
     * @param sys_as_of System-time instant to query.
     * @param valid_at Valid-time instant to query.
     * @return Matching versions, or an empty vector when none match.
     */
    std::vector<VersionedDocument> scanBiTemporal(Timestamp sys_as_of,
                                                   Timestamp valid_at) const;

    /**
     * @brief Return every key ever written to the table.
     * Includes keys whose rows have all been logically deleted. Useful for
     * bi-temporal joins that must enumerate every key ever written.
     * @return All known keys.
     */
    std::vector<std::string> getAllKeys() const;

    // ── Cross-node reconciliation ─────────────────────────────────────────────

    /**
     * Result of a merge operation.
     */
    struct MergeResult {
        size_t rows_inserted{0};   ///< Rows from @p other not present locally
        size_t rows_skipped{0};    ///< Rows that were already present (no diff)
        size_t conflicts_lww{0};   ///< Rows accepted via Last-Writer-Wins (higher sys_time)
    };

    /**
     * Merge all rows from another BiTemporalTable into this table.
     *
     * The merge follows Last-Writer-Wins (LWW) semantics based on
     * `sys_time.start`: for each key, a remote row is considered conflicting
     * with the local table when it overlaps a current local row in valid-time.
     * The row with the later `sys_time.start` wins.
     *
     * Rows whose key has no overlapping current local valid-time are inserted.
     * If table names differ (`tableName() != other.tableName()`), the merge is
     * treated as a no-op and returns zero counters.
     *
     * The operation is atomic on each key (keys are locked one at a time) and
     * does not modify @p other.
     *
     * @param other  Source table.  Must not be the same object as @p this.
     * @return       MergeResult with counters for inserted, skipped, and
     *               conflict-resolved rows.
     */
    MergeResult merge(const BiTemporalTable& other);

    // ── Metadata ─────────────────────────────────────────────────────────────

    /**
     * @brief Return this table's logical name.
     * @return Reference to the name owned by this table, valid for its lifetime.
     */
    const std::string& tableName() const noexcept { return table_name_; }

    /**
     * @brief Count distinct keys retained in the table.
     * @return Number of keys, including keys whose rows were logically deleted.
     */
    size_t keyCount() const;

    /**
     * @brief Count all current and historical row versions.
     * @return Total number of stored versions.
     */
    size_t versionCount() const;

    /**
     * @brief Return a snapshot of table statistics.
     * @return JSON with table_name, key_count, current_rows, historical_rows,
     *         and total_versions.
     */
    nlohmann::json getStatistics() const;

private:
    std::string table_name_;
    std::string source_node_;

    using VersionList = std::vector<VersionedDocument>;
    std::map<std::string, VersionList> rows_;

    mutable std::mutex mutex_;

    // Close the sys_time of all currently-open rows that match the predicate.
    // Returns the number of rows closed.  Caller must hold lock.
    size_t closeCurrentRows(VersionList& versions,
                             Timestamp close_time,
                             const std::function<bool(const VersionedDocument&)>& pred);
};

} // namespace temporal
} // namespace themisdb
