/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bi_temporal.h                                      ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

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
    explicit BiTemporalTable(std::string table_name,
                             std::string source_node = "local");

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
     * Bi-temporal AS-OF query.
     * Returns rows that were current at sys_as_of and whose valid-time period
     * contains valid_at.
     */
    std::vector<VersionedDocument> queryBiTemporal(const std::string& key,
                                                   Timestamp sys_as_of,
                                                   Timestamp valid_at) const;

    /**
     * Return all current rows for a key whose valid-time period contains
     * valid_at.
     */
    std::vector<VersionedDocument> queryCurrentByValidTime(
        const std::string& key, Timestamp valid_at) const;

    /**
     * Detect overlaps among current rows for a key.
     * Returns pairs of overlapping VersionedDocuments.
     */
    std::vector<std::pair<VersionedDocument, VersionedDocument>> findOverlaps(
        const std::string& key) const;

    /**
     * Return all versions (history) for a key.
     */
    std::vector<VersionedDocument> getHistory(const std::string& key) const;

    // ── Metadata ─────────────────────────────────────────────────────────────

    const std::string& tableName() const noexcept { return table_name_; }
    size_t keyCount() const;
    size_t versionCount() const;
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
