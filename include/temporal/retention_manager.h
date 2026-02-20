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
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/** Retention strategy discriminator. */
enum class RetentionType {
    TIME_BASED,          ///< Keep history younger than a given duration
    VERSION_COUNT_BASED, ///< Keep only the N most-recent versions per key
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

    // --- CUSTOM ---
    /// Return true to keep the version, false to delete it.
    std::function<bool(const VersionedDocument&)> should_keep;

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
    std::chrono::milliseconds execution_time{0};
    std::vector<std::string> errors;

    nlohmann::json toJson() const {
        return {{"versions_examined", versions_examined},
                {"versions_deleted", versions_deleted},
                {"versions_archived", versions_archived},
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
 * @note **Read-only analysis in v1.1**: `SystemVersionedTable` is currently
 *       append-only (it has no `purgeVersion()` API).  Therefore
 *       `enforceRetention()` analyses which versions *would* be deleted and
 *       records them in the stats/archive, but does NOT physically remove
 *       them from the table.  A future `purgeVersion()` API on
 *       `SystemVersionedTable` will make enforcement fully destructive.
 *
 * @note **Deleted-key limitation**: Only keys that still have at least one
 *       current (open-ended) row are scanned.  Historical data for fully-
 *       deleted keys is not yet enumerated.  This will be addressed once
 *       `SystemVersionedTable` exposes a full key-list API.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class RetentionManager {
public:
    RetentionManager() = default;

    // ── Policy management ────────────────────────────────────────────────────

    /** Set the retention policy for a named table. */
    void setPolicy(const std::string& table_name, const RetentionPolicy& policy);

    /** Retrieve the retention policy for a table, if set. */
    std::optional<RetentionPolicy> getPolicy(const std::string& table_name) const;

    // ── Enforcement ──────────────────────────────────────────────────────────

    /**
     * Apply the registered policy to the given table.
     * Non-current versions that violate the policy are deleted (and optionally
     * archived).
     */
    RetentionStats enforceRetention(SystemVersionedTable& table);

    /**
     * Apply the given policy directly without registering it.
     */
    RetentionStats enforceRetention(SystemVersionedTable& table,
                                    const RetentionPolicy& policy);

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

    mutable std::mutex mutex_;

    RetentionStats applyPolicy(SystemVersionedTable& table,
                               const RetentionPolicy& policy);
};

} // namespace temporal
} // namespace themisdb
