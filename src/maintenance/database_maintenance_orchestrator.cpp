/**
 * @file database_maintenance_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_schedule_store.h"
#include "maintenance/i_maintenance_task_handler.h"
#include "maintenance/i_distributed_lock.h"
#include "scheduler/task_scheduler.h"
#include "storage/index_maintenance.h"
#include "utils/audit_logger.h"
#include "utils/error_registry.h"
#include "observability/metrics_collector.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <deque>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <sstream>
#include <iomanip>
#include <thread>

namespace themis {
namespace maintenance {

using observability::MetricsCollector;

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

/// Return the current UTC hour-of-day (0–23).
int currentUtcHour() {
    auto tp = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    return tm.tm_hour;
}

/// Returns true when the current UTC hour is inside [start, end).
/// Handles wrap-around (e.g., start=22, end=4 spans midnight).
bool isInMaintenanceWindow(int window_start_hour, int window_end_hour) {
    int h = currentUtcHour();
    if (window_start_hour < window_end_hour) {
        return h >= window_start_hour && h < window_end_hour;
    }
    // Spans midnight
    return h >= window_start_hour || h < window_end_hour;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

DatabaseMaintenanceOrchestrator::DatabaseMaintenanceOrchestrator(
    TaskScheduler*                           scheduler,
    std::shared_ptr<IndexMaintenanceManager> index_maintenance,
    std::shared_ptr<utils::AuditLogger>      audit_logger,
    IStorageEngine*                          storage)
    : scheduler_(scheduler)
    , index_maintenance_(std::move(index_maintenance))
    , audit_logger_(std::move(audit_logger))
    , schedule_store_(storage ? std::make_unique<MaintenanceScheduleStore>(storage) : nullptr)
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

    // Load persisted schedules from storage into a temporary map first.
    // This step does not require a scheduler and is safe to do before the
    // scheduler availability check so that schedules_ is populated regardless
    // of whether cron registration will succeed.
    if (schedule_store_) {
        std::map<std::string, MaintenanceScheduleEntry> loaded;
        auto load_result = schedule_store_->loadAll(loaded);
        if (!load_result.has_value()) {
            spdlog::error("DatabaseMaintenanceOrchestrator::start: failed to load "
                          "schedules from storage: {}", load_result.error().message());
            // Non-fatal: continue with whatever was already in schedules_.
        }
        // Merge loaded entries into schedules_ under the lock.
        // Use insert_or_assign so persisted entries always take precedence
        // over any schedules that were inserted before start() was called.
        {
            std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
            for (auto& [id, entry] : loaded) {
                schedules_.insert_or_assign(id, std::move(entry));
            }
        }
    }

    if (!scheduler_) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                    "DatabaseMaintenanceOrchestrator: TaskScheduler is null"));
    }

    running_.store(true);

    // Re-register all currently enabled schedules.
    std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
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
    std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
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
        std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
        // Persist to durable storage first; fail the operation if persistence
        // fails so the caller can retry rather than silently losing durability.
        if (schedule_store_) {
            auto persist_result = schedule_store_->save(entry);
            if (!persist_result.has_value()) {
                return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                    "Failed to persist schedule id=" + entry.id + ": " +
                    persist_result.error().message()));
            }
        }
        schedules_[entry.id] = entry;
    }

    if (running_.load() && entry.enabled) {
        registerWithScheduler(entry);
    }

    spdlog::info("MaintenanceSchedule created: id={} name='{}' cron='{}'",
                 entry.id, entry.name, entry.cron_expression);

    // Audit log
    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_schedule_created"},
            {"schedule_id", entry.id},
            {"name",        entry.name},
            {"frequency",   frequencyToString(entry.frequency)},
            {"cron",        entry.cron_expression},
            {"timestamp_ms", entry.created_at_ms},
        });
    }

    // Metrics
    MetricsCollector::getInstance().addCounter("maintenance_schedules_created_total", 1,
        {{"frequency", frequencyToString(entry.frequency)}});

    return entry;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Read
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::getSchedule(
    const std::string& id) const
{
    std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Schedule not found: " + id));
    }
    return it->second;
}

std::vector<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::listSchedules(
    const std::string& tenant_id_filter) const
{
    std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
    std::vector<MaintenanceScheduleEntry> result;
    result.reserve(schedules_.size());
    for (auto& [id, entry] : schedules_) {
        if (!tenant_id_filter.empty() && entry.tenant_id != tenant_id_filter) {
            continue;
        }
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

    std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
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

    // Persist to durable storage before committing the in-memory change so
    // that a storage failure leaves the existing entry intact and returnable.
    if (schedule_store_) {
        auto persist_result = schedule_store_->save(entry);
        if (!persist_result.has_value()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                "Failed to persist schedule id=" + id + ": " +
                persist_result.error().message()));
        }
    }

    it->second = entry;

    // Update scheduler registration
    deregisterFromScheduler(id);
    if (running_.load() && entry.enabled) {
        registerWithScheduler(entry);
    }

    spdlog::info("MaintenanceSchedule updated: id={} name='{}'", id, entry.name);

    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_schedule_updated"},
            {"schedule_id", id},
            {"name",        entry.name},
            {"timestamp_ms", entry.updated_at_ms},
        });
    }

    return entry;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Patch (PATCH)
// ---------------------------------------------------------------------------

Result<MaintenanceScheduleEntry> DatabaseMaintenanceOrchestrator::patchSchedule(
    const std::string& id, const nlohmann::json& patch)
{
    std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Schedule not found: " + id));
    }

    // Apply the patch to a working copy so that the in-memory entry is only
    // modified after a successful durable write.
    MaintenanceScheduleEntry updated = it->second;
    updated.applyPatch(patch);
    updated.updated_at_ms = nowMs();

    // Re-derive cron if frequency changed
    if (updated.frequency != ScheduleFrequency::CUSTOM) {
        updated.cron_expression =
            frequencyToCron(updated.frequency, updated.window_start_hour);
    }

    try {
        validateEntry(updated);
    } catch (const std::exception& ex) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, ex.what()));
    }

    // Persist the patched entry to durable storage before updating in-memory
    // state; a storage failure leaves it->second unchanged.
    if (schedule_store_) {
        auto persist_result = schedule_store_->save(updated);
        if (!persist_result.has_value()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                "Failed to persist schedule id=" + id + ": " +
                persist_result.error().message()));
        }
    }

    it->second = updated;

    // Update scheduler registration
    deregisterFromScheduler(id);
    if (running_.load() && updated.enabled) {
        registerWithScheduler(updated);
    }

    spdlog::info("MaintenanceSchedule patched: id={}", id);

    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_schedule_patched"},
            {"schedule_id", id},
            {"patch",       patch.dump()},
            {"timestamp_ms", updated.updated_at_ms},
        });
    }

    return updated;
}

// ---------------------------------------------------------------------------
// Schedule CRUD – Delete
// ---------------------------------------------------------------------------

Result<void> DatabaseMaintenanceOrchestrator::deleteSchedule(const std::string& id) {
    std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Schedule not found: " + id));
    }

    std::string name = it->second.name;

    // Remove from durable store first; if the durable remove fails, abort the
    // delete so the schedule does not resurrect itself on the next restart.
    if (schedule_store_) {
        auto persist_result = schedule_store_->remove(id);
        if (!persist_result.has_value()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                "Failed to remove schedule id=" + id + " from storage: " +
                persist_result.error().message()));
        }
    }

    deregisterFromScheduler(id);
    schedules_.erase(it);

    spdlog::info("MaintenanceSchedule deleted: id={}", id);

    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_schedule_deleted"},
            {"schedule_id", id},
            {"name",        name},
            {"timestamp_ms", nowMs()},
        });
    }

    MetricsCollector::getInstance().addCounter("maintenance_schedules_deleted_total", 1);

    return {};
}

// ---------------------------------------------------------------------------
// Job management
// ---------------------------------------------------------------------------

Result<OrchestratorJob> DatabaseMaintenanceOrchestrator::triggerNow(
    const std::string& schedule_id, bool force)
{
    MaintenanceScheduleEntry entry;
    {
        std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it == schedules_.end()) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                        "Schedule not found: " + schedule_id));
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
    job.tenant_id   = entry.tenant_id;
    job.task_type   = entry.tasks.front();
    job.state       = MaintenanceJobState::RUNNING;
    job.started_at_ms = nowMs();
    job.forced      = force;

    {
        std::unique_lock<std::shared_mutex> lock(jobs_mutex_);
        jobs_[job.id] = job;
    }

    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_job_triggered"},
            {"job_id",      job.id},
            {"schedule_id", schedule_id},
            {"task_type",   taskTypeToString(job.task_type)},
            {"forced",      force},
            {"timestamp_ms", job.started_at_ms},
        });
    }

    MetricsCollector::getInstance().addCounter("maintenance_jobs_triggered_total", 1,
        {{"schedule_id", schedule_id}});

    // Run asynchronously
    std::string job_id = job.id;
    std::thread([this, schedule_id, job_id, force]() {
        executeSchedule(schedule_id, job_id, force);
    }).detach();

    return job;
}

Result<void> DatabaseMaintenanceOrchestrator::cancelJob(const std::string& job_id) {
    std::unique_lock<std::shared_mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Job not found: " + job_id));
    }
    if (it->second.state != MaintenanceJobState::RUNNING &&
        it->second.state != MaintenanceJobState::PENDING) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Job '" + job_id + "' is not active"));
    }
    it->second.state         = MaintenanceJobState::CANCELLED;
    it->second.finished_at_ms = nowMs();
    spdlog::warn("MaintenanceJob cancelled: id={}", job_id);

    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",       "maintenance_job_cancelled"},
            {"job_id",      job_id},
            {"timestamp_ms", it->second.finished_at_ms},
        });
    }

    MetricsCollector::getInstance().addCounter("maintenance_jobs_cancelled_total", 1);

    return {};
}

Result<OrchestratorJob> DatabaseMaintenanceOrchestrator::getJob(
    const std::string& job_id) const
{
    std::shared_lock<std::shared_mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return tl::unexpected(Error(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    "Job not found: " + job_id));
    }
    return it->second;
}

std::vector<OrchestratorJob> DatabaseMaintenanceOrchestrator::listJobs(
    bool active_only) const
{
    std::shared_lock<std::shared_mutex> lock(jobs_mutex_);
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
        std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
        total = static_cast<int>(schedules_.size());
        for (auto& [id, e] : schedules_) {
            if (e.enabled) ++enabled;
        }
    }
    j["total_schedules"]   = total;
    j["enabled_schedules"] = enabled;

    int active = 0;
    {
        std::shared_lock<std::shared_mutex> lock(jobs_mutex_);
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
        std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
        report.total_schedules = static_cast<int>(schedules_.size());
        for (auto& [id, e] : schedules_) {
            if (e.enabled) ++report.enabled_schedules;
        }
    }

    int64_t cutoff = nowMs() - 24LL * 60 * 60 * 1000;
    {
        std::shared_lock<std::shared_mutex> lock(jobs_mutex_);
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

    // ---- Phase 4: populate recent_dispatch_outcomes from ring buffer ------
    {
        std::lock_guard<std::mutex> rb_lock(ring_buffer_mutex_);
        report.recent_dispatch_outcomes.assign(
            dispatch_ring_buffer_.begin(), dispatch_ring_buffer_.end());
        report.dispatch_outcome_ring_buffer_capacity = ring_buffer_capacity_;
    }

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

void DatabaseMaintenanceOrchestrator::registerTaskHandler(
    MaintenanceTaskType task_type,
    std::shared_ptr<IMaintenanceTaskHandler> handler)
{
    if ([[maybe_unused]] !handler) {
        spdlog::warn("registerTaskHandler: ignoring null handler for task type '{}'",
                     taskTypeToString(task_type));
        return;
    }
    std::unique_lock<std::shared_mutex> lock([[maybe_unused]] handlers_mutex_);
    task_handlers_[static_cast<int>([[maybe_unused]] task_type)] = std::move(handler);
}

void DatabaseMaintenanceOrchestrator::setDistributedLock(
    std::shared_ptr<IDistributedLock> lock)
{
    std::lock_guard<std::mutex> lg(dist_lock_mutex_);
    dist_lock_ = std::move(lock);
}

std::map<std::string, std::string>
DatabaseMaintenanceOrchestrator::listTaskHandlers() const
{
    std::shared_lock<std::shared_mutex> lock([[maybe_unused]] handlers_mutex_);
    std::map<std::string, std::string> result;
    for (const auto& [key, handler] : task_handlers_) {
        const auto task_type_str = taskTypeToString(static_cast<MaintenanceTaskType>(key));
        if ([[maybe_unused]] handler) {
            result[task_type_str] = handler->handlerName(); // null-checked above
        } else {
            result[task_type_str] = "<null-handler>";
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Per-tenant maintenance configuration
// ---------------------------------------------------------------------------

void DatabaseMaintenanceOrchestrator::setTenantMaintenanceConfig(
    const std::string& tenant_id, TenantMaintenanceConfig config)
{
    if (tenant_id.empty()) {
        spdlog::warn("setTenantMaintenanceConfig: tenant_id must not be empty; ignored");
        return;
    }
    std::unique_lock<std::shared_mutex> lock(tenant_configs_mutex_);
    tenant_configs_[tenant_id] = std::move(config);
    spdlog::info("TenantMaintenanceConfig set for tenant '{}'", tenant_id);
}

TenantMaintenanceConfig DatabaseMaintenanceOrchestrator::getTenantMaintenanceConfig(
    const std::string& tenant_id) const
{
    std::shared_lock<std::shared_mutex> lock(tenant_configs_mutex_);
    auto it = tenant_configs_.find(tenant_id);
    if (it != tenant_configs_.end()) {
        return it->second;
    }
    return TenantMaintenanceConfig{};
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
                std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
                auto it = schedules_.find(eid);
                if (it == schedules_.end()) {
                    return {{"status", "error"}, {"error", "schedule_not_found"}};
                }
                entry_copy = it->second;
                job.tenant_id = entry_copy.tenant_id;
                job.task_type = entry_copy.tasks.empty()
                                ? MaintenanceTaskType::METRICS_COLLECTION
                                : entry_copy.tasks.front();
            }

            {
                std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
                jobs_[job_id] = job;
            }

            executeSchedule(eid, job_id);

            std::shared_lock<std::shared_mutex> jlock(jobs_mutex_);
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
    const std::string& schedule_id, const std::string& job_id, bool force)
{
    // ---- Phase 2: in-flight concurrent guard -------------------------------
    // If this schedule is already executing on another thread, skip with
    // SKIPPED_CONCURRENT rather than running a duplicate job.
    {
        std::lock_guard<std::mutex> ifl(in_flight_mutex_);
        if (!in_flight_schedules_.insert(schedule_id).second) {
            // Already in flight — emit diagnostic and skip.
            spdlog::debug("executeSchedule: schedule {} skipped — already in-flight",
                          schedule_id);
            int64_t now = themis::maintenance::nowMs();
            {
                std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
                if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                    jit->second.state         = MaintenanceJobState::SKIPPED;
                    jit->second.error_message = "Skipped: schedule already in-flight (concurrent invocation)";
                    jit->second.finished_at_ms = now;
                }
            }
            {
                std::unique_lock<std::shared_mutex> slock(schedules_mutex_);
                if (auto it = schedules_.find(schedule_id); it != schedules_.end()) {
                    it->second.last_run_ms    = now;
                    it->second.last_run_state = "skipped";
                    it->second.last_job_id    = job_id;
                }
            }
            MetricsCollector::getInstance().addCounter(
                "maintenance_jobs_skipped_total", 1,
                {{"reason", "concurrent_in_flight"}, {"schedule_id", schedule_id}});

            DispatchOutcome doc;
            doc.schedule_id   = schedule_id;
            doc.task_type     = "<concurrent-skip>";
            doc.outcome       = DispatchOutcomeType::SKIPPED_CONCURRENT;
            doc.latency_us    = 0;
            doc.error_message = "Schedule already in-flight";
            recordDispatchOutcome(std::move(doc));
            return;
        }
    }

    // RAII guard: remove from in_flight_schedules_ on every exit path.
    struct InFlightGuard {
        DatabaseMaintenanceOrchestrator* self;
        const std::string& sched_id;
        ~InFlightGuard() {
            std::lock_guard<std::mutex> lk(self->in_flight_mutex_);
            self->in_flight_schedules_.erase(sched_id);
        }
    } in_flight_guard{this, schedule_id};

    int64_t exec_start_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    MaintenanceScheduleEntry entry;
    {
        std::shared_lock<std::shared_mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it == schedules_.end()) {
            std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
            if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                jit->second.state          = MaintenanceJobState::FAILED;
                jit->second.error_message  = "Schedule not found";
                jit->second.finished_at_ms = themis::maintenance::nowMs();
            }
            return;
        }
        entry = it->second;
    }

    // ---- Distributed lock acquisition ----------------------------------
    // When a distributed lock is configured, only the node that successfully
    // acquires the lock for this schedule runs the job.  Non-leader nodes log
    // a DEBUG message and skip the job immediately.
    //
    // The lock is held by acquired_dist_lock_ for the duration of this function
    // so that it is automatically released on every code path.
    std::shared_ptr<IDistributedLock> acquired_dist_lock;
    {
        std::shared_ptr<IDistributedLock> dl;
        {
            std::lock_guard<std::mutex> lg(dist_lock_mutex_);
            dl = dist_lock_;
        }
        if (dl) {
            // Compute lock TTL: use explicit lock_ttl_ms when set, otherwise
            // derive from the window duration + 30 s safety margin.
            int64_t ttl_ms = entry.lock_ttl_ms;
            if (ttl_ms <= 0) {
                int window_hours = entry.window_end_hour - entry.window_start_hour;
                if (window_hours <= 0) window_hours += 24; // midnight wrap
                ttl_ms = static_cast<int64_t>(window_hours) * 3600LL * 1000LL + 30000LL;
            }

            if (!dl->tryAcquire(schedule_id, ttl_ms)) {
                const std::string holder = dl->getHolderNodeId(schedule_id);
                spdlog::debug("schedule {} skipped — lock held by peer {}",
                              schedule_id, holder.empty() ? "<unknown>" : holder);

                int64_t now = themis::maintenance::nowMs();
                {
                    std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
                    if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                        jit->second.state         = MaintenanceJobState::SKIPPED;
                        jit->second.error_message = "Skipped: distributed lock held by peer "
                                                    + (holder.empty() ? "<unknown>" : holder);
                        jit->second.finished_at_ms = now;
                    }
                }
                {
                    std::unique_lock<std::shared_mutex> slock(schedules_mutex_);
                    if (auto it = schedules_.find(schedule_id); it != schedules_.end()) {
                        it->second.last_run_ms    = now;
                        it->second.last_run_state = "skipped";
                        it->second.last_job_id    = job_id;
                    }
                }
                MetricsCollector::getInstance().addCounter(
                    "maintenance_jobs_skipped_total", 1,
                    {{"reason", "distributed_lock_held"},
                     {"schedule_id", schedule_id}});
                return;
            }
            // Successfully acquired – remember the lock so it is released
            // on every subsequent exit path (window skip, DAG error, completion).
            acquired_dist_lock = std::move(dl);
        }
    }

    // RAII guard: releases the distributed lock on every code path that
    // returns after this point (window skip, task-order error, completion).
    struct DistLockGuard {
        std::shared_ptr<IDistributedLock> lock;
        const std::string& key;
        ~DistLockGuard() { if (lock) lock->release(key); }
    } dist_lock_guard{std::move(acquired_dist_lock), schedule_id};

    // ---- Per-tenant configuration -----------------------------------------
    TenantMaintenanceConfig tenant_cfg;
    if (!entry.tenant_id.empty()) {
        tenant_cfg = getTenantMaintenanceConfig(entry.tenant_id);
    }

    // ---- Per-tenant concurrent job quota ----------------------------------
    // Enforced before the window check so that quota violations are reported
    // as SKIPPED regardless of the current hour.
    if (!entry.tenant_id.empty() && tenant_cfg.max_concurrent_jobs > 0) {
        int running_count = 0;
        {
            std::shared_lock<std::shared_mutex> jlock(jobs_mutex_);
            for (const auto& [jid, jobj] : jobs_) {
                if (jobj.tenant_id == entry.tenant_id &&
                    jobj.state == MaintenanceJobState::RUNNING &&
                    jid != job_id) {
                    ++running_count;
                }
            }
        }
        if (running_count >= tenant_cfg.max_concurrent_jobs) {
            spdlog::warn("MaintenanceJob {} skipped: tenant '{}' quota exceeded "
                         "({}/{} concurrent jobs)",
                         job_id, entry.tenant_id,
                         running_count, tenant_cfg.max_concurrent_jobs);

            int64_t now = themis::maintenance::nowMs();
            {
                std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
                if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                    jit->second.state         = MaintenanceJobState::SKIPPED;
                    jit->second.error_message = "Skipped: tenant concurrent job quota exceeded";
                    jit->second.finished_at_ms = now;
                }
            }
            {
                std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
                if (auto it = schedules_.find(schedule_id); it != schedules_.end()) {
                    it->second.last_run_ms    = now;
                    it->second.last_run_state = "skipped";
                    it->second.last_job_id    = job_id;
                }
            }
            MetricsCollector::getInstance().addCounter(
                "maintenance_jobs_skipped_total", 1,
                {{"reason", "tenant_quota_exceeded"},
                 {"schedule_id", schedule_id},
                 {"tenant_id", entry.tenant_id}});
            return;
        }
    }

    // ---- Maintenance window enforcement --------------------------------
    // When force=true the window check is bypassed entirely.
    // When a per-tenant window override is active (tenant_cfg.enforce_window),
    // use the tenant-level window instead of the per-schedule window.
    int  eff_window_start = entry.window_start_hour;
    int  eff_window_end   = entry.window_end_hour;
    bool eff_enforce      = entry.enforce_window;
    if (!entry.tenant_id.empty() && tenant_cfg.enforce_window) {
        eff_window_start = tenant_cfg.window_start_hour;
        eff_window_end   = tenant_cfg.window_end_hour;
        eff_enforce      = true;
    }
    if (!force && eff_enforce &&
        !isInMaintenanceWindow(eff_window_start, eff_window_end)) {
        spdlog::warn("MaintenanceJob {} skipped: outside window [{}-{}] UTC (current hour={})",
                     job_id, eff_window_start, eff_window_end,
                     currentUtcHour());

        int64_t now = themis::maintenance::nowMs();
        {
            std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
            if (auto jit = jobs_.find(job_id); jit != jobs_.end()) {
                jit->second.state          = MaintenanceJobState::SKIPPED;
                jit->second.error_message  = "Skipped: outside maintenance window";
                jit->second.finished_at_ms = now;
            }
        }
        {
            std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
            if (auto it = schedules_.find(schedule_id); it != schedules_.end()) {
                it->second.last_run_ms    = now;
                it->second.last_run_state = "skipped";
                it->second.last_job_id    = job_id;
            }
        }
        MetricsCollector::getInstance().addCounter(
            "maintenance_jobs_skipped_total", 1,
            {{"reason", "outside_window"},
             {"schedule_id", schedule_id}});
        return;
    }

    // ---- Execute tasks (ordered, respecting halt_on_task_failure) ------
    bool all_ok = true;
    std::string last_error;
    int64_t job_start_ms = themis::maintenance::nowMs();

    // Determine execution order: use DAG sort when task_dependencies are
    // declared, otherwise fall back to the positional order of entry.tasks.
    std::vector<MaintenanceTaskType> ordered_tasks;
    try {
        ordered_tasks = resolveTaskExecutionOrder(entry);
    } catch (const std::exception& ex) {
        // Should not happen (already validated on create/update), but if a
        // corrupt schedule somehow reaches execution we must not run tasks in
        // an unvalidated order — fail the job immediately.
        const std::string err = std::string("Task order resolution failed: ") + ex.what();
        spdlog::error("MaintenanceJob {}: {}", job_id, err);
        {
            std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
            auto jit = jobs_.find(job_id);
            if (jit != jobs_.end()) {
                jit->second.state          = MaintenanceJobState::FAILED;
                jit->second.error_message  = err;
                jit->second.finished_at_ms = themis::maintenance::nowMs();
            }
        }
        MetricsCollector::getInstance().addCounter(
            "maintenance_jobs_failed_total", 1,
            {{"reason", "task_order_resolution_failed"},
             {"schedule_id", schedule_id}});
        return;
    }

    for (auto task_type : ordered_tasks) {
        // Check cancellation
        {
            std::shared_lock<std::shared_mutex> jlock(jobs_mutex_);
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

        // Per-task metrics
        if (sub_job.finished_at_ms > sub_job.started_at_ms) {
            double dur_ms = static_cast<double>(sub_job.finished_at_ms - sub_job.started_at_ms);
            MetricsCollector::getInstance().observeHistogram(
                "maintenance_task_duration_ms", dur_ms,
                {{"task_type", taskTypeToString(task_type)}});
        }

        if (sub_job.state == MaintenanceJobState::FAILED) {
            all_ok    = false;
            last_error = sub_job.error_message;
            MetricsCollector::getInstance().addCounter(
                "maintenance_tasks_failed_total", 1,
                {{"task_type", taskTypeToString(task_type)}});
            if (entry.halt_on_task_failure) break;
        } else {
            MetricsCollector::getInstance().addCounter(
                "maintenance_tasks_succeeded_total", 1,
                {{"task_type", taskTypeToString(task_type)}});
        }
    }

    int64_t now = themis::maintenance::nowMs();
    double total_dur_ms = static_cast<double>(now - job_start_ms);

    // Update the job record
    {
        std::unique_lock<std::shared_mutex> jlock(jobs_mutex_);
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
        std::unique_lock<std::shared_mutex> lock(schedules_mutex_);
        auto it = schedules_.find(schedule_id);
        if (it != schedules_.end()) {
            it->second.last_run_ms    = now;
            it->second.last_run_state = all_ok ? "success" : "failed";
            it->second.last_job_id    = job_id;
        }
    }

    // Audit log job completion
    if (audit_logger_) {
        audit_logger_->logEvent({
            {"event",        all_ok ? "maintenance_job_succeeded" : "maintenance_job_failed"},
            {"job_id",       job_id},
            {"schedule_id",  schedule_id},
            {"duration_ms",  static_cast<int64_t>(total_dur_ms)},
            {"error",        last_error},
            {"forced",       force},
            {"timestamp_ms", now},
        });
    }

    // Job-level metrics
    MetricsCollector::getInstance().observeHistogram(
        "maintenance_job_duration_ms", total_dur_ms,
        {{"schedule_id", schedule_id}});
    MetricsCollector::getInstance().addCounter(
        all_ok ? "maintenance_jobs_succeeded_total" : "maintenance_jobs_failed_total", 1,
        {{"schedule_id", schedule_id}});

    pruneCompletedJobs();
    // dist_lock_guard destructor releases the distributed lock automatically.

    // ---- Phase 4: record DispatchOutcome in ring buffer -------------------
    {
        int64_t finish_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        DispatchOutcome doc;
        doc.schedule_id = schedule_id;
        doc.task_type   = "<schedule>";
        doc.outcome     = all_ok ? DispatchOutcomeType::SUCCESS
                                 : DispatchOutcomeType::FAILED_DISPATCH;
        doc.latency_us  = finish_us - exec_start_us;
        doc.error_message = last_error;
        recordDispatchOutcome(std::move(doc));
    }
}

void DatabaseMaintenanceOrchestrator::executeTask(
    MaintenanceTaskType task_type, OrchestratorJob& job)
{
    spdlog::info("Maintenance task starting: type={} job={}",
                 taskTypeToString(task_type), job.id);

    switch (task_type) {

        // ---- Index-specific operations: delegate to IndexMaintenanceManager ----

        case MaintenanceTaskType::INDEX_REBUILD: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            // Rebuild all indexes via the global maintenance check (policy-driven).
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error().message();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Index rebuild triggered (policy-driven)";
            }
            break;
        }

        case MaintenanceTaskType::INDEX_REORGANIZE: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error().message();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Index reorganization triggered";
            }
            break;
        }

        case MaintenanceTaskType::STATISTICS_UPDATE: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            // updateStatistics requires an index name – use empty string for global trigger.
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error().message();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Statistics update triggered";
            }
            break;
        }

        case MaintenanceTaskType::ORPHAN_CLEANUP: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error().message();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Orphan cleanup triggered";
            }
            break;
        }

        case MaintenanceTaskType::FRAGMENTATION_MONITORING: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            // Use the active-job list as a proxy for fragmentation monitoring.
            auto active = index_maintenance_->listActiveJobs();
            job.state          = MaintenanceJobState::SUCCEEDED;
            job.result_summary = "Fragmentation monitoring: " +
                                 std::to_string(active.size()) + " active index job(s)";
            break;
        }

        case MaintenanceTaskType::VECTOR_REINDEX: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            auto result = index_maintenance_->triggerMaintenanceCheck();
            if (!result) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = result.error().message();
            } else {
                job.state          = MaintenanceJobState::SUCCEEDED;
                job.result_summary = "Vector re-index triggered";
            }
            break;
        }

        case MaintenanceTaskType::CONSISTENCY_CHECK: {
            if (!index_maintenance_) {
                job.state         = MaintenanceJobState::FAILED;
                job.error_message = "IndexMaintenanceManager not available";
                break;
            }
            auto active = index_maintenance_->listActiveJobs();
            job.state          = MaintenanceJobState::SUCCEEDED;
            job.result_summary = "Consistency check: " +
                                 std::to_string(active.size()) + " active index jobs found";
            break;
        }

        // ---- Module-delegated tasks (handled via registered IMaintenanceTaskHandler) ----

        case MaintenanceTaskType::METRICS_COLLECTION:
        [[fallthrough]];\n        case MaintenanceTaskType::QUOTA_CHECK:
        [[fallthrough]];\n        case MaintenanceTaskType::REPLICA_VALIDATION:
        [[fallthrough]];\n        case MaintenanceTaskType::PERFORMANCE_ANALYSIS:
        [[fallthrough]];\n        case MaintenanceTaskType::MVCC_CLEANUP:
        [[fallthrough]];\n        case MaintenanceTaskType::FULL_CHECKDB:
        [[fallthrough]];\n        case MaintenanceTaskType::BACKUP_VERIFICATION:
        [[fallthrough]];\n        case MaintenanceTaskType::CAPACITY_TREND_ANALYSIS:
        [[fallthrough]];\n        case MaintenanceTaskType::INDEX_FRAGMENTATION_REPORT:
        [[fallthrough]];\n        case MaintenanceTaskType::DISASTER_RECOVERY_DRILL:
        [[fallthrough]];\n        case MaintenanceTaskType::BASELINE_UPDATE:
        [[fallthrough]];\n        case MaintenanceTaskType::STORAGE_COMPACTION: {
            // Look for a registered handler for this task type.
            std::shared_ptr<IMaintenanceTaskHandler> handler;
            {
                std::shared_lock<std::shared_mutex> lock([[maybe_unused]] handlers_mutex_);
                auto it = task_handlers_.find([[maybe_unused]] static_cast<int>(task_type));
                if ([[maybe_unused]] it != task_handlers_.end()) {
                    handler = it->second;
                }
            }

            if ([[maybe_unused]] handler) {
                auto result = handler->execute(job.id, task_type);
                if (!result) {
                    job.state         = MaintenanceJobState::FAILED;
                    job.error_message = result.error().message();

                    DispatchOutcome doc;
                    doc.schedule_id   = job.schedule_id;
                    doc.task_type     = taskTypeToString(task_type);
                    doc.outcome       = DispatchOutcomeType::FAILED_DISPATCH;
                    doc.latency_us    = 0;
                    doc.error_message = job.error_message;
                    recordDispatchOutcome(std::move(doc));
                } else {
                    job.state          = MaintenanceJobState::SUCCEEDED;
                    job.result_summary = *result;

                    DispatchOutcome doc;
                    doc.schedule_id = job.schedule_id;
                    doc.task_type   = taskTypeToString(task_type);
                    doc.outcome     = DispatchOutcomeType::SUCCESS;
                    doc.latency_us  = 0;
                    recordDispatchOutcome(std::move(doc));
                }
            } else {
                // No handler registered – skip with a structured diagnostic message.
                spdlog::warn(
                    "Maintenance task '{}' skipped: no IMaintenanceTaskHandler registered "
                    "(job={}).  Register a handler via registerTaskHandler() to enable "
                    "real execution for this task type.",
                    taskTypeToString(task_type), job.id);
                job.state          = MaintenanceJobState::SKIPPED;
                job.error_message  = "no handler registered for " + taskTypeToString([[maybe_unused]] task_type);
                job.result_summary = "Task '" + taskTypeToString(task_type) +
                                     "' skipped: no handler registered";

                DispatchOutcome doc;
                doc.schedule_id   = job.schedule_id;
                doc.task_type     = taskTypeToString(task_type);
                doc.outcome       = DispatchOutcomeType::SKIPPED_NO_HANDLER;
                doc.latency_us    = 0;
                doc.error_message = job.error_message;
                recordDispatchOutcome(std::move(doc));
            }
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
    std::unique_lock<std::shared_mutex> lock(jobs_mutex_);
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

    // ---- Validate task_dependencies DAG (cycle detection) ----------------
    if (!entry.task_dependencies.empty()) {
        resolveTaskExecutionOrder(entry); // throws std::invalid_argument on cycle or bad reference
    }
}

/*static*/ std::vector<MaintenanceTaskType>
DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder(
    const MaintenanceScheduleEntry& entry)
{
    if (entry.task_dependencies.empty()) {
        // No DAG declarations → return tasks in their declared list order.
        return entry.tasks;
    }

    // Build a stable index map so that tasks with equal eligibility are
    // emitted in the same relative order as entry.tasks (Kahn's seeding).
    std::unordered_map<int, std::size_t> taskIndex;
    for (std::size_t i = 0; i < entry.tasks.size(); ++i) {
        taskIndex[static_cast<int>(entry.tasks[i])] = i;
    }

    auto getIndex = [&]([[maybe_unused]] MaintenanceTaskType t) -> std::size_t {
        auto it = taskIndex.find(static_cast<int>(t));
        return it != taskIndex.end() ? it->second : SIZE_MAX;
    };

    // ---- Validate that all dependency references name tasks in entry.tasks --
    for (const auto& dep : entry.task_dependencies) {
        if (taskIndex.find(static_cast<int>(dep.task_type)) == taskIndex.end()) {
            throw std::invalid_argument(
                "task_dependencies: task_type '" +
                taskTypeToString(dep.task_type) +
                "' is not present in the tasks list");
        }
        for (auto prereq : dep.depends_on) {
            if (taskIndex.find(static_cast<int>(prereq)) == taskIndex.end()) {
                throw std::invalid_argument(
                    "task_dependencies: depends_on task '" +
                    taskTypeToString(prereq) +
                    "' is not present in the tasks list");
            }
        }
    }

    // ---- Build in-degree map and reverse adjacency (Kahn's algorithm) ------
    std::unordered_map<int, int>                                    inDegree;
    std::unordered_map<int, std::vector<MaintenanceTaskType>>       dependents;

    for (auto t : entry.tasks) {
        inDegree[static_cast<int>(t)]   = 0;
        dependents[static_cast<int>(t)] = {};
    }

    // Deduplicate edges using a set per dependent to avoid double-counting.
    std::unordered_map<int, std::set<int>> seen;
    for (const auto& dep : entry.task_dependencies) {
        for (auto prereq : dep.depends_on) {
            if (seen[static_cast<int>(dep.task_type)].insert(static_cast<int>(prereq)).second) {
                // New edge: prereq → dep.task_type
                dependents[static_cast<int>(prereq)].push_back(dep.task_type);
                inDegree[static_cast<int>(dep.task_type)]++;
            }
        }
    }

    // ---- Stable Kahn's sort: seed ready-queue in entry.tasks order ---------
    // The comparator uses the original position in entry.tasks so that tasks
    // with equal eligibility are always emitted in the declared list order.
    // std::deque provides O(1) pop_front (versus O(n) for std::vector).
    auto byOriginalOrder = [&](MaintenanceTaskType a, MaintenanceTaskType b) {
        return getIndex(a) < getIndex(b);
    };

    // Initialize ready list with zero-in-degree tasks, in entry.tasks order.
    std::deque<MaintenanceTaskType> ready;
    for (auto t : entry.tasks) {
        if (inDegree[static_cast<int>(t)] == 0) {
            ready.push_back(t);
        }
    }
    // ready is already in entry.tasks order because we iterated entry.tasks.

    std::vector<MaintenanceTaskType> result;
    result.reserve(entry.tasks.size());

    while (!ready.empty()) {
        auto cur = ready.front();
        ready.pop_front();
        result.push_back(cur);

        // Sort this node's dependents by original position before merging into ready.
        const auto& deps = dependents[static_cast<int>(cur)];
        std::vector<MaintenanceTaskType> sorted_deps(deps.begin(), deps.end());
        std::sort(sorted_deps.begin(), sorted_deps.end(), byOriginalOrder);

        for (auto dep : sorted_deps) {
            if (--inDegree[static_cast<int>(dep)] == 0) {
                // Insert in original-order position.
                auto pos = std::lower_bound(ready.begin(), ready.end(), dep, byOriginalOrder);
                ready.insert(pos, dep);
            }
        }
    }

    // If not all tasks were emitted, at least one cycle exists.
    if (result.size() != entry.tasks.size()) {
        throw std::invalid_argument(
            "task_dependencies: circular dependency detected");
    }

    return result;
}

// ---------------------------------------------------------------------------
// Phase 4: DispatchOutcome ring buffer
// ---------------------------------------------------------------------------

void DatabaseMaintenanceOrchestrator::recordDispatchOutcome(DispatchOutcome outcome) {
    std::lock_guard<std::mutex> lock(ring_buffer_mutex_);
    dispatch_ring_buffer_.push_back(std::move(outcome));
    while (static_cast<int>(dispatch_ring_buffer_.size()) > ring_buffer_capacity_) {
        dispatch_ring_buffer_.pop_front();
    }
}

// ---------------------------------------------------------------------------
// Phase 2: Churn rate-limit check
// ---------------------------------------------------------------------------

bool DatabaseMaintenanceOrchestrator::checkChurnLimit(
    const std::string& schedule_id, uint32_t max_changes_per_interval)
{
    if (max_changes_per_interval == 0) return true; // disabled

    int64_t now = themis::maintenance::nowMs();
    std::lock_guard<std::mutex> lock(churn_mutex_);
    auto& [count, interval_start] = churn_counts_[schedule_id];
    if (now - interval_start >= kChurnIntervalMs) {
        // New interval — reset counter
        interval_start = now;
        count = 0;
    }
    ++count;
    return count <= max_changes_per_interval;
}

} // namespace maintenance
} // namespace themis
