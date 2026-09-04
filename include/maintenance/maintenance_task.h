/**
 * @file maintenance_task.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// Operation types
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of all maintenance operations the orchestrator can run.
 */
enum class MaintenanceTaskType {
    // --- Daily ---
    METRICS_COLLECTION,        ///< Collect Prometheus metrics snapshots
    FRAGMENTATION_MONITORING,  ///< Scan index fragmentation levels
    QUOTA_CHECK,               ///< Validate storage quota thresholds

    // --- Weekly ---
    CONSISTENCY_CHECK,         ///< Run integrity / consistency validation
    REPLICA_VALIDATION,        ///< Verify replica lag and consistency
    PERFORMANCE_ANALYSIS,      ///< Slow-query / lock-contention analysis
    MVCC_CLEANUP,              ///< Garbage-collect stale MVCC versions

    // --- Monthly ---
    FULL_CHECKDB,              ///< Full CHECKDB-equivalent database check
    BACKUP_VERIFICATION,       ///< Verify backup integrity
    CAPACITY_TREND_ANALYSIS,   ///< Analyse storage growth trends
    INDEX_FRAGMENTATION_REPORT,///< Produce full index fragmentation report

    // --- Quarterly ---
    DISASTER_RECOVERY_DRILL,   ///< Simulate DR restore procedure
    BASELINE_UPDATE,           ///< Refresh performance baselines

    // --- On-demand ---
    INDEX_REBUILD,             ///< Trigger index rebuild
    INDEX_REORGANIZE,          ///< Trigger index reorganization
    STATISTICS_UPDATE,         ///< Refresh query-planner statistics
    STORAGE_COMPACTION,        ///< Trigger RocksDB compaction
    ORPHAN_CLEANUP,            ///< Remove orphaned index entries
    VECTOR_REINDEX,            ///< Incremental HNSW re-index
};

/// Convert a MaintenanceTaskType to its string representation.
inline std::string taskTypeToString(MaintenanceTaskType t) {
    switch (t) {
        case MaintenanceTaskType::METRICS_COLLECTION:        return "metrics_collection";
        case MaintenanceTaskType::FRAGMENTATION_MONITORING:  return "fragmentation_monitoring";
        case MaintenanceTaskType::QUOTA_CHECK:               return "quota_check";
        case MaintenanceTaskType::CONSISTENCY_CHECK:         return "consistency_check";
        case MaintenanceTaskType::REPLICA_VALIDATION:        return "replica_validation";
        case MaintenanceTaskType::PERFORMANCE_ANALYSIS:      return "performance_analysis";
        case MaintenanceTaskType::MVCC_CLEANUP:              return "mvcc_cleanup";
        case MaintenanceTaskType::FULL_CHECKDB:              return "full_checkdb";
        case MaintenanceTaskType::BACKUP_VERIFICATION:       return "backup_verification";
        case MaintenanceTaskType::CAPACITY_TREND_ANALYSIS:   return "capacity_trend_analysis";
        case MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT:return "index_fragmentation_report";
        case MaintenanceTaskType::DISASTER_RECOVERY_DRILL:   return "disaster_recovery_drill";
        case MaintenanceTaskType::BASELINE_UPDATE:           return "baseline_update";
        case MaintenanceTaskType::INDEX_REBUILD:             return "index_rebuild";
        case MaintenanceTaskType::INDEX_REORGANIZE:          return "index_reorganize";
        case MaintenanceTaskType::STATISTICS_UPDATE:         return "statistics_update";
        case MaintenanceTaskType::STORAGE_COMPACTION:        return "storage_compaction";
        case MaintenanceTaskType::ORPHAN_CLEANUP:            return "orphan_cleanup";
        case MaintenanceTaskType::VECTOR_REINDEX:            return "vector_reindex";
        default:                                             return "unknown";
    }
}

/// Parse a MaintenanceTaskType from its string representation.
/// Returns MaintenanceTaskType::METRICS_COLLECTION if not recognized.
inline MaintenanceTaskType taskTypeFromString(const std::string& s) {
    if (s == "metrics_collection") {
      return MaintenanceTaskType::METRICS_COLLECTION;
    }
    if (s == "fragmentation_monitoring") {
      return MaintenanceTaskType::FRAGMENTATION_MONITORING;
    }
    if (s == "quota_check") {
      return MaintenanceTaskType::QUOTA_CHECK;
    }
    if (s == "consistency_check") {
      return MaintenanceTaskType::CONSISTENCY_CHECK;
    }
    if (s == "replica_validation") {
      return MaintenanceTaskType::REPLICA_VALIDATION;
    }
    if (s == "performance_analysis") {
      return MaintenanceTaskType::PERFORMANCE_ANALYSIS;
    }
    if (s == "mvcc_cleanup") {
      return MaintenanceTaskType::MVCC_CLEANUP;
    }
    if (s == "full_checkdb") {
      return MaintenanceTaskType::FULL_CHECKDB;
    }
    if (s == "backup_verification") {
      return MaintenanceTaskType::BACKUP_VERIFICATION;
    }
    if (s == "capacity_trend_analysis") {
      return MaintenanceTaskType::CAPACITY_TREND_ANALYSIS;
    }
    if (s == "index_fragmentation_report") {
      return MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT;
    }
    if (s == "disaster_recovery_drill") {
      return MaintenanceTaskType::DISASTER_RECOVERY_DRILL;
    }
    if (s == "baseline_update") {
      return MaintenanceTaskType::BASELINE_UPDATE;
    }
    if (s == "index_rebuild") {
      return MaintenanceTaskType::INDEX_REBUILD;
    }
    if (s == "index_reorganize") {
      return MaintenanceTaskType::INDEX_REORGANIZE;
    }
    if (s == "statistics_update") {
      return MaintenanceTaskType::STATISTICS_UPDATE;
    }
    if (s == "storage_compaction") {
      return MaintenanceTaskType::STORAGE_COMPACTION;
    }
    if (s == "orphan_cleanup") {
      return MaintenanceTaskType::ORPHAN_CLEANUP;
    }
    if (s == "vector_reindex") {
      return MaintenanceTaskType::VECTOR_REINDEX;
    }
    return MaintenanceTaskType::METRICS_COLLECTION;
}

// ---------------------------------------------------------------------------
// Job status
// ---------------------------------------------------------------------------

/**
 * @brief Runtime state of a maintenance job triggered by the orchestrator.
 */
enum class MaintenanceJobState {
    PENDING,   ///< Scheduled but not yet started
    RUNNING,   ///< Currently executing
    SUCCEEDED, ///< Completed successfully
    FAILED,    ///< Completed with errors
    CANCELLED, ///< Cancelled by operator
    SKIPPED,   ///< Skipped (e.g., outside maintenance window)
};

inline std::string jobStateToString(MaintenanceJobState s) {
    switch (s) {
        case MaintenanceJobState::PENDING:   return "pending";
        case MaintenanceJobState::RUNNING:   return "running";
        case MaintenanceJobState::SUCCEEDED: return "succeeded";
        case MaintenanceJobState::FAILED:    return "failed";
        case MaintenanceJobState::CANCELLED: return "cancelled";
        case MaintenanceJobState::SKIPPED:   return "skipped";
        default:                             return "unknown";
    }
}

/**
 * @brief Represents a single running or completed orchestrator job.
 */
struct OrchestratorJob {
    std::string          id;           ///< Unique job identifier (UUID)
    std::string          schedule_id;  ///< Parent schedule that spawned this job ("" = ad-hoc)
    std::string          tenant_id;    ///< Tenant that owns this job (from schedule; "" = global)
    MaintenanceTaskType  task_type;    ///< Which operation is being performed
    MaintenanceJobState  state = MaintenanceJobState::PENDING;
    std::string          error_message;
    std::string          result_summary;
    double               progress_pct = 0.0;
    int64_t              started_at_ms = 0;
    int64_t              finished_at_ms = 0;
    bool                 forced = false; ///< true when the job was triggered with force=true (bypassed window)

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["id"]             = id;
        j["schedule_id"]    = schedule_id;
        j["tenant_id"]      = tenant_id;
        j["task_type"]      = taskTypeToString(task_type);
        j["state"]          = jobStateToString(state);
        j["error_message"]  = error_message;
        j["result_summary"] = result_summary;
        j["progress_pct"]   = progress_pct;
        j["started_at_ms"]  = started_at_ms;
        j["finished_at_ms"] = finished_at_ms;
        j["forced"]         = forced;
        if (started_at_ms > 0 && finished_at_ms > 0) {
            j["duration_ms"] = finished_at_ms - started_at_ms;
        }
        return j;
    }
};

} // namespace maintenance
} // namespace themis
