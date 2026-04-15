/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            retention_manager.h                                ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     263                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • eff45c52df  2026-03-12  Address PR review: fix resolveArchiveTag, retry exception... ║
    • 9587568640  2026-03-12  Implement Automated Retention Policies: STORAGE_BASED typ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Retention Manager
 *
 * Configurable lifecycle management for historical temporal data.
 * Supports time-based, version-count-based, and custom retention policies.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace themisdb {
namespace temporal {

/** Retention strategy discriminator. */
enum class RetentionType {
    TIME_BASED,          ///< Keep history younger than a given duration
    VERSION_COUNT_BASED, ///< Keep only the N most-recent versions per key
    STORAGE_BASED,       ///< Keep historical data up to a maximum total size
    CUSTOM               ///< User-supplied predicate
};

/**
 * Policy that governs how historical data is cleaned up.
 */
struct RetentionPolicy {
    RetentionType type{RetentionType::TIME_BASED};

    // --- TIME_BASED ---
    std::chrono::milliseconds retention_period{
        std::chrono::hours(24 * 365)}; ///< Default: 1 year

    // --- VERSION_COUNT_BASED ---
    size_t max_versions_per_key{10};

    // --- STORAGE_BASED ---
    /// Maximum total bytes of historical (non-current) versions for the table.
    /// When total historical storage exceeds this limit the oldest versions are
    /// removed first.  0 means unlimited (no storage-based enforcement).
    uint64_t max_storage_bytes{0};

    // --- CUSTOM ---
    /// Return true to keep the version, false to delete it.
    std::function<bool(const VersionedDocument&)> should_keep;

    // --- Compliance ---
    /// Regulatory label for this policy (e.g. "GDPR", "HIPAA").
    /// Used as the archive_tag fallback (prefixed with the table name so that
    /// getArchivedRecords("<table>") can still locate the records) and
    /// propagated to ArchivedRecord::archive_tag when archive_before_delete is set.
    std::string compliance_tag;

    /// Minimum age a version must reach before it may be deleted by this
    /// policy.  Versions younger than this are always kept regardless of the
    /// primary retention type.  Zero means no minimum (default).
    std::chrono::milliseconds minimum_retention_period{0};

    // --- Incremental enforcement ---
    /// Maximum number of versions deleted in a single enforcement run.
    /// 0 means unlimited (delete all eligible versions in one pass).
    size_t incremental_batch_size{0};

    // --- Retry ---
    /// How many times to retry enforcement when errors are encountered.
    /// 0 means no retry (single attempt only).
    int max_retries{0};

    // --- Archiving ---
    bool archive_before_delete{false};
    std::string archive_tag; ///< Label attached to archived entries
};

/**
 * Statistics produced by a single retention enforcement run.
 */
struct RetentionStats {
    size_t versions_examined{0};
    size_t versions_deleted{0};
    size_t versions_archived{0};
    uint64_t space_freed_bytes{0};        ///< Estimated bytes freed by deletions
    std::chrono::milliseconds execution_time{0};
    std::vector<std::string> errors;

    nlohmann::json toJson() const {
        return {{"versions_examined", versions_examined},
                {"versions_deleted", versions_deleted},
                {"versions_archived", versions_archived},
                {"space_freed_bytes", space_freed_bytes},
                {"execution_time_ms", execution_time.count()},
                {"errors", errors}};
    }
};

/**
 * An archived record (a version that was removed by a retention run).
 */
struct ArchivedRecord {
    VersionedDocument document;
    std::string archive_tag;
    Timestamp archived_at;
};

/**
 * RetentionManager
 *
 * Applies retention policies to SystemVersionedTable instances to enforce
 * data-lifecycle requirements.  Archived versions are kept in an in-memory
 * archive for the duration of the process; in production these would be
 * written to cold storage.
 *
 * @note `enforceRetention()` physically removes historical versions that
 *       violate the policy via `SystemVersionedTable::purgeHistoricalVersions()`.
 *       The current (live) row is never removed.  If `archive_before_delete`
 *       is set, a copy is placed in the in-memory archive before deletion.
 *
 * @note **Deleted-key coverage**: `getAllKeys()` is used so that even fully-
 *       deleted keys (no current row) are included in the retention scan.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class RetentionManager {
public:
    RetentionManager() = default;
    ~RetentionManager();

    // ── Policy management ────────────────────────────────────────────────────

    /** Set the retention policy for a named table. */
    void setPolicy(const std::string& table_name, const RetentionPolicy& policy);

    /** Retrieve the retention policy for a table, if set. */
    std::optional<RetentionPolicy> getPolicy(const std::string& table_name) const;

    // ── Enforcement ──────────────────────────────────────────────────────────

    /**
     * Apply the registered policy to the given table.
     * Non-current versions that violate the policy are physically deleted
     * (and optionally archived before deletion).
     */
    RetentionStats enforceRetention(SystemVersionedTable& table);

    /**
     * Apply the given policy directly without registering it.
     */
    RetentionStats enforceRetention(SystemVersionedTable& table,
                                    const RetentionPolicy& policy);

    // ── Background Scheduler ─────────────────────────────────────────────────

    /**
     * Register a table for periodic background retention enforcement.
     *
     * The scheduler runs in a dedicated thread.  Each registered table is
     * checked every `interval` and the previously-registered policy
     * (setPolicy) is applied.  The table pointer must remain valid until
     * stopScheduler() or the RetentionManager is destroyed.
     *
     * @param table     Reference to the table to maintain.
     * @param interval  How often to enforce the policy (minimum 1 millisecond).
     *
     * Call startScheduler() once to activate background processing.
     */
    void scheduleTable(SystemVersionedTable& table,
                       std::chrono::milliseconds interval);

    /**
     * Start the background retention thread.
     * Calling this more than once is a no-op.
     */
    void startScheduler();

    /**
     * Stop the background retention thread and wait for it to exit.
     */
    void stopScheduler();

    /** Return true if the background scheduler is currently running. */
    bool schedulerRunning() const noexcept;

    // ── Archive ──────────────────────────────────────────────────────────────

    /** Return all archived records (across all tables). */
    std::vector<ArchivedRecord> getArchivedRecords() const;

    /** Return archived records for a specific table. */
    std::vector<ArchivedRecord> getArchivedRecords(
        const std::string& table_name) const;

    /** Clear the in-memory archive. */
    void clearArchive();

    // ── Statistics ───────────────────────────────────────────────────────────

    nlohmann::json getCumulativeStats() const;

private:
    std::map<std::string, RetentionPolicy> policies_;
    std::vector<ArchivedRecord> archive_;

    // Cumulative counters
    size_t total_deleted_{0};
    size_t total_archived_{0};
    uint64_t total_space_freed_bytes_{0};

    mutable std::mutex mutex_;

    // Background scheduler state
    struct ScheduledTable {
        SystemVersionedTable* table;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point next_run;
    };
    std::vector<ScheduledTable> scheduled_tables_;

    std::thread scheduler_thread_;
    std::atomic<bool> scheduler_running_{false};
    std::atomic<bool> scheduler_stop_{false};

    void schedulerLoop();

    RetentionStats applyPolicy(SystemVersionedTable& table,
                               const RetentionPolicy& policy);
};

} // namespace temporal
} // namespace themisdb
