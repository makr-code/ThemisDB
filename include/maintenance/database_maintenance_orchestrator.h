/**
 * @file database_maintenance_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_health_report.h"
#include "maintenance/i_maintenance_task_handler.h"
#include "maintenance/i_distributed_lock.h"
#include "utils/expected.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <functional>
#include <deque>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations – keeps the orchestrator header dependency-light.
class TaskScheduler;
class IndexMaintenanceManager;
class IStorageEngine;

namespace utils { class AuditLogger; }

namespace maintenance {

// Forward declaration – avoids pulling in storage headers transitively.
class MaintenanceScheduleStore;

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
// TenantMaintenanceConfig
// ---------------------------------------------------------------------------

/**
 * @brief Per-tenant maintenance configuration overrides.
 *
 * When a schedule has a non-empty tenant_id and a TenantMaintenanceConfig is
 * registered for that tenant via setTenantMaintenanceConfig(), the orchestrator
 * applies these overrides instead of (or in addition to) the per-schedule values.
 */
struct TenantMaintenanceConfig {
    /// When true, the tenant-level window [window_start_hour, window_end_hour)
    /// takes precedence over the per-schedule window.
    bool enforce_window       = false;
    int  window_start_hour    = 0;    ///< Inclusive start (0–23, UTC)
    int  window_end_hour      = 23;   ///< Exclusive end (0–23, UTC)

    /// Maximum number of concurrently RUNNING maintenance jobs for this tenant.
    /// 0 means unlimited.
    int  max_concurrent_jobs  = 0;
};

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
 *     GET    /api/v1/maintenance/task-handlers     – listTaskHandlers
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
     * @param storage           Optional storage engine for schedule persistence
     *                          (RocksDB via StorageEngine).  When non-null,
     *                          schedules are written through on every CRUD
     *                          mutation and reloaded on start().  May be nullptr
     *                          for in-memory-only operation (e.g., in tests).
     */
    explicit DatabaseMaintenanceOrchestrator(
        TaskScheduler*                           scheduler,
        std::shared_ptr<IndexMaintenanceManager> index_maintenance = nullptr,
        std::shared_ptr<utils::AuditLogger>      audit_logger      = nullptr,
        IStorageEngine*                          storage           = nullptr);

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
     * @brief List schedules, optionally filtered by tenant.
     *
     * @param tenant_id_filter  When non-empty, only schedules whose tenant_id
     *                          equals this value are returned.  An empty string
     *                          returns all schedules (global + all tenants).
     * @return Vector of matching schedule entries.
     */
    std::vector<MaintenanceScheduleEntry> listSchedules(
        const std::string& tenant_id_filter = "") const;

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

    /**
     * @brief Register or update per-tenant maintenance configuration.
     *
     * Affects all schedules whose tenant_id matches @p tenant_id:
     *   - If config.enforce_window is true, the tenant-level window overrides
     *     the per-schedule window during executeSchedule().
     *   - If config.max_concurrent_jobs > 0, the orchestrator enforces the
     *     quota in executeSchedule() and SKIPs the job when the limit is reached.
     *
     * Call with a default-constructed TenantMaintenanceConfig (enforce_window=false,
     * max_concurrent_jobs=0) to effectively remove constraints for a tenant.
     *
     * @param tenant_id  Tenant identifier.  Must not be empty.
     * @param config     Configuration to apply for this tenant.
     */
    void setTenantMaintenanceConfig(const std::string& tenant_id,
                                    TenantMaintenanceConfig config);

    /**
     * @brief Retrieve the per-tenant maintenance configuration.
     *
     * @param tenant_id  Tenant identifier.
     * @return The registered TenantMaintenanceConfig, or a default-constructed one
     *         (enforce_window=false, max_concurrent_jobs=0) if none is registered.
     */
    TenantMaintenanceConfig getTenantMaintenanceConfig(
        const std::string& tenant_id) const;

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

    /**
     * @brief Inject a distributed lock implementation.
     *
     * When set, the orchestrator calls `dist_lock_->tryAcquire(schedule_id,
     * ttl_ms)` before firing each scheduled job.  Only the cluster node that
     * successfully acquires the lock runs the job; all other nodes log a DEBUG
     * message and mark the job as SKIPPED.
     *
     * The TTL is derived from `MaintenanceScheduleEntry::lock_ttl_ms` when
     * non-zero, otherwise it is computed as the maintenance-window duration
     * plus a 30-second safety margin.
     *
     * This method is thread-safe.  Pass `nullptr` to disable distributed
     * locking (single-node or test deployments).
     *
     * @param lock  Shared pointer to the IDistributedLock implementation,
     *              or nullptr to disable distributed locking.
     */
    void setDistributedLock(std::shared_ptr<IDistributedLock> lock);

    /**
     * @brief Register a task handler for a specific task type.
     *
     * Modules call this to wire real execution logic into the orchestrator for
     * their task type(s).  A previously registered handler for the same type
     * is silently replaced.  Thread-safe.
     *
     * @param task_type  The task type this handler owns.
     * @param handler    Non-null shared pointer to the handler implementation.
     */
    void registerTaskHandler(MaintenanceTaskType task_type,
                             std::shared_ptr<IMaintenanceTaskHandler> handler);

    /**
     * @brief Return a map of registered task handlers.
     *
     * Keys are the string representation of the task type
     * (see taskTypeToString()).  Values are the handler names
     * (IMaintenanceTaskHandler::handlerName()).
     *
     * Used by GET /api/v1/maintenance/task-handlers to let operators
     * diagnose which task types have no handler registered.
     */
    std::map<std::string, std::string> listTaskHandlers() const;

    /**
     * @brief Resolve task execution order from task_dependencies using topological sort.
     *
     * When task_dependencies is empty, returns entry.tasks in positional order.
     * Otherwise performs a stable Kahn's-algorithm topological sort seeded in
     * entry.tasks order, so unrelated tasks preserve their original relative
     * position.  Throws std::invalid_argument if:
     *   - any task_type or depends_on reference is not present in entry.tasks, or
     *   - a cycle is detected.
     */
    static std::vector<MaintenanceTaskType> resolveTaskExecutionOrder(
        const MaintenanceScheduleEntry& entry);

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

    /// Record a DispatchOutcome into the ring buffer (thread-safe).
    void recordDispatchOutcome(DispatchOutcome outcome);

    /// Check and enforce churn rate limit for a schedule.
    /// Returns true if the change is permitted; false if the limit is exceeded.
    bool checkChurnLimit(const std::string& schedule_id, uint32_t max_changes_per_interval);

    // ---- Members ----------------------------------------------------------
    TaskScheduler*                           scheduler_;
    std::shared_ptr<IndexMaintenanceManager> index_maintenance_;
    std::shared_ptr<utils::AuditLogger>      audit_logger_;

    // Optional durable store (nullptr → in-memory only)
    std::unique_ptr<MaintenanceScheduleStore> schedule_store_;

    // Persisted schedules
    mutable std::shared_mutex                                schedules_mutex_;
    std::unordered_map<std::string, MaintenanceScheduleEntry> schedules_;

    // In-flight and recently completed jobs
    mutable std::shared_mutex                                jobs_mutex_;
    std::unordered_map<std::string, OrchestratorJob>         jobs_;
    static constexpr int64_t kJobRetentionMs = 24LL * 60 * 60 * 1000; // 24 h

    // Module health probes
    mutable std::mutex                                       probes_mutex_;
    std::unordered_map<std::string, HealthProbe>             health_probes_;

    // Registered task handlers (keyed by MaintenanceTaskType cast to int)
    mutable std::shared_mutex                                        handlers_mutex_;
    std::unordered_map<int, std::shared_ptr<IMaintenanceTaskHandler>> task_handlers_;

    // Optional distributed lock (nullptr → no distributed coordination)
    mutable std::mutex                                              dist_lock_mutex_;
    std::shared_ptr<IDistributedLock>                               dist_lock_;

    // Per-tenant maintenance configuration overrides
    mutable std::shared_mutex                                     tenant_configs_mutex_;
    std::unordered_map<std::string, TenantMaintenanceConfig>      tenant_configs_;

    // ---- Phase 2: in-flight schedule guard --------------------------------
    /// Schedules currently executing; protects against concurrent re-entry.
    mutable std::mutex                                              in_flight_mutex_;
    std::unordered_set<std::string>                                 in_flight_schedules_;

    // ---- Phase 2: churn rate-limit tracking --------------------------------
    /// Rolling interval in ms for max_schedule_changes_per_interval enforcement.
    static constexpr int64_t kChurnIntervalMs = 60'000LL; // 60 seconds
    /// Protects churn_counts_.
    mutable std::mutex                                              churn_mutex_;
    /// Per-schedule change count and interval start: {count, interval_start_ms}
    std::unordered_map<std::string, std::pair<uint32_t, int64_t>>  churn_counts_;

    // ---- Phase 4: DispatchOutcome ring buffer ------------------------------
    static constexpr int kDefaultRingBufferCapacity = 256;
    mutable std::mutex                   ring_buffer_mutex_;
    std::deque<DispatchOutcome>          dispatch_ring_buffer_;
    int                                  ring_buffer_capacity_{kDefaultRingBufferCapacity};

    std::atomic<bool>                                        running_{false};
    mutable std::atomic<uint64_t>                            id_counter_{0};
};

} // namespace maintenance
} // namespace themis
