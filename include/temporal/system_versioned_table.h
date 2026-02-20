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
 * Every mutating operation (insert / update / delete) on the current row
 * automatically creates a history entry.  Rows are stored as a sequence of
 * VersionedDocument entries keyed by (key, sys_start).
 *
 * Thread-safety: all public methods are thread-safe.
 */
class SystemVersionedTable {
public:
    explicit SystemVersionedTable(std::string table_name,
                                  std::string source_node = "local");

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

    // ── Metadata ─────────────────────────────────────────────────────────────

    const std::string& tableName() const noexcept { return table_name_; }

    /** Number of distinct keys (including deleted ones). */
    size_t keyCount() const;

    /** Total number of row versions stored (current + historical). */
    size_t versionCount() const;

    /** JSON statistics for monitoring. */
    nlohmann::json getStatistics() const;

private:
    std::string table_name_;
    std::string source_node_;

    // key → ordered list of versions (ascending sys_start)
    using VersionList = std::vector<VersionedDocument>;
    std::map<std::string, VersionList> rows_;

    mutable std::mutex mutex_;

    // Close the current (open-ended) version for a key.  Caller must hold lock.
    void closeCurrentVersion(VersionList& versions, Timestamp close_time);
};

} // namespace temporal
} // namespace themisdb
