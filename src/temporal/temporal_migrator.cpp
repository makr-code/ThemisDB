/**
 * @file temporal_migrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Migrator Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_migrator.h"
#include <algorithm>
#include <chrono>
#include <unordered_set>

namespace themisdb {
namespace temporal {

// ============================================================================
// Static helpers
// ============================================================================

std::string TemporalMigrator::statusName(MigrationStatus s) {
    switch (s) {
        case MigrationStatus::PENDING:   return "PENDING";
        case MigrationStatus::ANALYZING: return "ANALYZING";
        case MigrationStatus::MIGRATING: return "MIGRATING";
        case MigrationStatus::VERIFYING: return "VERIFYING";
        case MigrationStatus::COMPLETE:  return "COMPLETE";
        case MigrationStatus::FAILED:    return "FAILED";
    }
    return "UNKNOWN";
}

std::string TemporalMigrator::inferType(const nlohmann::json& value) {
    if (value.is_null()) {
      return "null";
    }
    if (value.is_boolean()) {
      return "boolean";
    }
    if (value.is_number()) {
      return "number";
    }
    if (value.is_string()) {
      return "string";
    }
    if (value.is_array()) {
      return "array";
    }
    if (value.is_object()) {
      return "object";
    }
    return "unknown";
}

std::vector<ColumnInfo> TemporalMigrator::inferColumns(
    const std::unordered_map<std::string, Document>& docs) {

    // Collect per-field: dominant type + presence count
    struct FieldStats {
        std::unordered_map<std::string, size_t> type_counts;
        size_t present_count{0};
    };
    std::unordered_map<std::string, FieldStats> field_map;

    for (const auto& [key, doc] : docs) {
        if (!doc.is_object()) {
          continue;
        }
        for (const auto& [field, val] : doc.items()) {
            auto& fs = field_map[field];
            fs.present_count++;
            fs.type_counts[inferType(val)]++;
        }
    }

    const size_t total = docs.size();
    std::vector<ColumnInfo> columns = {};

    columns.reserve(field_map.size());

    for (auto& [name, fs] : field_map) {
        ColumnInfo ci;
        ci.name     = name;
        ci.nullable = (fs.present_count < total);

        // Dominant type = type with the highest occurrence count
        std::string dominant = {};
        size_t      max_count = 0;
        for (const auto& [type, cnt] : fs.type_counts) {
            if (cnt > max_count) { max_count = cnt; dominant = type; }
        }
        ci.inferred_type = dominant.empty() ? "unknown" : dominant;
        columns.push_back(std::move(ci));
    }

    // Stable sort by name for deterministic output
    std::sort(columns.begin(), columns.end(),
              [](const ColumnInfo& a, const ColumnInfo& b) {
                  return a.name < b.name;
              });

    return columns;
}

// ============================================================================
// MigrationPlan serialisation
// ============================================================================

nlohmann::json MigrationPlan::toJson() const {
    nlohmann::json j;
    j["source_table_name"]     = source_table_name;
    j["source_row_count"]      = source_row_count;
    j["baseline_timestamp"]    = baseline_timestamp;
    j["has_history_to_backfill"] = has_history_to_backfill;
    j["keys_are_unique"]       = keys_are_unique;
    j["has_empty_documents"]   = has_empty_documents;

    nlohmann::json cols = nlohmann::json::array();
    for (const auto& c : columns) { cols.push_back(c.toJson()); }
    j["columns"] = cols;

    return j;
}

// ============================================================================
// MigrationStats serialisation
// ============================================================================

nlohmann::json MigrationStats::toJson() const {
    nlohmann::json j;
    j["rows_migrated"]       = rows_migrated;
    j["versions_backfilled"] = versions_backfilled;
    j["rows_skipped"]        = rows_skipped;
    j["validation_errors"]   = validation_errors;
    j["elapsed_ms"]          = elapsed_ms.count();
    j["errors"]              = errors;
    return j;
}

// ============================================================================
// MigrationReport helpers
// ============================================================================

size_t MigrationReport::failedCheckCount() const {
    size_t n = 0;
    for (const auto& c : checks) { if (!c.passed) ++n; }
    return n;
}

nlohmann::json MigrationReport::toJson() const {
    nlohmann::json j;
    j["success"]    = success;
    j["table_name"] = table_name;
    j["stats"]      = stats.toJson();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& c : checks) { arr.push_back(c.toJson()); }
    j["checks"] = arr;
    return j;
}

// ============================================================================
// TemporalMigrator – internal helpers
// ============================================================================

void TemporalMigrator::setStatus(MigrationStatus s, const std::string& msg) {
    status_ = s;
    if (progress_cb_) {
        progress_cb_(s, msg.empty() ? statusName(s) : msg);
    }
}

void TemporalMigrator::setProgressCallback([[maybe_unused]] ProgressCallback cb) {
    progress_cb_ = std::move(cb);
}

// ============================================================================
// Step 1: analyzeMigration
// ============================================================================

MigrationPlan TemporalMigrator::analyzeMigration(
    const std::string& table_name,
    const std::unordered_map<std::string, Document>& source_docs) {

    setStatus(MigrationStatus::ANALYZING,
              "Analyzing table '" + table_name + "' ("
              + std::to_string(source_docs.size()) + " rows)");

    MigrationPlan plan;
    plan.source_table_name  = table_name;
    plan.source_row_count   = source_docs.size();
    plan.baseline_timestamp = now();

    // Infer schema
    plan.columns = inferColumns(source_docs);

    // Check uniqueness – map keys are inherently unique in std::unordered_map,
    // but we expose this flag for downstream consumers that may be working with
    // raw arrays.
    plan.keys_are_unique = true; // unordered_map guarantees uniqueness

    // Detect empty documents
    for (const auto& [key, doc] : source_docs) {
        if (doc.empty() || doc.is_null()) {
            plan.has_empty_documents = true;
            break;
        }
    }

    // Default versioned_config
    plan.versioned_config.history_table_name = table_name + "_history";
    plan.versioned_config.compress_history   = true;
    plan.versioned_config.track_user_id      = true;

    last_plan_ = plan;
    setStatus(MigrationStatus::PENDING, "Analysis complete");
    return plan;
}

// ============================================================================
// Step 2: migrateToTemporal
// ============================================================================

std::pair<SystemVersionedTable, bool> TemporalMigrator::migrateToTemporal(
    const MigrationPlan& plan,
    const std::unordered_map<std::string, Document>& source_docs) {

    setStatus(MigrationStatus::MIGRATING,
              "Migrating " + std::to_string(source_docs.size()) +
              " rows to system-versioned table '" + plan.source_table_name + "'");

    stats_ = MigrationStats{};
    const auto start_wall = std::chrono::steady_clock::now();

    // Build schema document from plan columns
    nlohmann::json schema = nlohmann::json::object();
    for (const auto& col : plan.columns) {
        schema[col.name] = col.inferred_type;
    }

    SystemVersionedTable table = SystemVersionedTable::createVersionedTable(
        plan.source_table_name, schema, plan.versioned_config, "migrator");

    bool overall_ok = true;

    for (const auto& [key, doc] : source_docs) {
        // Insert into the versioned table.  SystemVersionedTable::insert()
        // sets sys_start = now(); sys_start reflects migration time.
        bool inserted = table.insert(key, doc);
        if (inserted) {
            ++stats_.rows_migrated;
        } else {
            // Row was not inserted (shouldn't happen for fresh table, but guard
            // against duplicate keys in an ill-formed source map).
            ++stats_.rows_skipped;
            stats_.errors.push_back("Duplicate or failed insert for key: " + key);
            overall_ok = false;
        }
    }

    const auto end_wall = std::chrono::steady_clock::now();
    stats_.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_wall - start_wall);

    if (overall_ok) {
        setStatus(MigrationStatus::COMPLETE,
                  "Migration complete: " + std::to_string(stats_.rows_migrated) + " rows");
    } else {
        setStatus(MigrationStatus::FAILED,
                  "Migration finished with " + std::to_string(stats_.rows_skipped) + " skip(s)");
    }

    return {std::move(table), overall_ok};
}

// ============================================================================
// Step 3: backfillHistory
// ============================================================================

size_t TemporalMigrator::backfillHistory(
    SystemVersionedTable& table,
    const std::vector<VersionedDocument>& history_entries) {

    size_t inserted = 0;

    for (const auto& entry : history_entries) {
        // Skip open-ended versions (they would clash with the current version)
        if (entry.sys_time.end == kMaxTimestamp) {
            stats_.errors.push_back(
                "Skipped open-ended backfill entry for key: " + entry.key);
            continue;
        }

        // Inject as a closed historical version via the replaceHistoricalPayload
        // pathway: first insert a placeholder if the key is not present, then
        // use replaceHistoricalPayload.  For a fully correct backfill we insert
        // the entry as a normal insert and then close it with the correct
        // sys_end.  The SystemVersionedTable API does not expose a direct
        // "inject historical version" method, so we use the update path:
        //   1. Insert current version at entry.sys_time.start (if key absent)
        //   2. Immediately close it (update with a no-op) to push the start
        //      version into history.
        // For the purposes of this migrator we expose the replaceHistoricalPayload
        // API: insert the entry and patch the payload/timestamp via the public API.
        // The cleanest approach: use the table insert + deleteRow flow to create a
        // closed version with the correct data, then patch the payload.
        bool needs_insert = !table.getAsOf(entry.key, entry.sys_time.start).has_value();
        if (needs_insert) {
            table.insert(entry.key, entry.data);
        }

        // Attempt to replace the payload of the historical slot closest to
        // entry.sys_time.start.
        bool replaced = table.replaceHistoricalPayload(
            entry.key, entry.sys_time.start, entry.data);

        if (replaced || !needs_insert) {
            ++inserted;
            ++stats_.versions_backfilled;
        } else {
            // Fallback: the slot didn't exist at that exact sys_start because
            // we used a live insert (sys_start = now()).  Record as a warning.
            stats_.errors.push_back(
                "Could not pin backfill version to sys_start=" +
                std::to_string(entry.sys_time.start) + " for key: " + entry.key);
        }
    }

    return inserted;
}

// ============================================================================
// Step 4: verifyMigration
// ============================================================================

MigrationReport TemporalMigrator::verifyMigration(const SystemVersionedTable& table) {
    setStatus(MigrationStatus::VERIFYING,
              "Verifying table '" + table.tableName() + "'");

    MigrationReport report;
    report.table_name = table.tableName();
    report.stats      = stats_;

    // ── Check 1: KEY_COUNT ───────────────────────────────────────────────────
    {
        ValidationResult r;
        r.check_name = "KEY_COUNT";
        const size_t actual   = table.keyCount();
        const size_t expected = last_plan_.source_row_count;
        if (expected == 0 || actual == expected) {
            r.passed = true;
            r.detail = "key count = " + std::to_string(actual);
        } else {
            r.passed = false;
            r.detail = "expected " + std::to_string(expected)
                     + " keys but found " + std::to_string(actual);
            ++report.stats.validation_errors;
        }
        report.checks.push_back(r);
    }

    // ── Check 2: VERSION_ORDER ────────────────────────────────────────────────
    {
        ValidationResult r;
        r.check_name = "VERSION_ORDER";
        r.passed     = true;

        for (const auto& key : table.getAllKeys()) {
            auto history = table.getHistory(key);
            for (size_t i = 1; i <static_cast<int>(history.size()); ++i) {
                if (history[i].sys_time.start < history[static_cast<int>(i - 1)].sys_time.start) {
                    r.passed = false;
                    r.detail = "Key '" + key + "': versions out of order at index "
                             + std::to_string(i);
                    ++report.stats.validation_errors;
                    break;
                }
            }
            if (!r.passed) {
              break;
            }
        }
        if (r.passed) {
          r.detail = "all versions in ascending sys_start order";
        }
        report.checks.push_back(r);
    }

    // ── Check 3: NO_OVERLAPPING_VERSIONS ─────────────────────────────────────
    {
        ValidationResult r;
        r.check_name = "NO_OVERLAPPING_VERSIONS";
        r.passed     = true;

        for (const auto& key : table.getAllKeys()) {
            auto history = table.getHistory(key);
            for (size_t i = 1; i <static_cast<int>(history.size()); ++i) {
                if (history[i].sys_time.start < history[static_cast<int>(i - 1)].sys_time.end) {
                    r.passed = false;
                    r.detail = "Key '" + key + "': overlapping versions at index "
                             + std::to_string(i);
                    ++report.stats.validation_errors;
                    break;
                }
            }
            if (!r.passed) {
              break;
            }
        }
        if (r.passed) {
          r.detail = "no overlapping version periods detected";
        }
        report.checks.push_back(r);
    }

    // ── Check 4: CURRENT_VERSION_OPEN ────────────────────────────────────────
    {
        ValidationResult r;
        r.check_name = "CURRENT_VERSION_OPEN";
        r.passed     = true;

        for (const auto& key : table.getAllKeys()) {
            auto current = table.getCurrent(key);
            if (current.has_value() && current->sys_time.end != kMaxTimestamp) {
                r.passed = false;
                r.detail = "Key '" + key + "': current version has a closed sys_end";
                ++report.stats.validation_errors;
                break;
            }
        }
        if (r.passed) {
          r.detail = "all current versions have open sys_time.end";
        }
        report.checks.push_back(r);
    }

    // ── Check 5: HISTORY_CONTINUITY (only when backfill was requested) ────────
    if (last_plan_.has_history_to_backfill) {
        ValidationResult r;
        r.check_name = "HISTORY_CONTINUITY";
        r.passed     = true;
        size_t gaps  = 0;

        for (const auto& key : table.getAllKeys()) {
            auto history = table.getHistory(key);
            // Expect closed version[i].sys_end == next version[i+1].sys_start
            for (size_t i = 0; i + 1 <static_cast<int>(history.size()); ++i) {
                if (!history[i].isCurrent() &&
                    history[i].sys_time.end != history[i + 1].sys_time.start) {
                    ++gaps;
                }
            }
        }

        if (gaps > 0) {
            r.passed = false;
            r.detail = std::to_string(gaps) + " gap(s) detected in backfilled history";
            ++report.stats.validation_errors;
        } else {
            r.detail = "history chain is continuous (no gaps)";
        }
        report.checks.push_back(r);
    }

    report.success = (report.failedCheckCount() == 0);
    last_report_   = report;

    setStatus(report.success ? MigrationStatus::COMPLETE : MigrationStatus::FAILED,
              report.success ? "Verification passed" : "Verification failed");

    return report;
}

} // namespace temporal
} // namespace themisdb
