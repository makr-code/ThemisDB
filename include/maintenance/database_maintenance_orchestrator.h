/**
 * @file database_maintenance_orchestrator.h
 * @brief Central coordinator for all database maintenance operations.
 *
 * The DatabaseMaintenanceOrchestrator is the single point of control for
 * recurring and on-demand database maintenance.  It:
 *
 *   • Stores a set of MaintenanceScheduleEntry objects (full CRUD).
 *   • Registers each enabled schedule as a cron task in the existing
 *     TaskScheduler, so maintenance respects the same scheduling
 *     infrastructure as the rest of the system.
 *   • Executes tasks by delegating to the appropriate module
 *     (IndexMaintenanceManager for index operations, etc.).
 *   • Tracks in-flight OrchestratorJob objects and exposes them to operators.
 *   • Aggregates per-module health signals into a MaintenanceHealthReport.
 *
 * ### Modularity guarantee
 * The orchestrator **never** replaces module logic.  Each module keeps its
 * own maintenance implementation; the orchestrator only drives when and what
 * to run, not how.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 */

#pragma once

#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_health_report.h"
#include "utils/expected.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations – keeps the orchestrator header dependency-light.
class TaskScheduler;
class IndexMaintenanceManager;

namespace utils { class AuditLogger; }

namespace maintenance {

// ---------------------------------------------------------------------------
// Callback type for module health probes
// ---------------------------------------------------------------------------

/**
 * @brief Callable registered by a module to supply its health signal.
 *
 * Called synchronously during getHealthReport().  Must be cheap and
 * non-blocking (< 10 ms).
 */
using HealthProbe = std::function<ModuleHealthSignal()>;

// ---------------------------------------------------------------------------
// DatabaseMaintenanceOrchestrator
// ---------------------------------------------------------------------------

/**
 * @brief Central maintenance coordinator.
 *
 * ### Operator API (HTTP)
 *
 *   Schedule CRUD:
 *     POST   /api/v1/maintenance/schedules         – createSchedule
 *     GET    /api/v1/maintenance/schedules          – listSchedules
 *     GET    /api/v1/maintenance/schedules/{id}     – getSchedule
 *     PUT    /api/v1/maintenance/schedules/{id}     – updateSchedule (full replace)
 *     PATCH  /api/v1/maintenance/schedules/{id}     – patchSchedule (partial update)
 *     DELETE /api/v1/maintenance/schedules/{id}     – deleteSchedule
 *
 *   Jobs & Control:
 *     GET    /api/v1/maintenance/jobs              – listJobs
 *     GET    /api/v1/maintenance/jobs/{id}         – getJob
 *     POST   /api/v1/maintenance/jobs/{id}/cancel  – cancelJob
 *     POST   /api/v1/maintenance/schedules/{id}/run – triggerNow (ad-hoc run)
 *
 *   Observability:
 *     GET    /api/v1/maintenance/status            – getStatus
 *     GET    /api/v1/maintenance/health            – getHealthReport
 */
class DatabaseMaintenanceOrchestrator {
public:
    // ---- Construction & lifecycle -----------------------------------------

    /**
     * @brief Construct the orchestrator.
     *
     * @param scheduler         TaskScheduler used for cron-based scheduling.
     *                          Must outlive this object.
     * @param index_maintenance IndexMaintenanceManager for index operations.
     *                          May be nullptr; index operations will return
     *                          an error in that case.
     * @param audit_logger      Optional audit logger.  May be nullptr.
     */
    explicit DatabaseMaintenanceOrchestrator(
        TaskScheduler*                           scheduler,
        std::shared_ptr<IndexMaintenanceManager> index_maintenance = nullptr,
        std::shared_ptr<utils::AuditLogger>      audit_logger      = nullptr);

    ~DatabaseMaintenanceOrchestrator();

    // Non-copyable, non-movable.
    DatabaseMaintenanceOrchestrator(const DatabaseMaintenanceOrchestrator&) = delete;
    DatabaseMaintenanceOrchestrator& operator=(const DatabaseMaintenanceOrchestrator&) = delete;

    /**
     * @brief Start the orchestrator.
     *
     * Re-registers all enabled schedules in the TaskScheduler and begins
     * accepting ad-hoc job submissions.
     *
     * @return Result<void> – error if the scheduler is not available.
     */
    Result<void> start();

    /**
     * @brief Stop the orchestrator.
     *
     * Deregisters all schedules from the TaskScheduler.  Running jobs are
     * allowed to complete.
     */
    void stop();

    bool isRunning() const noexcept;

    // ---- Schedule CRUD ----------------------------------------------------

    /**
     * @brief Create a new maintenance schedule.
     *
     * Assigns a UUID, timestamps created_at_ms, derives cron_expression from
     * frequency (unless CUSTOM), and registers the task in the TaskScheduler
     * if the orchestrator is running and the schedule is enabled.
     *
     * Validation:
     *   - name must be non-empty.
     *   - tasks must contain at least one entry.
     *   - For CUSTOM frequency, cron_expression must be non-empty.
     *   - window_start_hour and window_end_hour must be in [0, 23].
     *
     * @param entry  Schedule definition.  id, created_at_ms, updated_at_ms,
     *               and runtime fields (last_run_ms, last_job_id, …) are
     *               ignored on input and set by the orchestrator.
     * @return Result with the persisted entry (including assigned id) or error.
     */
    Result<MaintenanceScheduleEntry> createSchedule(MaintenanceScheduleEntry entry);

    /**
     * @brief Retrieve a schedule by ID.
     * @return Result with the entry or error if not found.
     */
    Result<MaintenanceScheduleEntry> getSchedule(const std::string& id) const;

    /**
     * @brief List all schedules (enabled and disabled).
     * @return Vector of all stored schedule entries.
     */
    std::vector<MaintenanceScheduleEntry> listSchedules() const;

    /**
     * @brief Full-replace update of a schedule (PUT semantics).
     *
     * Replaces all mutable fields with the values from @p entry.
     * Read-only fields (created_at_ms, created_by) are preserved.
     * The TaskScheduler registration is updated accordingly.
     *
     * @param id     ID of the schedule to update.
     * @param entry  New schedule definition.  The id field is ignored;
     *               the path parameter @p id is authoritative.
     * @return Result with the updated entry or error if not found / invalid.
     */
    Result<MaintenanceScheduleEntry> updateSchedule(const std::string& id,
                                                    MaintenanceScheduleEntry entry);

    /**
     * @brief Partial update of a schedule (PATCH semantics).
     *
     * Only the JSON fields present in @p patch are applied; absent fields
     * retain their current values.  See MaintenanceScheduleEntry::applyPatch.
     *
     * @param id     ID of the schedule to patch.
     * @param patch  JSON object with only the fields to change.
     * @return Result with the updated entry or error if not found / invalid.
     */
    Result<MaintenanceScheduleEntry> patchSchedule(const std::string& id,
                                                   const nlohmann::json& patch);

    /**
     * @brief Delete a schedule.
     *
     * Deregisters the corresponding task from the TaskScheduler and removes
     * the entry from the registry.  Active jobs spawned by this schedule are
     * allowed to complete.
     *
     * @param id ID of the schedule to delete.
     * @return Result<void> or error if not found.
     */
    Result<void> deleteSchedule(const std::string& id);

    // ---- Job management ---------------------------------------------------

    /**
     * @brief Immediately trigger a schedule regardless of its cron time.
     *
     * Creates an ad-hoc OrchestratorJob and runs the tasks synchronously in
     * a background thread.
     *
     * @param schedule_id  ID of the schedule to run now.
     * @param force        When true, bypass the UTC maintenance window check.
     *                     Requires `maintenance:admin` scope at the API layer.
     *                     The resulting job has `forced=true` and the audit log
     *                     entry carries `"forced": true`.
     * @return Result with the newly created job (in RUNNING state) or error.
     */
    Result<OrchestratorJob> triggerNow(const std::string& schedule_id,
                                       bool force = false);

    /**
     * @brief Cancel a running job.
     *
     * Signals the job's execution thread to abort.  Already-completed tasks
     * within the job are not rolled back.
     *
     * @param job_id ID of the job to cancel.
     * @return Result<void> or error if the job is not found / not running.
     */
    Result<void> cancelJob(const std::string& job_id);

    /**
     * @brief Get the current status of a job.
     * @param job_id Job identifier.
     * @return Result with the OrchestratorJob or error if not found.
     */
    Result<OrchestratorJob> getJob(const std::string& job_id) const;

    /**
     * @brief List all known jobs (active + recently completed).
     *
     * Completed jobs are retained for a configurable TTL (default 24 h).
     *
     * @param active_only  When true, only PENDING and RUNNING jobs are returned.
     */
    std::vector<OrchestratorJob> listJobs(bool active_only = false) const;

    // ---- Observability ----------------------------------------------------

    /**
     * @brief Return a structured status snapshot of the orchestrator.
     *
     * Includes version, running state, schedule counts, active job count, and
     * the time of the most recently completed job.
     */
    nlohmann::json getStatus() const;

    /**
     * @brief Aggregate and return a health report from all registered modules.
     *
     * Calls each registered HealthProbe synchronously, combines the signals,
     * and derives the overall status.
     */
    MaintenanceHealthReport getHealthReport() const;

    // ---- Module integration -----------------------------------------------

    /**
     * @brief Register a health probe for a module.
     *
     * Modules call this once at startup to contribute their health signal to
     * the aggregated report returned by getHealthReport().
     *
     * @param module_name  Human-readable module name (e.g. "index_maintenance").
     * @param probe        Callable returning a ModuleHealthSignal.
     */
    void registerHealthProbe(const std::string& module_name, HealthProbe probe);

private:
    // ---- Internal helpers -------------------------------------------------

    std::string generateId() const;
    int64_t     nowMs() const;

    void registerWithScheduler(const MaintenanceScheduleEntry& entry);
    void deregisterFromScheduler(const std::string& schedule_id);
    std::string schedulerTaskId(const std::string& schedule_id) const;

    void executeSchedule(const std::string& schedule_id, const std::string& job_id,
                         bool force = false);
    void executeTask(MaintenanceTaskType task_type,
                     OrchestratorJob& job);

    void pruneCompletedJobs();

    void validateEntry(const MaintenanceScheduleEntry& entry) const;

    // ---- Members ----------------------------------------------------------
    TaskScheduler*                           scheduler_;
    std::shared_ptr<IndexMaintenanceManager> index_maintenance_;
    std::shared_ptr<utils::AuditLogger>      audit_logger_;

    // Persisted schedules
    mutable std::mutex                                       schedules_mutex_;
    std::map<std::string, MaintenanceScheduleEntry>          schedules_;

    // In-flight and recently completed jobs
    mutable std::mutex                                       jobs_mutex_;
    std::map<std::string, OrchestratorJob>                   jobs_;
    static constexpr int64_t kJobRetentionMs = 24LL * 60 * 60 * 1000; // 24 h

    // Module health probes
    mutable std::mutex                                       probes_mutex_;
    std::map<std::string, HealthProbe>                       health_probes_;

    std::atomic<bool>                                        running_{false};
    mutable std::atomic<uint64_t>                            id_counter_{0};
};

} // namespace maintenance
} // namespace themis
