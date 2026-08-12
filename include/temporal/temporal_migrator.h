/**
 * @file temporal_migrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Migrator
 *
 * Converts existing plain document collections to SQL:2011-compatible
 * system-versioned tables with optional history backfill from audit logs
 * or change-tracking snapshots.
 *
 * ## Features
 * - Analyze an existing key/document set to produce a MigrationPlan
 * - Execute the migration: create a SystemVersionedTable and insert all
 *   current rows with correct system timestamps
 * - Backfill historical versions from a caller-supplied audit log
 * - Verify post-migration data integrity (key count, version count,
 *   temporal ordering)
 * - Progress reporting via MigrationStatus enum and optional callback
 *
 * ## Usage
 * ```cpp
 * TemporalMigrator migrator;
 *
 * // Step 1 – analyse
 * auto plan = migrator.analyzeMigration("employees", current_rows);
 *
 * // Step 2 – migrate
 * auto [table, ok] = migrator.migrateToTemporal(plan, current_rows);
 *
 * // (Optional) Step 3 – backfill history
 * migrator.backfillHistory(table, historical_versions);
 *
 * // Step 4 – verify
 * auto report = migrator.verifyMigration(table);
 * assert(report.success);
 * ```
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace temporal {

// ============================================================================
// MigrationStatus
// ============================================================================

/** Lifecycle state of a single migration run. */
enum class MigrationStatus {
    PENDING,    ///< Not yet started
    ANALYZING,  ///< analyzeMigration() in progress
    MIGRATING,  ///< migrateToTemporal() in progress
    VERIFYING,  ///< verifyMigration() in progress
    COMPLETE,   ///< Migration completed successfully
    FAILED      ///< Migration failed; see MigrationReport::errors
};

// ============================================================================
// ColumnInfo
// ============================================================================

/**
 * Metadata about a single column inferred from the source documents.
 *
 * During analyzeMigration() the migrator samples all source documents and
 * collects the distinct field names together with the JSON type observed for
 * the majority of values.
 */
struct ColumnInfo {
    std::string name;          ///< JSON field name
    std::string inferred_type; ///< "string", "number", "boolean", "object", "array", "null"
    bool        nullable{true};///< true when the field is absent in at least one document

    nlohmann::json toJson() const {
        return {{"name", name}, {"type", inferred_type}, {"nullable", nullable}};
    }
};

// ============================================================================
// MigrationPlan
// ============================================================================

/**
 * A migration plan produced by TemporalMigrator::analyzeMigration().
 *
 * The plan is a value type: it can be inspected, adjusted, and then passed
 * to migrateToTemporal() for execution.
 */
struct MigrationPlan {
    // ── Source description ────────────────────────────────────────────────────

    /** Name of the source (non-versioned) table / collection. */
    std::string source_table_name;

    /** Number of key/document pairs in the source collection. */
    size_t source_row_count{0};

    /** Inferred schema of the source documents (one entry per distinct field). */
    std::vector<ColumnInfo> columns;

    // ── Target configuration ──────────────────────────────────────────────────

    /**
     * Configuration for the resulting SystemVersionedTable.
     * Defaults are usually sufficient; callers may override before passing
     * the plan to migrateToTemporal().
     */
    SystemVersionedTable::Config versioned_config;

    /**
     * Baseline timestamp to assign as `sys_start` for current rows that have
     * no known creation time.  Defaults to now() at the time
     * analyzeMigration() was called.
     */
    Timestamp baseline_timestamp{0};

    /**
     * When true, migrateToTemporal() expects a subsequent backfillHistory()
     * call with historical audit-log entries.
     */
    bool has_history_to_backfill{false};

    // ── Validation flags ──────────────────────────────────────────────────────

    /** Whether all source keys are unique (detected during analysis). */
    bool keys_are_unique{true};

    /** Whether any source document was found to be empty ({}). */
    bool has_empty_documents{false};

    nlohmann::json toJson() const;
};

// ============================================================================
// MigrationStats
// ============================================================================

/** Runtime counters collected during a migration run. */
struct MigrationStats {
    size_t rows_migrated{0};          ///< Current rows written to the versioned table
    size_t versions_backfilled{0};    ///< Historical versions inserted via backfillHistory()
    size_t rows_skipped{0};           ///< Rows skipped (e.g. duplicate keys)
    size_t validation_errors{0};      ///< Issues found during verifyMigration()
    std::chrono::milliseconds elapsed_ms{0}; ///< Wall-clock time for the migration step

    /** Non-fatal warnings / error messages accumulated during migration. */
    std::vector<std::string> errors;

    nlohmann::json toJson() const;
};

// ============================================================================
// ValidationResult
// ============================================================================

/** A single data-integrity check result produced by verifyMigration(). */
struct ValidationResult {
    std::string check_name;  ///< Human-readable name of the check
    bool        passed{true};
    std::string detail;      ///< Explanation (especially on failure)

    nlohmann::json toJson() const {
        return {{"check", check_name}, {"passed", passed}, {"detail", detail}};
    }
};

// ============================================================================
// MigrationReport
// ============================================================================

/**
 * Summary produced by verifyMigration().
 *
 * `success` is true only when ALL ValidationResult entries have `passed == true`.
 */
struct MigrationReport {
    bool                          success{false};
    std::string                   table_name;
    MigrationStats                stats;
    std::vector<ValidationResult> checks;

    /** Aggregate: number of checks that failed. */
    size_t failedCheckCount() const;

    nlohmann::json toJson() const;
};

// ============================================================================
// TemporalMigrator
// ============================================================================

/**
 * @brief Migrate existing document collections to system-versioned tables.
 *
 * TemporalMigrator is the single entry-point for Phase-5 table migration.
 * It follows a four-step workflow:
 *
 * 1. **analyzeMigration** – inspect source documents, infer schema, build plan
 * 2. **migrateToTemporal** – create and populate a SystemVersionedTable
 * 3. **backfillHistory** (optional) – insert historical audit-log versions
 * 4. **verifyMigration** – run integrity checks and return a MigrationReport
 *
 * ### Progress callbacks
 * Register an optional callback via setProgressCallback() to receive
 * MigrationStatus transitions.  The callback is invoked from the calling
 * thread, never concurrently.
 *
 * ### Thread-safety
 * A single TemporalMigrator instance should not be shared across threads
 * without external locking.  Creating one migrator per migration task is
 * the recommended pattern.
 */
class TemporalMigrator {
public:
    using ProgressCallback = std::function<void(MigrationStatus, const std::string& message)>;

    TemporalMigrator() = default;

    // Non-copyable; movable
    TemporalMigrator(const TemporalMigrator&)            = delete;
    TemporalMigrator& operator=(const TemporalMigrator&) = delete;
    TemporalMigrator(TemporalMigrator&&)                 = default;
    TemporalMigrator& operator=(TemporalMigrator&&)      = default;

    // ── Configuration ─────────────────────────────────────────────────────────

    /**
     * Register a callback that is invoked on every MigrationStatus transition.
     *
     * Pass nullptr to remove any previously registered callback.
     */
    void setProgressCallback(ProgressCallback cb);

    // ── Step 1: Analyse ───────────────────────────────────────────────────────

    /**
     * Analyse a source document collection and produce a MigrationPlan.
     *
     * @param table_name   Logical name of the source table / collection.
     * @param source_docs  Map from row key to document payload.  An empty
     *                     map is accepted (the plan will have source_row_count=0).
     * @return             A MigrationPlan ready for inspection and use in
     *                     migrateToTemporal().
     */
    MigrationPlan analyzeMigration(
        const std::string& table_name,
        const std::unordered_map<std::string, Document>& source_docs);

    // ── Step 2: Migrate ───────────────────────────────────────────────────────

    /**
     * Execute the migration described by @p plan.
     *
     * Creates a SystemVersionedTable and inserts every entry from
     * @p source_docs as the initial current version.  The `sys_start`
     * timestamp for each row is taken from plan.baseline_timestamp unless
     * the document contains a field named `"_created_at"` (interpreted as a
     * millisecond-epoch integer).
     *
     * @param plan         Plan produced by analyzeMigration().
     * @param source_docs  The actual key/document pairs to migrate.
     * @return             Pair of (populated SystemVersionedTable, success flag).
     *                     On failure, stats.errors describes what went wrong.
     */
    std::pair<SystemVersionedTable, bool> migrateToTemporal(
        const MigrationPlan& plan,
        const std::unordered_map<std::string, Document>& source_docs);

    // ── Step 3: Backfill history (optional) ───────────────────────────────────

    /**
     * Insert historical versions from an audit log into an already-migrated
     * SystemVersionedTable.
     *
     * Each VersionedDocument in @p history_entries is inserted as a closed
     * (historical) version.  Entries with sys_time.end == kMaxTimestamp are
     * treated as "open" and are skipped with a warning because they would
     * conflict with the current version inserted during migrateToTemporal().
     *
     * @param table            Target SystemVersionedTable (already migrated).
     * @param history_entries  Audit-log snapshots to backfill.
     * @return                 Number of historical versions successfully inserted.
     */
    size_t backfillHistory(
        SystemVersionedTable& table,
        const std::vector<VersionedDocument>& history_entries);

    // ── Step 4: Verify ────────────────────────────────────────────────────────

    /**
     * Run a suite of data-integrity checks against a migrated table.
     *
     * Checks performed:
     * - KEY_COUNT: total key count matches plan.source_row_count (if plan was set)
     * - VERSION_ORDER: for every key, versions are in ascending sys_start order
     * - NO_OVERLAPPING_VERSIONS: no two versions for the same key overlap in sys_time
     * - CURRENT_VERSION_OPEN: all current versions have sys_time.end == kMaxTimestamp
     * - HISTORY_CONTINUITY: closed versions form a contiguous chain (no gaps)
     *   (only checked when has_history_to_backfill was true in the plan)
     *
     * @param table        The migrated SystemVersionedTable to verify.
     * @return             MigrationReport with all check results and aggregated stats.
     */
    MigrationReport verifyMigration(const SystemVersionedTable& table);

    // ── Status / diagnostics ──────────────────────────────────────────────────

    /** Return the current migration lifecycle state. */
    MigrationStatus getStatus() const noexcept { return status_; }

    /** Return accumulated stats from the most recent migration run. */
    const MigrationStats& getStats() const noexcept { return stats_; }

    /**
     * Return the MigrationReport from the most recent verifyMigration() call.
     * Returns a default-constructed report if verifyMigration() has not been
     * called yet.
     */
    const MigrationReport& getLastReport() const noexcept { return last_report_; }

    // ── Static helpers ────────────────────────────────────────────────────────

    /** Convert MigrationStatus to a human-readable string. */
    static std::string statusName(MigrationStatus s);

private:
    MigrationStatus  status_{MigrationStatus::PENDING};
    MigrationStats   stats_;
    MigrationReport  last_report_;
    MigrationPlan    last_plan_;
    ProgressCallback progress_cb_;

    void setStatus(MigrationStatus s, const std::string& msg = "");

    /** Infer column metadata by sampling all documents. */
    static std::vector<ColumnInfo> inferColumns(
        const std::unordered_map<std::string, Document>& docs);

    /** Extract the inferred JSON type name for a single value. */
    static std::string inferType(const nlohmann::json& value);
};

} // namespace temporal
} // namespace themisdb
