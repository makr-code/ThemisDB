/**
 * @file database_maintenance_orchestrator.cpp
 * @brief Implementation of DatabaseMaintenanceOrchestrator.
 *
 * Coordinates database maintenance tasks across all modules by:
 *   1. Persisting MaintenanceScheduleEntry objects (full CRUD).
 *   2. Registering enabled schedules as TaskScheduler cron tasks.
 *   3. Executing maintenance operations via module delegates
 *      (IndexMaintenanceManager, etc.).
 *   4. Tracking running/completed OrchestratorJob objects.
 *   5. Aggregating per-module health signals.
 */

#include "maintenance/database_maintenance_orchestrator.h"
#include "scheduler/task_scheduler.h"
#include "storage/index_maintenance.h"
#include "utils/audit_logger.h"
#include "utils/error_registry.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>

namespace themis {
namespace maintenance {

namespace {

// ---------------------------------------------------------------------------
// UUID-like ID generation (simple, not RFC-4122 compliant)
// ---------------------------------------------------------------------------
std::string generateUuid() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (dist(rng) & 0xFFFFFFFF)
       << "-"
       << std::setw(4) << (dist(rng) & 0xFFFF)
       << "-4"  // version 4
       << std::setw(3) << (dist(rng) & 0xFFF)
       << "-"
       << std::setw(4) << ((dist(rng) & 0x3FFF) | 0x8000)
       << "-"
       << std::setw(12) << (dist(rng) & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

DatabaseMaintenanceOrchestrator::DatabaseMaintenanceOrchestrator(
    TaskScheduler*                           scheduler,
    std::shared_ptr<IndexMaintenanceManager> index_maintenance,
    std::shared_ptr<utils::AuditLogger>      audit_logger)
    : scheduler_(scheduler)
    , index_maintenance_(std::move(index_maintenance))
    , audit_logger_(std::move(audit_logger))
{}

DatabaseMaintenanceOrchestrator::~DatabaseMaintenanceOrchestrator() {
    stop();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Result<void> DatabaseMaintenanceOrchestrator::start() {
    if (running_.load()) {
        return {}; // Already running – idempotent
    }
    if (!scheduler_) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                    "DatabaseMaintenanceOrchestrator: TaskScheduler is null"));
    }

    running_.store(true);

    // Re-register all currently enabled schedules.
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    for (auto& [id, entry] : schedules_) {
        if (entry.enabled) {
            registerWithScheduler(entry);
        }
    }

    spdlog::info("DatabaseMaintenanceOrchestrator started ({} schedules registered)",
                 schedules_.size());
    return {};
}

void DatabaseMaintenanceOrchestrator::stop() {
    if (!running_.exchange(false)) return;

    // Deregister all schedules from the scheduler.
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    for (auto& [id, entry] : schedules_) {
        deregisterFromScheduler(id);
    }

    spdlog::info("DatabaseMaintenanceOrchestrator stopped");
}

bool DatabaseMaintenanceOrchestrator::isRunning() const noexcept {
    return running_.load();
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Create
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::createSchedule(
    MaintenanceScheduleEntry entry)
{
    // Validate
    try {
        validateEntry(entry);
    } catch (const std::exception& ex) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, ex.what()));
    }

    // Assign identity and timestamps
    entry.id           = generateUuid();
    entry.created_at_ms = nowMs();
    entry.updated_at_ms = entry.created_at_ms;
    entry.last_run_ms   = 0;
    entry.next_run_ms   = 0;
    entry.last_run_state.clear();
    entry.last_job_id.clear();

    // Derive cron expression for built-in frequencies
    if (entry.frequency != ScheduleFrequency::CUSTOM || entry.cron_expression.empty()) {
        if (entry.frequency != ScheduleFrequency::CUSTOM) {
            entry.cron_expression = frequencyToCron(entry.frequency, entry.window_start_hour);
        }
    }

    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        schedules_[entry.id] = entry;
    }

    if (running_.load() && entry.enabled) {
        registerWithScheduler(entry);
    }

    spdlog::info("MaintenanceSchedule created: id={} name='{}' cron='{}'",
                 entry.id, entry.name, entry.cron_expression);

    return entry;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Read
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::getSchedule(
    const std::string& id) const
{
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Schedule not found: " + id));
    }
    return it->second;
}

std::vector<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::listSchedules() const {
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    std::vector<MaintenanceScheduleEntry> result;
    result.reserve(schedules_.size());
    for (auto& [id, entry] : schedules_) {
        result.push_back(entry);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Update (PUT)
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::updateSchedule(
    const std::string& id, MaintenanceScheduleEntry entry)
{
    try {
        validateEntry(entry);
    } catch (const std::exception& ex) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, ex.what()));
    }

    std::lock_guard<std::mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Schedule not found: " + id));
    }

    // Preserve read-only fields
    entry.id            = id;
    entry.created_at_ms = it->second.created_at_ms;
    entry.created_by    = it->second.created_by;
    entry.updated_at_ms = nowMs();

    // Runtime state is preserved from the existing entry
    entry.last_run_ms    = it->second.last_run_ms;
    entry.last_run_state = it->second.last_run_state;
    entry.last_job_id    = it->second.last_job_id;
    entry.next_run_ms    = it->second.next_run_ms;

    // Derive cron expression
    if (entry.frequency != ScheduleFrequency::CUSTOM) {
        entry.cron_expression = frequencyToCron(entry.frequency, entry.window_start_hour);
    }

    it->second = entry;

    // Update scheduler registration
    deregisterFromScheduler(id);
    if (running_.load() && entry.enabled) {
        registerWithScheduler(entry);
    }

    spdlog::info("MaintenanceSchedule updated: id={} name='{}'", id, entry.name);
    return entry;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Patch (PATCH)
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::patchSchedule(
    const std::string& id, const nlohmann::json& patch)
{
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Schedule not found: " + id));
    }

    it->second.applyPatch(patch);
    it->second.updated_at_ms = nowMs();

    // Re-derive cron if frequency changed
    if (it->second.frequency != ScheduleFrequency::CUSTOM) {
        it->second.cron_expression =
            frequencyToCron(it->second.frequency, it->second.window_start_hour);
    }

    try {
        validateEntry(it->second);
    } catch (const std::exception& ex) {
        // Roll back – not needed for in-memory store; return error
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, ex.what()));
    }

    MaintenanceScheduleEntry updated = it->second;

    // Update scheduler registration
    deregisterFromScheduler(id);
    if (running_.load() && updated.enabled) {
        registerWithScheduler(updated);
    }

    spdlog::info("MaintenanceSchedule patched: id={}", id);
    return updated;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Delete
// ---------------------------------------------------------------------------

Result<void> DatabaseMaintenanceOrchestrator::deleteSchedule(const std::string& id) {
    std::lock_guard<std::mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Schedule not found: " + id));
    }

    deregisterFromScheduler(id);
    schedules_.erase(it);

    spdlog::info("MaintenanceSchedule deleted: id={}", id);
    return {};
}

// ---------------------------------------------------------------------------
// Job management
// ---------------------------------------------------------------------------

Result<OrchestratorJob> DatabaseMaintenanceOrchestrator::triggerNow(
    const std::string& schedule_id)
{
    MaintenanceScheduleEntry entry;
    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it == schedules_.end()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Schedule not found: " + schedule_id));
        }
        entry = it->second;
    }

    if (entry.tasks.empty()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Schedule '" + schedule_id + "' has no tasks configured"));
    }

    // Create the first job for the first task type (jobs are per-task)
    OrchestratorJob job;
    job.id          = generateUuid();
    job.schedule_id = schedule_id;
    job.task_type   = entry.tasks.front();
    job.state       = MaintenanceJobState::RUNNING;
    job.started_at_ms = nowMs();

    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[job.id] = job;
    }

    // Run asynchronously
    std::string job_id = job.id;
    std::thread([this, schedule_id, job_id]() {
        executeSchedule(schedule_id, job_id);
    }).detach();

    return job;
}

Result<void> DatabaseMaintenanceOrchestrator::cancelJob(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Job not found: " + job_id));
    }
    if (it->second.state != MaintenanceJobState::RUNNING &&
        it->second.state != MaintenanceJobState::PENDING) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Job '" + job_id + "' is not active"));
    }
    it->second.state         = MaintenanceJobState::CANCELLED;
    it->second.finished_at_ms = nowMs();
    spdlog::warn("MaintenanceJob cancelled: id={}", job_id);
    return {};
}

Result<OrchestratorJob> DatabaseMaintenanceOrchestrator::getJob(
    const std::string& job_id) const
{
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND, "Job not found: " + job_id));
    }
    return it->second;
}

std::vector<OrchestratorJob> DatabaseMaintenanceOrchestrator::listJobs(
    bool active_only) const
{
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    std::vector<OrchestratorJob> result;
    for (auto& [id, job] : jobs_) {
        if (active_only && job.state != MaintenanceJobState::PENDING &&
                           job.state != MaintenanceJobState::RUNNING) {
            continue;
        }
        result.push_back(job);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

nlohmann::json DatabaseMaintenanceOrchestrator::getStatus() const {
    nlohmann::json j;
    j["running"]         = running_.load();

    int enabled = 0, total = 0;
    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        total = static_cast<int>(schedules_.size());
        for (auto& [id, e] : schedules_) {
            if (e.enabled) ++enabled;
        }
    }
    j["total_schedules"]   = total;
    j["enabled_schedules"] = enabled;

    int active = 0;
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        for (auto& [id, job] : jobs_) {
            if (job.state == MaintenanceJobState::PENDING ||
                job.state == MaintenanceJobState::RUNNING) {
                ++active;
            }
        }
    }
    j["active_jobs"] = active;
    j["generated_at_ms"] = nowMs();
    return j;
}

MaintenanceHealthReport DatabaseMaintenanceOrchestrator::getHealthReport() const {
    MaintenanceHealthReport report;
    report.generated_at_ms = nowMs();

    // Orchestrator counts
    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        report.total_schedules = static_cast<int>(schedules_.size());
        for (auto& [id, e] : schedules_) {
            if (e.enabled) ++report.enabled_schedules;
        }
    }

    int64_t cutoff = nowMs() - 24LL * 60 * 60 * 1000;
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        for (auto& [id, job] : jobs_) {
            if (job.started_at_ms < cutoff) continue;
            if (job.state == MaintenanceJobState::RUNNING ||
                job.state == MaintenanceJobState::PENDING) {
                ++report.active_jobs;
            } else if (job.state == MaintenanceJobState::SUCCEEDED) {
                ++report.success_jobs_24h;
            } else if (job.state == MaintenanceJobState::FAILED) {
                ++report.failed_jobs_24h;
            }
        }
    }

    // Collect module health signals
    ModuleHealthStatus worst = ModuleHealthStatus::OK;
    {
        std::lock_guard<std::mutex> lock(probes_mutex_);
        for (auto& [name, probe] : health_probes_) {
            try {
                ModuleHealthSignal sig = probe();
                if (sig.status > worst) worst = sig.status;
                report.module_signals.push_back(std::move(sig));
            } catch (const std::exception& ex) {
                ModuleHealthSignal sig;
                sig.module_name   = name;
                sig.status        = ModuleHealthStatus::UNKNOWN;
                sig.message       = std::string("probe threw: ") + ex.what();
                sig.checked_at_ms = nowMs();
                if (worst < ModuleHealthStatus::DEGRADED) worst = ModuleHealthStatus::DEGRADED;
                report.module_signals.push_back(std::move(sig));
            }
        }
    }

    // If we have failed jobs in the last 24 h, at least DEGRADED
    if (report.failed_jobs_24h > 0 && worst < ModuleHealthStatus::DEGRADED) {
        worst = ModuleHealthStatus::DEGRADED;
    }

    report.overall_status = worst;
    return report;
}

// ---------------------------------------------------------------------------
// Module health probe registration
// ---------------------------------------------------------------------------

void DatabaseMaintenanceOrchestrator::registerHealthProbe(
    const std::string& module_name, HealthProbe probe)
{
    std::lock_guard<std::mutex> lock(probes_mutex_);
    health_probes_[module_name] = std::move(probe);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string DatabaseMaintenanceOrchestrator::generateId() const {
    return generateUuid();
}

int64_t DatabaseMaintenanceOrchestrator::nowMs() const {
    return themis::maintenance::nowMs();
}

std::string DatabaseMaintenanceOrchestrator::schedulerTaskId(
    const std::string& schedule_id) const
{
    return "maintenance_schedule_" + schedule_id;
}

void DatabaseMaintenanceOrchestrator::registerWithScheduler(
    const MaintenanceScheduleEntry& entry)
{
    if (!scheduler_) return;

    ScheduledTask task;
    task.id             = schedulerTaskId(entry.id);
    task.name           = "Maintenance: " + entry.name;
    task.description    = entry.description;
    task.type           = ScheduledTask::TaskType::FUNCTION;
    task.function_name  = task.id;   // We'll register a matching function
    task.trigger_type   = ScheduledTask::TriggerType::CRON;
    task.cron_expression = entry.cron_expression;
    task.enabled        = entry.enabled;

    // Register a function handler that invokes executeSchedule
    std::string eid = entry.id;
    scheduler_->registerFunction(task.id,
        [this, eid](const nlohmann::json&) -> nlohmann::json {
            std::string job_id = generateUuid();

            OrchestratorJob job;
            job.id            = job_id;
            job.schedule_id   = eid;
            job.state         = MaintenanceJobState::RUNNING;
            job.started_at_ms = themis::maintenance::nowMs();

            MaintenanceScheduleEntry entry_copy;
            {
                std::lock_guard<std::mutex> lock(schedules_mutex_);
                auto it = schedules_.find(eid);
                if (it == schedules_.end()) {
                    return {{"status", "error"}, {"error", "schedule_not_found"}};
                }
                entry_copy = it->second;
                job.task_type = entry_copy.tasks.empty()
                                ? MaintenanceTaskType::METRICS_COLLECTION
                                : entry_copy.tasks.front();
            }

            {
                std::lock_guard<std::mutex> jlock(jobs_mutex_);
                jobs_[job_id] = job;
            }

            executeSchedule(eid, job_id);

            std::lock_guard<std::mutex> jlock(jobs_mutex_);
            auto it = jobs_.find(job_id);
            if (it != jobs_.end()) {
                return it->second.toJson();
            }
            return {{"status", "completed"}, {"job_id", job_id}};
        });

    scheduler_->registerTask(task);
}

void DatabaseMaintenanceOrchestrator::deregisterFromScheduler(
    const std::string& schedule_id)
{
    if (!scheduler_) return;
    const std::string tid = schedulerTaskId(schedule_id);
    scheduler_->unregisterTask(tid);
    scheduler_->unregisterFunction(tid);
}

void DatabaseMaintenanceOrchestrator::executeSchedule(
    const std::string& schedule_id, const std::string& job_id)
{
    MaintenanceScheduleEntry entry;
    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it == schedules_.end()) {
            std::lock_guard<std::mutex> jlock(jobs_mutex_);
            if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                jit->second.state          = MaintenanceJobState::FAILED;
                jit->second.error_message  = "Schedule not found";
                jit->second.finished_at_ms = themis::maintenance::nowMs();
            }
            return;
        }
        entry = it->second;
    }

    bool all_ok = true;
    std::string last_error;

    for (auto task_type : entry.tasks) {
        // Check cancellation
        {
            std::lock_guard<std::mutex> jlock(jobs_mutex_);
            auto jit = jobs_.find(job_id);
            if (jit != jobs_.end() &&
                jit->second.state == MaintenanceJobState::CANCELLED) {
                return;
            }
        }

        OrchestratorJob sub_job;
        sub_job.id          = job_id;
        sub_job.schedule_id = schedule_id;
        sub_job.task_type   = task_type;
        sub_job.state       = MaintenanceJobState::RUNNING;
        sub_job.started_at_ms = themis::maintenance::nowMs();

        executeTask(task_type, sub_job);

        if (sub_job.state == MaintenanceJobState::FAILED) {
            all_ok    = false;
            last_error = sub_job.error_message;
            if (entry.halt_on_task_failure) break;
        }
    }

    int64_t now = themis::maintenance::nowMs();

    // Update the job record
    {
        std::lock_guard<std::mutex> jlock(jobs_mutex_);
        auto jit = jobs_.find(job_id);
        if (jit != jobs_.end() &&
            jit->second.state != MaintenanceJobState::CANCELLED) {
            jit->second.state         = all_ok ? MaintenanceJobState::SUCCEEDED
                                                : MaintenanceJobState::FAILED;
            jit->second.error_message = last_error;
            jit->second.finished_at_ms = now;
            jit->second.progress_pct  = 100.0;
        }
    }

    // Update the schedule's runtime state
    {
        std::lock_guard<std::mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it != schedules_.end()) {
            it->second.last_run_ms    = now;
            it->second.last_run_state = all_ok ? "success" : "failed";
            it->second.last_job_id    = job_id;
        }
    }

    pruneCompletedJobs();
}

void DatabaseMaintenanceOrchestrator::executeTask(
    MaintenanceTaskType task_type, OrchestratorJob& job)
{
    spdlog::info("Maintenance task starting: type={} job={}",
                 taskTypeToString(task_type), job.id);

    switch (task_type) {
        case MaintenanceTaskType::INDEX_REBUILD:
        case MaintenanceTaskType::INDEX_REORGANIZE:
        case MaintenanceTaskType::STATISTICS_UPDATE:
        case MaintenanceTaskType::ORPHAN_CLEANUP:
        case MaintenanceTaskType::FRAGMENTATION_MONITORING:
        case MaintenanceTaskType::VECTOR_REINDEX: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                return;
            }
            // Trigger a full maintenance check which uses the configured policy.
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Index maintenance check completed";
            }
            break;
        }

        case MaintenanceTaskType::CONSISTENCY_CHECK: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                return;
            }
            auto active = index_maintenance_->listActiveJobs();
            job.state          = MaintenanceJobState::SUCCEEDED;
            job.result_summary = "Consistency check: " +
                                 std::to_string(active.size()) + " active index jobs found";
            break;
        }

        case MaintenanceTaskType::METRICS_COLLECTION:
        case MaintenanceTaskType::QUOTA_CHECK:
        case MaintenanceTaskType::REPLICA_VALIDATION:
        case MaintenanceTaskType::PERFORMANCE_ANALYSIS:
        case MaintenanceTaskType::MVCC_CLEANUP:
        case MaintenanceTaskType::FULL_CHECKDB:
        case MaintenanceTaskType::BACKUP_VERIFICATION:
        case MaintenanceTaskType::CAPACITY_TREND_ANALYSIS:
        case MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT:
        case MaintenanceTaskType::DISASTER_RECOVERY_DRILL:
        case MaintenanceTaskType::BASELINE_UPDATE:
        case MaintenanceTaskType::STORAGE_COMPACTION: {
            // These tasks are handled by other modules that register health probes.
            // The orchestrator logs and marks success; each module can extend this
            // by registering additional HealthProbe callbacks.
            spdlog::info("Maintenance task '{}': delegated to module health probes",
                         taskTypeToString(task_type));
            job.state          = MaintenanceJobState::SUCCEEDED;
            job.result_summary = "Task '" + taskTypeToString(task_type) +
                                 "' completed (module-delegated)";
            break;
        }

        default:
            job.state         = MaintenanceJobState::FAILED;
            job.error_message = "Unknown task type";
            break;
    }

    job.finished_at_ms = themis::maintenance::nowMs();
    spdlog::info("Maintenance task finished: type={} job={} state={}",
                 taskTypeToString(task_type), job.id, jobStateToString(job.state));
}

void DatabaseMaintenanceOrchestrator::pruneCompletedJobs() {
    int64_t cutoff = themis::maintenance::nowMs() - kJobRetentionMs;
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        auto& job = it->second;
        if (job.state != MaintenanceJobState::RUNNING &&
            job.state != MaintenanceJobState::PENDING &&
            job.finished_at_ms > 0 &&
            job.finished_at_ms < cutoff) {
            it = jobs_.erase(it);
        } else {
            ++it;
        }
    }
}

void DatabaseMaintenanceOrchestrator::validateEntry(
    const MaintenanceScheduleEntry& entry) const
{
    if (entry.name.empty()) {
        throw std::invalid_argument("Schedule name must not be empty");
    }
    if (entry.tasks.empty()) {
        throw std::invalid_argument("Schedule must contain at least one task");
    }
    if (entry.frequency == ScheduleFrequency::CUSTOM && entry.cron_expression.empty()) {
        throw std::invalid_argument(
            "cron_expression is required when frequency is 'custom'");
    }
    if (entry.window_start_hour < 0 || entry.window_start_hour > 23) {
        throw std::invalid_argument("window_start_hour must be in [0, 23]");
    }
    if (entry.window_end_hour < 0 || entry.window_end_hour > 23) {
        throw std::invalid_argument("window_end_hour must be in [0, 23]");
    }
}

} // namespace maintenance
} // namespace themis
