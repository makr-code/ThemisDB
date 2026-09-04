/**
 * @file maintenance_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_health_report.h"
#include "storage/index_maintenance.h"

#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// Default schedule bundles
// ---------------------------------------------------------------------------

/**
 * @brief Returns the recommended DAILY maintenance schedule.
 *
 * Covers: metrics collection, fragmentation monitoring, quota checks.
 */
MaintenanceScheduleEntry defaultDailySchedule() {
    MaintenanceScheduleEntry e;
    e.name        = "Daily Maintenance";
    e.description = "Daily: metrics collection, fragmentation monitoring, quota checks";
    e.frequency   = ScheduleFrequency::DAILY;
    e.tasks       = {
        MaintenanceTaskType::METRICS_COLLECTION,
        MaintenanceTaskType::FRAGMENTATION_MONITORING,
        MaintenanceTaskType::QUOTA_CHECK,
    };
    e.window_start_hour    = 2;
    e.window_end_hour      = 6;
    e.enforce_window       = true;
    e.halt_on_task_failure = false;
    return e;
}

/**
 * @brief Returns the recommended WEEKLY maintenance schedule.
 *
 * Covers: consistency checks, replica validation, performance analysis,
 *         MVCC cleanup.
 */
MaintenanceScheduleEntry defaultWeeklySchedule() {
    MaintenanceScheduleEntry e;
    e.name        = "Weekly Maintenance";
    e.description = "Weekly: consistency checks, replica validation, "
                    "performance analysis, MVCC cleanup";
    e.frequency   = ScheduleFrequency::WEEKLY;
    e.tasks       = {
        MaintenanceTaskType::CONSISTENCY_CHECK,
        MaintenanceTaskType::REPLICA_VALIDATION,
        MaintenanceTaskType::PERFORMANCE_ANALYSIS,
        MaintenanceTaskType::MVCC_CLEANUP,
    };
    e.window_start_hour    = 2;
    e.window_end_hour      = 6;
    e.enforce_window       = true;
    e.halt_on_task_failure = false;
    return e;
}

/**
 * @brief Returns the recommended MONTHLY maintenance schedule.
 *
 * Covers: full CHECKDB, backup verification, capacity trend analysis,
 *         index fragmentation report.
 */
MaintenanceScheduleEntry defaultMonthlySchedule() {
    MaintenanceScheduleEntry e;
    e.name        = "Monthly Maintenance";
    e.description = "Monthly: full CHECKDB, backup verification, "
                    "capacity trend analysis, index fragmentation report";
    e.frequency   = ScheduleFrequency::MONTHLY;
    e.tasks       = {
        MaintenanceTaskType::FULL_CHECKDB,
        MaintenanceTaskType::BACKUP_VERIFICATION,
        MaintenanceTaskType::CAPACITY_TREND_ANALYSIS,
        MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT,
    };
    e.window_start_hour    = 1;
    e.window_end_hour      = 6;
    e.enforce_window       = true;
    e.halt_on_task_failure = false;
    return e;
}

/**
 * @brief Returns the recommended QUARTERLY maintenance schedule.
 *
 * Covers: disaster recovery drill, performance baseline update.
 */
MaintenanceScheduleEntry defaultQuarterlySchedule() {
    MaintenanceScheduleEntry e;
    e.name        = "Quarterly Maintenance";
    e.description = "Quarterly: disaster recovery drill, performance baseline update";
    e.frequency   = ScheduleFrequency::QUARTERLY;
    e.tasks       = {
        MaintenanceTaskType::DISASTER_RECOVERY_DRILL,
        MaintenanceTaskType::BASELINE_UPDATE,
    };
    e.window_start_hour    = 0;
    e.window_end_hour      = 6;
    e.enforce_window       = true;
    e.halt_on_task_failure = true;  // DR drill must not be skipped on prior failure
    return e;
}

// ---------------------------------------------------------------------------
// Module health probe factories
// ---------------------------------------------------------------------------

/**
 * @brief Create a HealthProbe for the IndexMaintenanceManager.
 *
 * @param mgr  Shared pointer to the IndexMaintenanceManager.
 * @return     HealthProbe callable suitable for registerHealthProbe().
 */
HealthProbe makeIndexMaintenanceHealthProbe(
    std::shared_ptr<IndexMaintenanceManager> mgr)
{
    return [mgr]() -> ModuleHealthSignal {
        ModuleHealthSignal sig;
        sig.module_name   = "index_maintenance";
        sig.checked_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (!mgr) {
            sig.status  = ModuleHealthStatus::UNKNOWN;
            sig.message = "IndexMaintenanceManager not available";
            return sig;
        }

        if (!mgr->isRunning()) {
            sig.status  = ModuleHealthStatus::DEGRADED;
            sig.message = "IndexMaintenanceManager is not running";
            return sig;
        }

        auto active_jobs = mgr->listActiveJobs();
        sig.details["active_jobs"] = std::to_string(active_jobs.size());

        // Check for failed jobs
        int failed = 0;
        for (auto& job : active_jobs) {
            if (job.is_failed) {
              ++failed;
            }
        }
        sig.details["failed_jobs"] = std::to_string(failed);

        if (failed > 0) {
            sig.status  = ModuleHealthStatus::DEGRADED;
            sig.message = std::to_string(failed) + " index maintenance job(s) failed";
        } else {
            sig.status  = ModuleHealthStatus::OK;
            sig.message = "Index maintenance healthy; " +
                          std::to_string(active_jobs.size()) + " active job(s)";
        }
        return sig;
    };
}

/**
 * @brief Register all default schedules and the IndexMaintenance health probe
 *        with the given orchestrator.
 *
 * Call this once during server start-up after constructing the orchestrator.
 *
 * @param orchestrator  The DatabaseMaintenanceOrchestrator to configure.
 * @param index_mgr     Optional IndexMaintenanceManager (may be nullptr).
 */
void registerDefaultMaintenanceSetup(
    DatabaseMaintenanceOrchestrator& orchestrator,
    std::shared_ptr<IndexMaintenanceManager> index_mgr)
{
    // Register default schedules (disabled by default so operators can enable)
    auto daily = defaultDailySchedule();
    daily.enabled = false;
    (void)orchestrator.createSchedule(daily);

    auto weekly = defaultWeeklySchedule();
    weekly.enabled = false;
    (void)orchestrator.createSchedule(weekly);

    auto monthly = defaultMonthlySchedule();
    monthly.enabled = false;
    (void)orchestrator.createSchedule(monthly);

    auto quarterly = defaultQuarterlySchedule();
    quarterly.enabled = false;
    (void)orchestrator.createSchedule(quarterly);

    // Register index maintenance health probe
    if (index_mgr) {
        orchestrator.registerHealthProbe(
            "index_maintenance",
            makeIndexMaintenanceHealthProbe(index_mgr));
    }

    spdlog::info("DatabaseMaintenanceOrchestrator: default schedules registered "
                 "(all disabled – enable via PATCH /api/v1/maintenance/schedules/<id>)");
}

} // namespace maintenance
} // namespace themis

