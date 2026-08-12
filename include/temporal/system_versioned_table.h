/**
 * @file system_versioned_table.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB System-Versioned Table
 *
 * Provides SQL:2011-compliant system-time versioning with automatic history
 * maintenance for insert, update and delete operations.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * SystemVersionedTable
 *
 * SQL:2011-compliant system-time versioning.  Every mutating operation
 * (insert / update / delete) on the current row automatically creates a
 * history entry.  Rows are stored as a sequence of VersionedDocument entries
 * keyed by (key, sys_start).
 *
 * Thread-safety: all public methods are thread-safe.
 */
class SystemVersionedTable {
public:
    /**
     * Configuration for system-versioned table behaviour.
     *
     * Follows the design specified in FUTURE_ENHANCEMENTS.md §Full
     * System-Versioned Table Support (v1.1.0).
     */
    struct Config {
        /** Name of the logical history table (informational; used in
         *  statistics/logging).  Defaults to "<table>_history". */
        std::string history_table_name;

        /** When true, historical versions are marked as eligible for
         *  compression.  This flag is currently reserved for future
         *  compression integration and does not change runtime behaviour. */
        bool compress_history = true;

        /** Maximum age of historical versions before automatic purge.
         *  Default: 1 year.  Set to 0 to disable automatic purging. */
        std::chrono::milliseconds retention_period{365LL * 24 * 3600 * 1000};

        /** When true, the source node / user identifier is recorded in
         *  VersionedDocument::modified_by on every DML operation. */
        bool track_user_id = true;
    };

    // ── Construction ─────────────────────────────────────────────────────────

    /**
     * Construct with explicit table name and optional source-node label.
     * Uses default Config values.
     */
    explicit SystemVersionedTable(std::string table_name,
                                  std::string source_node = "local");

    /**
     * Construct with full Config.
     *
     * @param table_name   Name of this table.
     * @param config       Runtime configuration (retention, compression, …).
     * @param source_node  Node/user label written to VersionedDocument::modified_by.
     */
    SystemVersionedTable(std::string table_name,
                         Config config,
                         std::string source_node = "local");

    /**
     * Move constructor.  Required because std::mutex is not movable; we
     * default-construct the destination mutex (which is always correct for a
     * newly-constructed object that nobody else holds a lock to).
     */
    SystemVersionedTable(SystemVersionedTable&& other) noexcept;

    /**
     * Static factory that creates a system-versioned table from a schema
     * descriptor (arbitrary JSON) and a Config.
     *
     * The schema is stored in the table's statistics under "schema" and can
     * be retrieved via getStatistics().  It does not affect storage behaviour
     * but enables DDL-aware tools to inspect column definitions.
     *
     * @param table_name  Logical table name (used for the history_table_name
     *                    default if config.history_table_name is empty).
     * @param schema      JSON object describing the table columns/types.
     * @param config      Runtime configuration.
     * @param source_node Source-node label for DML attribution.
     * @return            Fully configured SystemVersionedTable instance.
     */
    static SystemVersionedTable createVersionedTable(
        const std::string&  table_name,
        const Document&     schema,
        Config              config,
        const std::string&  source_node  = "local");

    /** Overload with default Config. */
    static SystemVersionedTable createVersionedTable(
        const std::string&  table_name,
        const Document&     schema);

    // ── DML ──────────────────────────────────────────────────────────────────

    /** Insert a new row.  Fails (returns false) if the key already exists. */
    bool insert(const std::string& key, const Document& doc);

    /**
     * Update an existing current row.
     * The previous version is closed (sys_end set to now) and a new version is
     * opened.  Returns false if no current row exists for the key.
     */
    bool update(const std::string& key, const Document& updates);

    /**
     * Insert a new row or update an existing one atomically.
     * If a current version exists the row is updated (patch-merge semantics);
     * otherwise a fresh row is inserted.
     *
     * @return true if an insert was performed, false if an update was performed.
     */
    bool upsert(const std::string& key, const Document& doc);

    /**
     * Logically delete the current row (closes its sys_time period).
     * Returns false if no current row exists for the key.
     */
    bool deleteRow(const std::string& key);

    // ── Queries ───────────────────────────────────────────────────────────────

    /** Return the current version of a row, if it exists. */
    std::optional<VersionedDocument> getCurrent(const std::string& key) const;

    /** Return the version that was current at the given timestamp. */
    std::optional<VersionedDocument> getAsOf(const std::string& key,
                                             Timestamp as_of) const;

    /** Return all historical versions of a row. */
    std::vector<VersionedDocument> getHistory(const std::string& key) const;

    /**
     * Return all versions of a row whose sys_time overlaps the given range.
     */
    std::vector<VersionedDocument> getHistoryInRange(const std::string& key,
                                                     const TimeRange& range) const;

    /**
     * Return all current rows as a snapshot at the given timestamp.
     * When as_of == kMaxTimestamp the latest current rows are returned.
     */
    std::vector<VersionedDocument> scan(Timestamp as_of = kMaxTimestamp) const;

    /**
     * Return all known keys (including keys whose rows have all been deleted).
     * This allows callers (e.g. RetentionManager) to enumerate every key that
     * ever had data, not just keys with a currently-alive row.
     */
    std::vector<std::string> getAllKeys() const;

    /**
     * Physically remove all closed (historical) versions for the given key
     * that match the supplied predicate.
     *
     * The current (open-ended) version is NEVER removed.
     *
     * @param key       The row key to purge.
     * @param predicate Return true for versions that should be deleted.
     * @return          The number of versions actually removed.
     */
    size_t purgeHistoricalVersions(
        const std::string& key,
        const std::function<bool(const VersionedDocument&)>& predicate);

    /**
     * Physically keep only the `keep_latest_n` most-recent historical
     * (closed) versions for a key, deleting the rest.
     *
     * The current (open-ended) version is always kept regardless of n.
     *
     * @param key           Row key.
     * @param keep_latest_n Number of historical versions to retain.
     * @return              Number of versions physically removed.
     */
    size_t purgeHistoricalVersionsKeepLatestN(const std::string& key,
                                              size_t keep_latest_n);

    /**
     * Convenience overload: purge all historical versions across every known
     * key that satisfy the predicate.
     *
     * @return  Total number of versions removed.
     */
    size_t purgeHistoricalVersions(
        const std::function<bool(const VersionedDocument&)>& predicate);

    /**
     * Replace the `data` payload of an existing historical version in-place.
     *
     * Identifies the version by `key` and the exact `sys_start` timestamp.
     * Only closed (non-current) versions may be replaced; attempting to
     * replace a current version returns false.
     *
     * This method is intended for use by TemporalCompressor to substitute
     * a compressed payload without altering the version's time metadata.
     *
     * @param key        Row key.
     * @param sys_start  sys_time.start of the target version.
     * @param new_data   Replacement payload (may be compressed).
     * @return           true if the version was found and replaced.
     */
    bool replaceHistoricalPayload(const std::string& key,
                                  Timestamp sys_start,
                                  const Document& new_data);

    // ── Retention ─────────────────────────────────────────────────────────────

    /**
     * Apply the configured retention policy to all keys.
     *
     * Historical versions older than Config::retention_period are physically
     * removed.  If Config::retention_period is zero the call is a no-op.
     * The current (open-ended) version is never removed.
     *
     * @return Number of historical versions physically removed.
     */
    size_t enforceRetentionPolicy();

    // ── Metadata ─────────────────────────────────────────────────────────────

    const std::string& tableName() const noexcept { return table_name_; }

    /** Returns the active Config for this table. */
    const Config& getConfig() const noexcept { return config_; }

    /** Number of distinct keys (including deleted ones). */
    size_t keyCount() const;

    /** Total number of row versions stored (current + historical). */
    size_t versionCount() const;

    /** JSON statistics for monitoring. */
    nlohmann::json getStatistics() const;

private:
    std::string table_name_;
    std::string source_node_;
    Config      config_;
    Document    schema_;   ///< Table schema provided via createVersionedTable()

    // key → ordered list of versions (ascending sys_start)
    using VersionList = std::vector<VersionedDocument>;
    std::map<std::string, VersionList> rows_;

    mutable std::mutex mutex_;

    // Close the current (open-ended) version for a key.  Caller must hold lock.
    void closeCurrentVersion(VersionList& versions, Timestamp close_time);

    // Build a VersionedDocument for a new (or replacement) row version.
    VersionedDocument makeVersion(const std::string& key,
                                  Document data,
                                  Timestamp ts) const;
};

} // namespace temporal
} // namespace themisdb
