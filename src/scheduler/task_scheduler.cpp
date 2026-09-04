/**
 * @file task_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=4, H=16, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scheduler/task_scheduler.h"
#include <stdexcept>
#include "scheduler/event_trigger.h"
#include "scheduler/task_audit_manager.h"
#include "scheduler/task_audit_event.h"
#include "scheduler/task_result_store.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"
#include "security/aql_injection_detector.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/audit_logger.h"
#include "utils/cron_parser.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include "themis/base/module_sandbox.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <deque>
#include <cctype>
#include <random>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#ifdef ERROR
#undef ERROR
#endif

// ⚠️ SECURITY WARNING: This implementation executes arbitrary AQL queries and functions.
// Production deployments MUST implement proper security controls:
// - Authentication and authorization for all task operations
// - Input validation and query sanitization
// - Resource limits and quotas per task
// - Comprehensive audit logging
// - Secure task definition storage (encryption at rest)
// - Sandboxed execution environments
//
// ---------------------------------------------------------------------------
// Note: User authentication context is propagated via thread-local
// TaskScheduler::RequestContext.  HTTP handlers call
//   TaskScheduler::setRequestContext({user_id, client_ip});
// before invoking scheduler operations, and clearRequestContext() afterwards.
// All audit events use currentUserId() / currentClientIp() accessors which
// return the thread-local values or "system" / "" as safe fallbacks.
// ---------------------------------------------------------------------------

namespace themis {

// ---------------------------------------------------------------------------
// Thread-local RequestContext  (replaces hardcoded "system" audit user)
// ---------------------------------------------------------------------------

namespace {
struct TLSRequestContext {
    std::string user_id;
    std::string client_ip;
    std::unordered_set<std::string> granted_permissions;
    std::unordered_set<std::string> roles;
    std::string authorization_justification;
    bool set = false;
};

constexpr const char* kAdminRole = "admin";
constexpr const char* kSystemAdminRole = "system_admin";
std::atomic<bool> g_warned_missing_permission_context{false};
std::atomic<bool> g_warned_missing_role_context{false};

bool hasSuperuserRole(const std::unordered_set<std::string>& roles) {
    return roles.count(kAdminRole) > 0 || roles.count(kSystemAdminRole) > 0;
}
} // anonymous namespace

static thread_local TLSRequestContext tls_request_ctx;

void TaskScheduler::setRequestContext(const RequestContext& ctx) noexcept {
    tls_request_ctx.user_id = ctx.user_id;
    tls_request_ctx.client_ip = ctx.client_ip;
    tls_request_ctx.granted_permissions = ctx.granted_permissions;
    tls_request_ctx.roles = ctx.roles;
    tls_request_ctx.authorization_justification = ctx.authorization_justification;
    tls_request_ctx.set = true;
}

void TaskScheduler::clearRequestContext() noexcept {
    tls_request_ctx.user_id.clear();
    tls_request_ctx.client_ip.clear();
    tls_request_ctx.granted_permissions.clear();
    tls_request_ctx.roles.clear();
    tls_request_ctx.authorization_justification.clear();
    tls_request_ctx.set = false;
}

std::string TaskScheduler::currentUserId(const char* fallback) noexcept {
    if (tls_request_ctx.set && !tls_request_ctx.user_id.empty()) {
        return tls_request_ctx.user_id;
    }
    return fallback ? fallback : "system";
}

std::string TaskScheduler::currentClientIp() noexcept {
    if (tls_request_ctx.set) {
        return tls_request_ctx.client_ip;
    }
    return {};
}

bool TaskScheduler::hasPermission(const std::string& permission) noexcept {
    if (!tls_request_ctx.set) {
        if (!g_warned_missing_permission_context.exchange(true)) {
            THEMIS_WARN(
                "TaskScheduler::hasPermission fallback allow without request context (permission='{}'). Configure RequestContext in security-sensitive entry points.",
                permission);
        }
        // Backward-compatible fallback for trusted in-process calls.
        return true;
    }
    return tls_request_ctx.granted_permissions.count(permission) > 0 ||
           hasSuperuserRole(tls_request_ctx.roles);
}

bool TaskScheduler::hasRole(const std::string& role) noexcept {
    if (!tls_request_ctx.set) {
        if (!g_warned_missing_role_context.exchange(true)) {
            THEMIS_WARN(
                "TaskScheduler::hasRole fallback allow without request context (role='{}'). Configure RequestContext in security-sensitive entry points.",
                role);
        }
        // Backward-compatible fallback for trusted in-process calls.
        return true;
    }
    return tls_request_ctx.roles.count(role) > 0 || hasSuperuserRole(tls_request_ctx.roles);
}

std::string TaskScheduler::currentAuthorizationJustification(const char* fallback) noexcept {
    if (tls_request_ctx.set && !tls_request_ctx.authorization_justification.empty()) {
        return tls_request_ctx.authorization_justification;
    }
    return fallback ? fallback : "";
}

// ---------------------------------------------------------------------------

// Default values for audit context (when auth context not available)
static constexpr const char* DEFAULT_AUDIT_USER = "system";
static constexpr const char* DEFAULT_AUDIT_IP = "localhost";

// Helper function to set audit context from thread-local RequestContext
static void setDefaultAuditContext([[maybe_unused]] scheduler::TaskAuditEvent& event) {
    event.user_id    = TaskScheduler::currentUserId([[maybe_unused]] DEFAULT_AUDIT_USER);
    const auto ip    = TaskScheduler::currentClientIp();
    event.ip_address = ip.empty() ? DEFAULT_AUDIT_IP : ip;
}

static void setDefaultAuditContext([[maybe_unused]] scheduler::TaskSecurityEvent& event) {
    event.user_id    = TaskScheduler::currentUserId([[maybe_unused]] DEFAULT_AUDIT_USER);
    const auto ip    = TaskScheduler::currentClientIp();
    event.ip_address = ip.empty() ? DEFAULT_AUDIT_IP : ip;
}

static void logUnauthorizedPermissionAttempt(
    const std::shared_ptr<scheduler::TaskAuditManager>& audit_manager,
    const std::string& task_id,
    const std::string& task_name,
    const std::string& required_permission,
    const std::string& operation,
    const std::string& reason)
{
    THEMIS_WARN(
        "Unauthorized scheduler operation denied: op='{}' task_id='{}' task_name='{}' user='{}' permission='{}' reason='{}' justification='{}'",
        operation,
        task_id,
        task_name,
        TaskScheduler::currentUserId(DEFAULT_AUDIT_USER),
        required_permission,
        reason,
        TaskScheduler::currentAuthorizationJustification("none"));

    if (!audit_manager) {
        return;
    }

    scheduler::TaskSecurityEvent security_event;
    security_event.uuid = scheduler::generateUUID();
    security_event.timestamp = std::chrono::system_clock::now();
    security_event.task_id = task_id;
    security_event.task_name = task_name;
    security_event.event_type = scheduler::TaskSecurityEventType::UNAUTHORIZED_ACCESS;
    security_event.severity = "HIGH";
    setDefaultAuditContext([[maybe_unused]] security_event);
    security_event.violation_type = "PERMISSION_DENIED";
    security_event.description = "Scheduler permission check denied operation";
    security_event.details = {
        {"operation", operation},
        {"required_permission", required_permission},
        {"reason", reason},
        {"justification", TaskScheduler::currentAuthorizationJustification("none")}
    };
    security_event.blocked = true;
    security_event.action_taken = "operation_blocked";
    audit_manager->logSecurityEvent([[maybe_unused]] security_event);
}

// Helper function to convert trigger type to string
static std::string getTriggerTypeString(ScheduledTask::TriggerType type) {
    switch (type) {
        case ScheduledTask::TriggerType::CRON: return "CRON";
        case ScheduledTask::TriggerType::CDC_EVENT: return "CDC";
        case ScheduledTask::TriggerType::INTERVAL: return "INTERVAL";
        case ScheduledTask::TriggerType::MANUAL: return "MANUAL";
        case ScheduledTask::TriggerType::WEBHOOK: return "WEBHOOK";
        default: return "UNKNOWN";
    }
}

// ===== Retry Policy Helpers =====

namespace {

/**
 * @brief Compute the delay before the next retry attempt.
 *
 * @param policy  The task's RetryPolicy.
 * @param attempt 0-based retry index (0 = first retry, after the initial failure).
 * @return Delay in milliseconds, clamped to policy.max_delay.
 */
std::chrono::milliseconds computeRetryDelay(const ScheduledTask::RetryPolicy& policy,
                                             size_t attempt) {
    double base_ms = static_cast<double>(policy.initial_delay.count());
    double delay_ms = base_ms;

    switch (policy.strategy) {
        case ScheduledTask::RetryStrategy::NONE:
        [[fallthrough]];\n        case ScheduledTask::RetryStrategy::FIXED_DELAY:
            delay_ms = base_ms;
            break;

        case ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF:
            // delay = initial * multiplier^attempt
            for (size_t i = 0; i < attempt; ++i) {
                delay_ms *= policy.backoff_multiplier;
            }
            break;

        case ScheduledTask::RetryStrategy::LINEAR_BACKOFF:
            // delay = initial * (attempt + 1)
            delay_ms = base_ms * static_cast<double>(attempt + 1);
            break;

        case ScheduledTask::RetryStrategy::JITTER_BACKOFF: {
            // Exponential base + uniform random jitter in [-jitter, +jitter]
            for (size_t i = 0; i < attempt; ++i) {
                delay_ms *= policy.backoff_multiplier;
            }
            // Thread-local RNG to avoid contention
            thread_local std::mt19937 rng{std::random_device{}()};
            double jitter_range = delay_ms * policy.jitter_factor;
            std::uniform_real_distribution<double> dist(-jitter_range, jitter_range);
            delay_ms += dist(rng);
            if (delay_ms < 0.0) delay_ms = 0.0;
            break;
        }

        case ScheduledTask::RetryStrategy::FIBONACCI_BACKOFF: {
            // delay = initial * fib(attempt + 1)
            // fib(1)=1, fib(2)=1, fib(3)=2, fib(4)=3, fib(5)=5, ...
            // Loop invariant: after k iterations, a = fib(k), b = fib(k+1).
            // Starting with a=1,b=1 (= fib(1),fib(2)) and running (attempt) iterations
            // yields a = fib(attempt+1).
            // Computed iteratively to avoid recursion overhead.
            size_t a = 1, b = 1;
            for (size_t i = 1; i <= attempt; ++i) {
                size_t c = a + b;
                a = b;
                b = c;
            }
            delay_ms = base_ms * static_cast<double>(a);
            break;
        }
    }

    // Clamp to max_delay
    double max_ms = static_cast<double>(policy.max_delay.count());
    if (delay_ms > max_ms) delay_ms = max_ms;

    return std::chrono::milliseconds(static_cast<int64_t>(delay_ms));
}

/**
 * @brief Build an effective RetryPolicy from a ScheduledTask.
 *
 * If the task has an explicit retry_policy, use it.
 * Otherwise fall back to a policy derived from the legacy max_retries field.
 */
ScheduledTask::RetryPolicy effectiveRetryPolicy(const ScheduledTask& task) {
    if (task.retry_policy) {
        return *task.retry_policy;
    }
    // Legacy fallback: exponential backoff with max_retries
    ScheduledTask::RetryPolicy p;
    p.strategy       = ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF;
    p.max_retries    = task.max_retries;
    p.initial_delay  = std::chrono::milliseconds{1000};
    p.max_delay      = std::chrono::milliseconds{30000};
    p.backoff_multiplier = 2.0;
    return p;
}

/**
 * @brief Apply SLO-based retry adaptation to a computed delay and max-attempts.
 *
 * When the task has both an SloRetryConfig and an sla_deadline, this function:
 *  - Clamps @p delay_ms to the remaining SLA budget fraction.
 *  - Returns false (skip retry) if no SLA budget remains.
 *  - Reduces @p effective_max_retries to min_retries_under_pressure when the
 *    rolling SLO compliance rate (tracked on the task) is below the threshold.
 *
 * @param task               The scheduled task (provides sla_deadline and SLO state).
 * @param elapsed_ms         Time already spent since task execution began (ms).
 * @param delay_ms           [in/out] Computed retry delay – may be clamped.
 * @param effective_max_retries [in/out] Max attempts – may be clamped.
 * @return true  – retry is permitted (possibly with an adjusted delay).
 * @return false – retry should be skipped (SLA budget exhausted).
 */
bool applySloAdaptation(const ScheduledTask& task,
                        double elapsed_ms,
                        double& delay_ms,
                        size_t& effective_max_retries) {
    // No adaptation without both an SloRetryConfig and a deadline.
    if (!task.slo_retry_config.has_value() || !task.sla_deadline.has_value()) {
        return true;
    }
    const auto& slo = *task.slo_retry_config;
    if (!slo.slo_aware) {
        return true;
    }

    const double deadline_ms = static_cast<double>(task.sla_deadline->count());

    // 1. If SLA budget is already exhausted, skip further retries.
    if (elapsed_ms >= deadline_ms) {
        return false;
    }

    // 2. Clamp retry delay to the remaining SLA budget fraction.
    const double budget_ms = deadline_ms * slo.slo_budget_fraction;
    const double remaining_budget_ms = budget_ms - elapsed_ms;
    if (remaining_budget_ms <= 0.0) {
        return false;  // Budget spent; do not retry
    }
    if (delay_ms > remaining_budget_ms) {
        delay_ms = remaining_budget_ms;
    }
    if (delay_ms < 0.0) {
        delay_ms = 0.0;
    }

    // 3. Reduce max retries when SLO compliance is below threshold.
    if (slo.slo_history_window > 0 && task.slo_window_count >= slo.slo_history_window) {
        const double compliance =
            1.0 - (static_cast<double>(task.slo_violations) /
                   static_cast<double>(task.slo_window_count));
        if (compliance < slo.slo_compliance_threshold) {
            if (effective_max_retries > 1 + slo.min_retries_under_pressure) {
                effective_max_retries = 1 + slo.min_retries_under_pressure;
            }
        }
    }

    return true;
}

} // anonymous namespace

// ── Error categorization helper ──────────────────────────────────────────────
// Classify a failure message into one of the ScheduledTask::ErrorCategory values.
// This function is intentionally conservative: when in doubt it returns TRANSIENT
// so that the retry policy is not prematurely abandoned.
static ScheduledTask::ErrorCategory categorizeError(const std::string& error_message) {
    // Permanent errors: configuration / code problems that won't fix themselves
    if (error_message.find("not found") != std::string::npos ||
        error_message.find("unknown function") != std::string::npos ||
        error_message.find("invalid argument") != std::string::npos ||
        error_message.find("syntax error") != std::string::npos ||
        error_message.find("parse error") != std::string::npos ||
        error_message.find("no such") != std::string::npos) {
        return ScheduledTask::ErrorCategory::PERMANENT;
    }
    // Timeout errors
    if (error_message.find("timeout") != std::string::npos ||
        error_message.find("timed out") != std::string::npos ||
        error_message.find("deadline exceeded") != std::string::npos) {
        return ScheduledTask::ErrorCategory::TIMEOUT;
    }
    // Security / authorisation errors
    if (error_message.find("security") != std::string::npos ||
        error_message.find("injection") != std::string::npos ||
        error_message.find("forbidden") != std::string::npos ||
        error_message.find("unauthorized") != std::string::npos ||
        error_message.find("permission denied") != std::string::npos) {
        return ScheduledTask::ErrorCategory::SECURITY;
    }
    // Resource / rate-limit errors
    if (error_message.find("rate limit") != std::string::npos ||
        error_message.find("resource limit") != std::string::npos ||
        error_message.find("quota") != std::string::npos ||
        error_message.find("out of memory") != std::string::npos ||
        error_message.find("too many") != std::string::npos) {
        return ScheduledTask::ErrorCategory::RESOURCE;
    }
    // Default: treat as transient (connection issues, temporary failures, etc.)
    return ScheduledTask::ErrorCategory::TRANSIENT;
}



TaskScheduler::TaskScheduler(QueryEngine* query_engine, const Config& config, 
                             Changefeed* changefeed, std::shared_ptr<utils::AuditLogger> audit_logger,
                             RocksDBWrapper* result_storage)
        : query_engine_(query_engine), changefeed_(changefeed),
            audit_logger_(audit_logger.get()), config_(config)
    , dynamic_limit_(config.max_concurrent_tasks) {
    if (!query_engine_) {
        throw std::invalid_argument("TaskScheduler: query_engine cannot be null");
    }
    
    // Initialize audit manager if audit logging is enabled
    if (config_.enable_audit_logging) {
        scheduler::TaskAuditConfig audit_config;
        audit_config.enable_audit_logging = config_.enable_audit_logging;
        audit_config.enable_anomaly_detection = config_.enable_anomaly_detection;
        audit_config.enable_gdpr_mode = config_.enable_gdpr_mode;
        if (!config_.audit_log_path.empty()) {
            audit_config.audit_log_path = config_.audit_log_path;
        }
        
        audit_manager_ = std::make_shared<scheduler::TaskAuditManager>(
            audit_logger, audit_config);
        
        THEMIS_INFO("TaskScheduler audit logging enabled (anomaly_detection={}, gdpr_mode={})",
                   config_.enable_anomaly_detection, config_.enable_gdpr_mode);
    }
    
    // Initialize event trigger manager if changefeed is provided
    if (changefeed_) {
        event_trigger_manager_ = std::make_unique<EventTriggerManager>([[maybe_unused]] changefeed_);
        THEMIS_INFO([[maybe_unused]] "TaskScheduler initialized with CDC event support");
    } else {
        THEMIS_INFO([[maybe_unused]] "TaskScheduler initialized without CDC event support");
    }

    // Initialize result store if enabled and storage is provided
    if (config_.enable_result_store) {
        if (result_storage) {
            result_store_ = std::make_unique<scheduler::TaskResultStore>(
                *result_storage, config_.result_store_max_results_per_task);
            THEMIS_INFO("TaskScheduler result store enabled (max_per_task={})",
                        config_.result_store_max_results_per_task);
        } else {
            THEMIS_WARN("TaskScheduler: enable_result_store=true but no result_storage provided; "
                        "result store is disabled");
        }
    }
    
    if (config_.persist_tasks) {
        loadTasks();
    }
}

TaskScheduler::~TaskScheduler() {
    stop();
}

// ===== Lifecycle =====

void TaskScheduler::start() {
    size_t task_count = 0;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        
        if (running_.load()) {
            THEMIS_WARN("TaskScheduler already running");
            return;
        }
        
        running_.store(true);
        scheduler_thread_ = std::thread(&TaskScheduler::schedulerLoop, this);
        task_count = tasks_.size();
    }

    // Restart any event triggers that were stopped (e.g. after a previous stop()).
    // Called outside tasks_mutex_ to avoid lock inversion with the trigger callback.
    if ([[maybe_unused]] event_trigger_manager_) {
        event_trigger_manager_->startAll();
    }
    
    THEMIS_INFO("TaskScheduler started with {} tasks, check interval: {}s",
                task_count, 
                config_.check_interval.count() / 1000);
}

void TaskScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    
    cv_.notify_all();
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    // Wait for running tasks to complete (with timeout)
    auto timeout = std::chrono::seconds(30);
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (active_task_threads_.load() == 0) {
            break;
        }
        
        if (std::chrono::steady_clock::now() - start > timeout) {
            THEMIS_WARN("TaskScheduler: Timeout waiting for tasks to complete");
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    
    // Join remaining threads
    {
        std::lock_guard<std::mutex> lock(running_mutex_);
        for (auto& [id, thread] : running_task_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        running_task_threads_.clear();
    }

    // Stop event triggers to prevent CDC-triggered execution after stop()
    if ([[maybe_unused]] event_trigger_manager_) {
        event_trigger_manager_->stopAll();
    }
    
    if (config_.persist_tasks) {
        saveTasks();
    }
    
    THEMIS_INFO("TaskScheduler stopped. Total executions: {}, Failed: {}",
                total_executions_.load(), failed_executions_.load());
}

// ===== Task Management =====

std::string TaskScheduler::registerTask(const ScheduledTask& task) {
    if (!hasPermission("task:register")) {
        const std::string reason = "missing permission 'task:register'";
        logUnauthorizedPermissionAttempt(
            audit_manager_, task.id, task.name, "task:register", "registerTask", reason);
        throw std::runtime_error(
            "Unauthorized: Missing required permission 'task:register' for registerTask");
    }
    
    // Validate AQL query for SQL injection patterns
    if (task.type == ScheduledTask::TaskType::AQL_QUERY) {
        validateAqlQuery(task.aql_query);
    }
    
    // Validate resource limits (timeout, max_retries)
    validateResourceLimits(task);
    
    // Validate trigger-specific configuration
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        validateCronExpression(task.cron_expression);
    } else if ([[maybe_unused]] task.trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        validateCDCTrigger(task.cdc_trigger);
        if (!changefeed_) {
            throw std::invalid_argument([[maybe_unused]] "CDC event triggers require a Changefeed instance");
        }
    }
    
    // Sanitize task parameters
    auto sanitized_task = sanitizeTask(task);
    
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    std::string id = sanitized_task.id;
    if (id.empty()) {
        id = generateTaskId(sanitized_task);
    }
    
    // ========================================================================
    // HARDENING: Enforce idempotent registration (Phase 2-3 contract)
    // ========================================================================
    // If a task with this ID already exists, verify byte-identical registration.
    // If descriptors differ, reject the registration with kTaskAlreadyExists error.
    auto existing_it = tasks_.find(id);
    if (existing_it != tasks_.end()) {
        const auto& existing_task = existing_it->second;
        
        // Compare critical fields for byte-identical check.
        // Use the resolved `id` (which includes auto-generated IDs) rather than
        // `sanitized_task.id`, which is empty when the ID was auto-generated.
        bool identical = (
            existing_task->id == id &&
            existing_task->name == sanitized_task.name &&
            existing_task->description == sanitized_task.description &&
            existing_task->type == sanitized_task.type &&
            existing_task->aql_query == sanitized_task.aql_query &&
            existing_task->function_name == sanitized_task.function_name &&
            existing_task->parameters == sanitized_task.parameters &&
            existing_task->trigger_type == sanitized_task.trigger_type &&
            existing_task->interval == sanitized_task.interval &&
            existing_task->cron_expression == sanitized_task.cron_expression &&
            existing_task->timeout == sanitized_task.timeout &&
            existing_task->max_retries == sanitized_task.max_retries &&
            existing_task->allow_concurrent == sanitized_task.allow_concurrent
        );
        
        if (!identical) {
            // Conflicting re-registration — fail with kTaskAlreadyExists
            THEMIS_WARN("Rejected re-registration of task {} with conflicting descriptor", id);
            
            if (audit_manager_) {
                scheduler::TaskAuditEvent event;
                event.uuid = scheduler::generateUUID();
                event.timestamp = std::chrono::system_clock::now();
                event.task_id = id;
                event.task_name = sanitized_task.name;
                event.event_type = scheduler::TaskEventType::TASK_REGISTRATION_REJECTED;
                event.success = false;
                event.error_message = "Conflicting task descriptor for existing task ID";
                setDefaultAuditContext([[maybe_unused]] event);
                audit_manager_->logAuditEvent([[maybe_unused]] event);
            }
            
            throw std::runtime_error(
                "Task registration conflict: task '" + id + "' already exists with "
                "a different descriptor. Cannot re-register with conflicting configuration.");
        } else {
            // Idempotent: identical re-registration is allowed
            THEMIS_DEBUG("Idempotent re-registration of task {} accepted", id);
            return id;
        }
    }
    
    auto task_ptr = std::make_shared<ScheduledTask>(sanitized_task);
    task_ptr->id = id;
    
    // Initialize next_run based on trigger type
    if (task_ptr->trigger_type == ScheduledTask::TriggerType::CRON) {
        // Parse cron expression and calculate next run
        updateCronExpression(id, task_ptr->cron_expression);
        auto cron = getCronExpression(id);
        if (cron) {
            auto next = cron->getNextExecution(std::chrono::system_clock::now());
            if (next) {
                task_ptr->next_run = *next;
            }
        }
    } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        // Traditional interval-based scheduling
        if (task_ptr->next_run == std::chrono::system_clock::time_point{}) {
            task_ptr->next_run = std::chrono::system_clock::now() + task_ptr->interval;
        }
    } else if ([[maybe_unused]] task_ptr->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        // Setup CDC event trigger
        setupEventTrigger([[maybe_unused]] task_ptr);
    }
    
    tasks_[id] = task_ptr;
    
    THEMIS_INFO("Registered task: {} (name={}, trigger_type={})",
                id, sanitized_task.name,
                static_cast<int>(sanitized_task.trigger_type));
    
    // Log audit event for task registration
    if (audit_manager_) {
        scheduler::TaskAuditEvent event;
        event.uuid = scheduler::generateUUID();
        event.timestamp = std::chrono::system_clock::now();
        event.task_id = id;
        event.task_name = sanitized_task.name;
        event.task_description = sanitized_task.description;
        event.event_type = scheduler::TaskEventType::TASK_REGISTERED;
        event.trigger_type = getTriggerTypeString([[maybe_unused]] sanitized_task.trigger_type);
        event.success = true;
        setDefaultAuditContext([[maybe_unused]] event);
        event.metadata["cron_expression"] = sanitized_task.cron_expression;
        event.metadata["interval_ms"] = sanitized_task.interval.count();
        
        audit_manager_->logAuditEvent([[maybe_unused]] event);
    }
    
    if (config_.persist_tasks) {
        saveTasks();
    }
    
    return id;
}

void TaskScheduler::unregisterTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        // Remove CDC event trigger if present
        if ([[maybe_unused]] it->second->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
            removeEventTrigger([[maybe_unused]] task_id);
        }
        
        // Remove cron expression from cache
        cron_expressions_.erase(task_id);
        
        THEMIS_INFO("Unregistered task: {}", task_id);
        
        // Audit log task unregistration
        if (config_.enable_audit_logging && audit_logger_) {
            nlohmann::json details = {
                {"task_name", it->second->name},
                {"total_executions", it->second->total_executions},
                {"successful_executions", it->second->successful_executions},
                {"failed_executions", it->second->failed_executions}
            };
            
            audit_logger_->logTaskSchedulerEvent(
                utils::SecurityEventType::TASK_UNREGISTERED,
                task_id,
                TaskScheduler::currentUserId(DEFAULT_AUDIT_USER),
                details
            );
        }
        
        tasks_.erase(it);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::enableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second->enabled = true;
        THEMIS_INFO("Enabled task: {}", task_id);
        
        // Audit log task enable
        if (config_.enable_audit_logging && audit_logger_) {
            nlohmann::json details = {
                {"task_name", it->second->name}
            };
            
            audit_logger_->logTaskSchedulerEvent(
                utils::SecurityEventType::TASK_ENABLED,
                task_id,
                TaskScheduler::currentUserId(DEFAULT_AUDIT_USER),
                details
            );
        }
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::disableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second->enabled = false;
        THEMIS_INFO("Disabled task: {}", task_id);
        
        // Audit log task disable
        if (config_.enable_audit_logging && audit_logger_) {
            nlohmann::json details = {
                {"task_name", it->second->name}
            };
            
            audit_logger_->logTaskSchedulerEvent(
                utils::SecurityEventType::TASK_DISABLED,
                task_id,
                TaskScheduler::currentUserId(DEFAULT_AUDIT_USER),
                details
            );
        }
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::updateTask(const ScheduledTask& task) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task.id);
    if (it != tasks_.end()) {
        // Keep statistics and state
        auto old_stats = *it->second;
        *it->second = task;
        it->second->total_executions = old_stats.total_executions;
        it->second->successful_executions = old_stats.successful_executions;
        it->second->failed_executions = old_stats.failed_executions;
        it->second->avg_execution_time_ms = old_stats.avg_execution_time_ms;
        it->second->last_run_ms = old_stats.last_run_ms;
        it->second->running = old_stats.running;
        
        THEMIS_INFO("Updated task: {}", task.id);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

// ===== Manual Execution =====

nlohmann::json TaskScheduler::executeTaskNow(const std::string& task_id) {
    if (!hasPermission("task:execute")) {
        const std::string reason = "missing permission 'task:execute'";
        logUnauthorizedPermissionAttempt(
            audit_manager_, task_id, task_id, "task:execute", "executeTaskNow", reason);
        return nlohmann::json{{"error", "Unauthorized: Missing required permission 'task:execute'"}};
    }
    
    // Log execution attempt for audit trail
    THEMIS_INFO("Manual task execution requested: task_id={}", task_id);
    
    // Check rate limiting to prevent abuse
    if (!checkRateLimit(task_id)) {
        THEMIS_WARN("Rate limit exceeded for task execution: task_id={}", task_id);
        return nlohmann::json{{"error", "Rate limit exceeded. Please try again later."}};
    }
    
    auto span = Tracer::startSpan("TaskScheduler.executeTaskNow");
    span.setAttribute("task_id", task_id);
    
    std::shared_ptr<ScheduledTask> task;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        auto it = tasks_.find(task_id);
        if (it == tasks_.end()) {
            THEMIS_WARN("Cannot execute unknown task: {}", task_id);
            return nlohmann::json{{"error", "Task not found"}};
        }
        task = it->second;
    }
    
    // ========================================================================
    // HARDENING: Execution Serialization (Phase 3 contract)
    // ========================================================================
    // Acquire per-task execution lock to serialize concurrent runs of the same task.
    // This ensures that only one instance of each task runs at a time.
    std::mutex& task_exec_lock = getTaskExecutionLock(task_id);
    std::lock_guard<std::mutex> exec_lock(task_exec_lock);
    
    // ========================================================================
    // HARDENING: Fail-Closed Coordination Check (Phase 3 contract)
    // ========================================================================
    // If coordination layer is unavailable (future: distributed task coordination),
    // fail-closed by not dispatching the task execution.
    // For now, this is a structural placeholder for coordination availability check.
    // When distributed_task_coordinator_ is added, uncomment:
    //   if (!coordinator_ || !coordinator_->isHealthy()) {
    //       THEMIS_ERROR("Coordination layer unavailable; failing closed for task {}",
    //                    task_id);
    //       span.setAttribute("coordination_error", true);
    //       return nlohmann::json{{"error", "Coordination layer unavailable"}};
    //   }
    
    // Audit log manual task execution trigger
    if (config_.enable_audit_logging && audit_logger_) {
        nlohmann::json details = {
            {"task_name", task->name},
            {"trigger_type", "MANUAL"}
        };
        
        audit_logger_->logTaskSchedulerEvent(
            utils::SecurityEventType::TASK_MANUAL_TRIGGERED,
            task_id,
            TaskScheduler::currentUserId(DEFAULT_AUDIT_USER),
            details
        );
    }
    
    auto start_time = std::chrono::steady_clock::now();

    // Dispatching via executeTaskNow — reset the aging counter.
    task->consecutive_skips = 0;

    // Execute synchronously with retry logic (same as scheduled execution)
    const ScheduledTask::RetryPolicy policy = effectiveRetryPolicy(*task);
    const size_t base_max_attempts = (policy.strategy == ScheduledTask::RetryStrategy::NONE)
                                         ? 1
                                         : 1 + policy.max_retries;
    size_t max_attempts = base_max_attempts;  // may be clamped by SLO adaptation
    std::string last_error;
    bool succeeded = false;
    nlohmann::json result;
    size_t attempts_made = 0;

    for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
        if (attempt > 0) {
            double delay_ms = static_cast<double>(computeRetryDelay(policy, attempt - 1).count());

            // SLO-based adaptive retry for manual execution
            {
                auto now_elapsed = std::chrono::steady_clock::now();
                double elapsed_so_far =
                    std::chrono::duration<double, std::milli>(now_elapsed - start_time).count();
                if (!applySloAdaptation(*task, elapsed_so_far, delay_ms, max_attempts)) {
                    THEMIS_INFO("executeTaskNow: task {} SLO budget exhausted after {:.0f}ms; "
                                "skipping retries",
                                task_id, elapsed_so_far);
                    break;
                }
            }

            THEMIS_INFO("executeTaskNow: retrying task {} (attempt {}/{}) after {}ms "
                        "[strategy={}]",
                        task_id, attempt + 1, max_attempts,
                        static_cast<int64_t>(delay_ms),
                        static_cast<int>(policy.strategy));
            if (delay_ms > 0.0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int64_t>(delay_ms)));
            }
        }

        ++attempts_made;
        try {
            if (task->type == ScheduledTask::TaskType::AQL_QUERY) {
                result = executeAqlQuery(task->aql_query);
            } else {
                result = executeFunction(task->function_name, task->parameters);
            }
            succeeded = true;
            break;
        } catch (const std::exception& e) {
            last_error = e.what();
            // Check conditional should_retry if provided
            if (policy.should_retry && !policy.should_retry(last_error)) {
                THEMIS_INFO("executeTaskNow task {} retry skipped by should_retry: {}",
                            task_id, last_error);
                break;
            }
            if (attempt + 1 < max_attempts) {
                THEMIS_WARN("executeTaskNow task {} attempt {}/{} failed: {} (will retry)",
                            task_id, attempt + 1, max_attempts, e.what());
            }
        } catch (...) {
            last_error = "unknown non-exception thrown";
            THEMIS_ERROR("executeTaskNow task {} threw non-exception type", task_id);
            break;  // Non-std exceptions are never retried
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    if (succeeded) {
        task->successful_executions++;
        task->last_success_time = std::chrono::system_clock::now();
        task->last_error_category = ScheduledTask::ErrorCategory::NONE;  // Reset on success

        if (task->on_success) {
            task->on_success(task_id, result);
        }

        // Resolve any active failure alert for this task
        resolveTaskFailureAlert(task_id);
    } else {
        result = nlohmann::json{{"error", last_error}};
        task->failed_executions++;
        task->last_error = last_error;
        task->last_error_category = categorizeError(last_error);
        task->last_failure_time = std::chrono::system_clock::now();

        if (task->on_failure) {
            task->on_failure(task_id, last_error);
        }

        // Fire failure alert
        fireTaskFailureAlert(*task, last_error);

        span.recordError(last_error);
    }

    task->total_executions++;
    total_executions_++;

    // Check SLA breach (applies regardless of success/failure)
    if (task->sla_deadline.has_value() &&
        elapsed_ms > static_cast<double>(task->sla_deadline->count())) {
        fireTaskSlaBreachAlert(*task, elapsed_ms);
    }

    // SLO compliance tracking (Phase 5: SLO-based adaptive retry).
    if (task->slo_retry_config.has_value() && task->sla_deadline.has_value()) {
        const auto& slo = *task->slo_retry_config;
        if (slo.slo_aware && slo.slo_history_window > 0) {
            const bool violated = elapsed_ms >
                static_cast<double>(task->sla_deadline->count());
            if (task->slo_window_count >= slo.slo_history_window) {
                const double ratio =
                    static_cast<double>(task->slo_violations) /
                    static_cast<double>(task->slo_window_count);
                task->slo_window_count  = slo.slo_history_window / 2;
                task->slo_violations    =
                    static_cast<size_t>(ratio * static_cast<double>(task->slo_window_count));
            }
            ++task->slo_window_count;
            if (violated) {
                ++task->slo_violations;
            }
        }
    }


    if (result_store_) {
        scheduler::TaskExecutionResult exec_result;
        exec_result.task_id      = task->id;
        exec_result.task_name    = task->name;
        exec_result.timestamp_ms = getCurrentTimeMs();
        exec_result.duration_ms  = elapsed_ms;
        exec_result.success      = succeeded;
        exec_result.output       = succeeded ? result : nlohmann::json{};
        exec_result.error        = succeeded ? "" : last_error;
        result_store_->store(exec_result);
    }
    
    return result;
}

// ===== DAG Execution =====

std::vector<std::string> TaskScheduler::topologicalSort(
    const std::vector<std::string>& task_ids,
    const std::map<std::string, std::vector<std::string>>& adj) const
{
    // Kahn's algorithm: compute in-degrees, then process nodes with zero in-degree.
    std::map<std::string, int> in_degree;
    for (const auto& id : task_ids) {
        in_degree[id] = 0;
    }
    for (const auto& [id, deps] : adj) {
        in_degree[id] += static_cast<int>(deps.size());
    }

    // Queue nodes with no dependencies
    std::deque<std::string> ready;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) {
            ready.push_back(id);
        }
    }

    // Build reverse adjacency: for each node, which nodes depend on it
    std::map<std::string, std::vector<std::string>> dependents;
    for (const auto& [id, deps] : adj) {
        for (const auto& dep : deps) {
            dependents[dep].push_back(id);
        }
    }

    std::vector<std::string> order;
    order.reserve(task_ids.size());

    while (!ready.empty()) {
        std::string cur = ready.front();
        ready.pop_front();
        order.push_back(cur);

        auto it = dependents.find(cur);
        if (it != dependents.end()) {
            for (const auto& dependent : it->second) {
                if (--in_degree[dependent] == 0) {
                    ready.push_back(dependent);
                }
            }
        }
    }

    if (order.size() != task_ids.size()) {
        throw std::runtime_error(
            "TaskScheduler::executeDAG: dependency graph contains a cycle");
    }

    return order;
}

TaskScheduler::DagExecutionResult TaskScheduler::executeDAG(
    const std::vector<std::string>& task_ids)
{
    if (!hasPermission("task:execute")) {
        const std::string reason = "missing permission 'task:execute'";
        logUnauthorizedPermissionAttempt(
            audit_manager_, "<dag>", "<dag>", "task:execute", "executeDAG", reason);
        throw std::runtime_error("Unauthorized: Missing required permission 'task:execute'");
    }

    if (task_ids.empty()) {
        return {};
    }

    // Build set of requested IDs for O(1) lookup
    std::set<std::string> id_set(task_ids.begin(), task_ids.end());

    // Resolve tasks and validate all IDs exist
    std::map<std::string, std::shared_ptr<ScheduledTask>> task_map;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        for (const auto& id : task_ids) {
            auto it = tasks_.find(id);
            if (it == tasks_.end()) {
                throw std::invalid_argument(
                    "TaskScheduler::executeDAG: unknown task id '" + id + "'");
            }
            task_map[id] = it->second;
        }
    }

    // Build adjacency: task -> list of its deps that are within the requested set.
    // adj[task] = [dep1, dep2, ...] means task depends on dep1, dep2, ...
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& id : task_ids) {
        adj[id] = {};
        for (const auto& dep : task_map[id]->dependencies) {
            if (id_set.count(dep)) {
                adj[id].push_back(dep);
            }
        }
    }

    // Determine execution order via topological sort
    std::vector<std::string> order = topologicalSort(task_ids, adj);

    // Build reverse adjacency for propagating failures (dep -> [dependents])
    std::map<std::string, std::vector<std::string>> dependents;
    for (const auto& [id, deps] : adj) {
        for (const auto& dep : deps) {
            dependents[dep].push_back(id);
        }
    }

    DagExecutionResult result;
    std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
    std::set<std::string> condition_skipped_set; // Tasks skipped by branch_condition

    // Execute wave by wave: gather tasks whose dependencies are all done,
    // run them in parallel, then repeat.
    std::set<std::string> completed;  // succeeded tasks
    std::set<std::string> processed;  // all tasks we have decided about

    while (processed.size() < task_ids.size()) {
        // Collect tasks that are ready (all deps completed successfully)
        std::vector<std::string> wave;
        for (const auto& id : order) {
            if (processed.count(id)) {
                continue;
            }
            // Check if it should be condition-skipped
            if (condition_skipped_set.count(id)) {
                result.condition_skipped.push_back(id);
                processed.insert(id);
                // Propagate condition-skip to dependents
                auto dit = dependents.find(id);
                if (dit != dependents.end()) {
                    for (const auto& dep : dit->second) {
                        condition_skipped_set.insert(dep);
                    }
                }
                continue;
            }
            // Check if it should be skipped due to a failed dep
            if (failed_or_skipped.count(id)) {
                result.skipped.push_back(id);
                processed.insert(id);
                // Propagate skip to dependents
                auto dit = dependents.find(id);
                if (dit != dependents.end()) {
                    for (const auto& dep : dit->second) {
                        failed_or_skipped.insert(dep);
                    }
                }
                continue;
            }
            // Check if all deps are in completed
            bool deps_ready = true;
            for (const auto& dep : adj[id]) {
                if (!completed.count(dep)) {
                    deps_ready = false;
                    break;
                }
            }
            if (deps_ready) {
                // Evaluate branch_condition if set
                auto& task = task_map[id];
                if (task->branch_condition) {
                    // Build dep_results map from succeeded dependency results
                    std::map<std::string, nlohmann::json> dep_results;
                    for (const auto& dep : adj[id]) {
                        auto it = result.succeeded.find(dep);
                        if (it != result.succeeded.end()) {
                            dep_results[dep] = it->second;
                        }
                    }
                    if (!task->branch_condition(dep_results)) {
                        // Condition not met – condition-skip this task
                        result.condition_skipped.push_back(id);
                        processed.insert(id);
                        condition_skipped_set.insert(id);
                        // Propagate condition-skip to direct dependents
                        auto dit = dependents.find(id);
                        if (dit != dependents.end()) {
                            for (const auto& dep : dit->second) {
                                condition_skipped_set.insert(dep);
                            }
                        }
                        continue;
                    }
                }
                wave.push_back(id);
            }
        }

        if (wave.empty()) {
            // No progress possible – all remaining tasks must have already been
            // processed as condition-skipped, failure-skipped, or have unresolvable
            // deps (should not occur with a cycle-free graph).
            for (const auto& id : order) {
                if (!processed.count(id)) {
                    result.skipped.push_back(id);
                    processed.insert(id);
                }
            }
            break;
        }

        // Execute wave tasks in parallel
        struct WaveResult {
            std::string id;
            bool succeeded = false;
            nlohmann::json result;
            std::string error;
            std::string error_type;
        };
        std::vector<WaveResult> wave_results(wave.size());
        std::vector<std::thread> threads;
        threads.reserve(wave.size());

        for (size_t i = 0; i < wave.size(); ++i) {
            wave_results[i].id = wave[i];
            threads.emplace_back([this, &wave_results, i, &task_map]() {
                const auto& id = wave_results[i].id;
                auto task = task_map[id];
                const int64_t exec_start_ms = getCurrentTimeMs();
                const auto exec_start = std::chrono::steady_clock::now();

                // Log TASK_STARTED audit event
                if (audit_manager_) {
                    scheduler::TaskAuditEvent start_event;
                    start_event.uuid = scheduler::generateUUID();
                    start_event.timestamp = std::chrono::system_clock::now();
                    start_event.task_id = task->id;
                    start_event.task_name = task->name;
                    start_event.task_description = task->description;
                    start_event.event_type = scheduler::TaskEventType::TASK_STARTED;
                    start_event.trigger_type = getTriggerTypeString([[maybe_unused]] task->trigger_type);
                    setDefaultAuditContext([[maybe_unused]] start_event);
                    audit_manager_->logAuditEvent([[maybe_unused]] start_event);
                }

                try {
                    nlohmann::json r;
                    if (task->type == ScheduledTask::TaskType::AQL_QUERY) {
                        r = executeAqlQuery(task->aql_query);
                    } else {
                        r = executeFunction(task->function_name, task->parameters);
                    }
                    wave_results[i].succeeded = true;
                    wave_results[i].result = std::move(r);
                } catch (const std::exception& e) {
                    task->last_error = e.what();
                    task->last_error_category = categorizeError(e.what());
                    wave_results[i].error = e.what();
                    wave_results[i].error_type = "EXECUTION_ERROR";
                } catch (...) {
                    task->last_error = "unknown non-exception thrown";
                    task->last_error_category = ScheduledTask::ErrorCategory::TRANSIENT;
                    wave_results[i].error = "unknown non-exception thrown";
                    wave_results[i].error_type = "UNKNOWN_ERROR";
                }

                // Compute duration once after execution (for stats, audit, SLA, result store).
                const double dur_ms = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - exec_start).count();

                if (wave_results[i].succeeded) {
                    // Update task stats
                    task->total_executions++;
                    task->successful_executions++;
                    task->last_run_ms = exec_start_ms;
                    task->last_success_time = std::chrono::system_clock::now();
                    task->last_error_category = ScheduledTask::ErrorCategory::NONE;
                    task->avg_execution_time_ms =
                        (task->avg_execution_time_ms * (task->total_executions - 1) + dur_ms)
                        / task->total_executions;

                    // Log TASK_COMPLETED audit event
                    if (audit_manager_) {
                        scheduler::TaskAuditEvent completion_event;
                        completion_event.uuid = scheduler::generateUUID();
                        completion_event.timestamp = std::chrono::system_clock::now();
                        completion_event.duration_ms = dur_ms;
                        completion_event.task_id = task->id;
                        completion_event.task_name = task->name;
                        completion_event.task_description = task->description;
                        completion_event.event_type = scheduler::TaskEventType::TASK_COMPLETED;
                        completion_event.trigger_type = getTriggerTypeString([[maybe_unused]] task->trigger_type);
                        setDefaultAuditContext([[maybe_unused]] completion_event);
                        completion_event.success = true;
                        // cpu_time_ms is approximated by wall-clock time (same as executeTask)
                        completion_event.resource_usage.execution_time_ms = dur_ms;
                        completion_event.resource_usage.cpu_time_ms = dur_ms;
                        if (wave_results[i].result.is_object()) {
                            if (wave_results[i].result.contains("rows")) {
                                completion_event.resource_usage.result_rows =
                                    wave_results[i].result["rows"].get<uint64_t>();
                            }
                            if (wave_results[i].result.contains("affected")) {
                                completion_event.resource_usage.affected_rows =
                                    wave_results[i].result["affected"].get<uint64_t>();
                            }
                        }
                        audit_manager_->logAuditEvent([[maybe_unused]] completion_event);
                    }

                    if (task->on_success) {
                        task->on_success(id, wave_results[i].result);
                    }
                    resolveTaskFailureAlert(id);
                } else {
                    // Update failure stats
                    task->total_executions++;
                    task->failed_executions++;
                    task->last_failure_time = std::chrono::system_clock::now();

                    // Log TASK_FAILED audit event
                    if (audit_manager_) {
                        scheduler::TaskAuditEvent failure_event;
                        failure_event.uuid = scheduler::generateUUID();
                        failure_event.timestamp = std::chrono::system_clock::now();
                        failure_event.duration_ms = dur_ms;
                        failure_event.task_id = task->id;
                        failure_event.task_name = task->name;
                        failure_event.task_description = task->description;
                        failure_event.event_type = scheduler::TaskEventType::TASK_FAILED;
                        failure_event.trigger_type = getTriggerTypeString([[maybe_unused]] task->trigger_type);
                        setDefaultAuditContext([[maybe_unused]] failure_event);
                        failure_event.success = false;
                        failure_event.error_message = wave_results[i].error;
                        failure_event.error_type = wave_results[i].error_type;
                        // cpu_time_ms is approximated by wall-clock time (same as executeTask)
                        failure_event.resource_usage.execution_time_ms = dur_ms;
                        failure_event.resource_usage.cpu_time_ms = dur_ms;
                        audit_manager_->logAuditEvent([[maybe_unused]] failure_event);
                    }

                    if (task->on_failure) {
                        task->on_failure(id, wave_results[i].error);
                    }
                    fireTaskFailureAlert(*task, wave_results[i].error);
                }

                // Check SLA breach (applies regardless of success/failure)
                if (task->sla_deadline.has_value() &&
                    dur_ms > static_cast<double>(task->sla_deadline->count())) {
                    fireTaskSlaBreachAlert(*task, dur_ms);
                }

                if (result_store_) {
                    scheduler::TaskExecutionResult exec_result;
                    exec_result.task_id      = task->id;
                    exec_result.task_name    = task->name;
                    exec_result.timestamp_ms = exec_start_ms;
                    exec_result.duration_ms  = dur_ms;
                    exec_result.success      = wave_results[i].succeeded;
                    exec_result.output       = wave_results[i].succeeded
                                                   ? wave_results[i].result
                                                   : nlohmann::json{};
                    exec_result.error        = wave_results[i].error;
                    result_store_->store(exec_result);
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        total_executions_ += wave.size();

        // Collect wave results
        for (auto& wr : wave_results) {
            processed.insert(wr.id);
            if (wr.succeeded) {
                completed.insert(wr.id);
                result.succeeded[wr.id] = std::move(wr.result);
            } else {
                failed_executions_++;
                result.failed[wr.id] = wr.error;
                failed_or_skipped.insert(wr.id);
                // Propagate failure to direct dependents
                auto dit = dependents.find(wr.id);
                if (dit != dependents.end()) {
                    for (const auto& dep : dit->second) {
                        failed_or_skipped.insert(dep);
                    }
                }
            }
        }
    }

    return result;
}

// ===== Function Registration =====

void TaskScheduler::registerFunction(const std::string& name, TaskFunction func) {
    // ⚠️ SECURITY CRITICAL: This allows arbitrary code execution.
    if (!hasPermission("task:register_function") || !hasRole("system_admin")) {
        const std::string reason = !hasPermission("task:register_function")
            ? "missing permission 'task:register_function'"
            : "missing role 'system_admin'";
        logUnauthorizedPermissionAttempt(
            audit_manager_, name, name, "task:register_function", "registerFunction", reason);
        throw std::runtime_error(
            "Unauthorized: Missing 'task:register_function' permission and/or 'system_admin' role");
    }

    THEMIS_INFO("Function registration attempt: name={}", name);
    
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    functions_[name] = func;
    THEMIS_INFO("Registered task function: {}", name);
}

void TaskScheduler::unregisterFunction(const std::string& name) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    functions_.erase(name);
    THEMIS_INFO("Unregistered task function: {}", name);
}

// ===== Statistics =====

TaskScheduler::Stats TaskScheduler::getStats() const {
    // Acquire both locks atomically (canonical order tasks < running) to avoid
    // inversion against threads that acquire running_mutex_ alone.
    std::scoped_lock lock(tasks_mutex_, running_mutex_);
    
    Stats stats;
    stats.registered_tasks = tasks_.size();
    stats.active_tasks = std::count_if(tasks_.begin(), tasks_.end(),
        [](const auto& pair) { return pair.second->enabled; });
    
    stats.running_tasks = running_task_threads_.size();
    stats.total_executions = total_executions_.load();
    stats.failed_executions = failed_executions_.load();
    stats.last_run = last_run_;
    
    // Find next scheduled run
    auto next = std::chrono::system_clock::time_point::max();
    for (const auto& [id, task] : tasks_) {
        if (task->enabled && task->next_run < next) {
            next = task->next_run;
        }
    }
    stats.next_run = next;
    
    return stats;
}

std::string TaskScheduler::exportMetrics() const {
    std::ostringstream out;

    // Helper lambda for Prometheus text format (gauge metric with HELP + TYPE)
    auto write_gauge = [&](const std::string& name, const std::string& help,
                           double value) {
        out << "# HELP " << name << " " << help << "\n";
        out << "# TYPE " << name << " gauge\n";
        out << name << " " << value << "\n";
    };

    // Capture snapshot under the lock
    size_t registered_tasks, active_tasks, running_tasks;
    size_t total_exec, failed_exec;
    std::vector<ScheduledTask> task_snapshot;

    // Acquire both locks atomically (canonical order tasks < running).
    std::scoped_lock lock(tasks_mutex_, running_mutex_);
    registered_tasks = tasks_.size();
    active_tasks = std::count_if(tasks_.begin(), tasks_.end(),
                                 [](const auto& p) { return p.second->enabled; });
    running_tasks = running_task_threads_.size();
    total_exec   = total_executions_.load();
    failed_exec  = failed_executions_.load();

    task_snapshot.reserve(tasks_.size());
    for (const auto& [id, task] : tasks_) {
        task_snapshot.push_back(*task);
    }

    // ── Scheduler-level gauges ────────────────────────────────────────────
    write_gauge("themis_scheduler_tasks_registered",
                "Total number of registered scheduled tasks.",
                static_cast<double>(registered_tasks));

    write_gauge("themis_scheduler_tasks_active",
                "Number of enabled (active) scheduled tasks.",
                static_cast<double>(active_tasks));

    write_gauge("themis_scheduler_tasks_running",
                "Number of task instances currently executing.",
                static_cast<double>(running_tasks));

    // ── Scheduler-level counters ──────────────────────────────────────────
    out << "# HELP themis_scheduler_executions_total"
           " Total number of task execution attempts.\n";
    out << "# TYPE themis_scheduler_executions_total counter\n";
    out << "themis_scheduler_executions_total{status=\"success\"} "
        << (total_exec - failed_exec) << "\n";
    out << "themis_scheduler_executions_total{status=\"failure\"} "
        << failed_exec << "\n";

    // ── Per-task metrics ──────────────────────────────────────────────────
    if (!task_snapshot.empty()) {
        // Counters per task
        out << "# HELP themis_scheduler_task_executions_total"
               " Execution count per task.\n";
        out << "# TYPE themis_scheduler_task_executions_total counter\n";
        for (const auto& t : task_snapshot) {
            // Sanitize label values (replace " and \ which are illegal in labels)
            std::string safe_name = t.name;
            for (char& c : safe_name) {
                if (c == '"' || c == '\\' || c == '\n') c = '_';
            }
            std::string labels = "task_id=\"" + t.id + "\",task_name=\"" + safe_name + "\"";
            out << "themis_scheduler_task_executions_total{"
                << labels << ",status=\"success\"} "
                << t.successful_executions << "\n";
            out << "themis_scheduler_task_executions_total{"
                << labels << ",status=\"failure\"} "
                << t.failed_executions << "\n";
        }

        // Avg execution duration gauge per task
        out << "# HELP themis_scheduler_task_execution_duration_ms"
               " Moving-average execution duration per task in milliseconds.\n";
        out << "# TYPE themis_scheduler_task_execution_duration_ms gauge\n";
        for (const auto& t : task_snapshot) {
            std::string safe_name = t.name;
            for (char& c : safe_name) {
                if (c == '"' || c == '\\' || c == '\n') c = '_';
            }
            out << "themis_scheduler_task_execution_duration_ms"
                << "{task_id=\"" << t.id << "\",task_name=\"" << safe_name << "\"} "
                << t.avg_execution_time_ms << "\n";
        }

        // Last-run timestamp (unix epoch seconds) per task
        out << "# HELP themis_scheduler_task_last_run_timestamp_seconds"
               " Unix timestamp of the last execution for each task.\n";
        out << "# TYPE themis_scheduler_task_last_run_timestamp_seconds gauge\n";
        for (const auto& t : task_snapshot) {
            std::string safe_name = t.name;
            for (char& c : safe_name) {
                if (c == '"' || c == '\\' || c == '\n') c = '_';
            }
            double last_run_sec = 0.0;
            if (t.last_run_ms > 0) {
                last_run_sec = static_cast<double>(t.last_run_ms) / 1000.0;
            }
            out << "themis_scheduler_task_last_run_timestamp_seconds"
                << "{task_id=\"" << t.id << "\",task_name=\"" << safe_name << "\"} "
                << last_run_sec << "\n";
        }

        // Enabled flag (1 = enabled, 0 = disabled)
        out << "# HELP themis_scheduler_task_enabled"
               " Whether a task is currently enabled (1) or disabled (0).\n";
        out << "# TYPE themis_scheduler_task_enabled gauge\n";
        for (const auto& t : task_snapshot) {
            std::string safe_name = t.name;
            for (char& c : safe_name) {
                if (c == '"' || c == '\\' || c == '\n') c = '_';
            }
            out << "themis_scheduler_task_enabled"
                << "{task_id=\"" << t.id << "\",task_name=\"" << safe_name << "\"} "
                << (t.enabled ? 1 : 0) << "\n";
        }
    }

    // Dynamic scaling metrics (always emitted; values reflect config when scaling disabled)
    out << "# HELP themis_scheduler_concurrency_limit"
           " Current effective max-concurrent-tasks limit (dynamic or static).\n";
    out << "# TYPE themis_scheduler_concurrency_limit gauge\n";
    out << "themis_scheduler_concurrency_limit " << dynamic_limit_.load() << "\n";

    out << "# HELP themis_scheduler_queue_depth"
           " Number of tasks ready to run but waiting for a free worker slot on the last tick.\n";
    out << "# TYPE themis_scheduler_queue_depth gauge\n";
    out << "themis_scheduler_queue_depth " << queue_depth_.load() << "\n";

    return out.str();
}

std::vector<ScheduledTask> TaskScheduler::listTasks() const {

    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    std::vector<ScheduledTask> result;
    result.reserve(tasks_.size());
    
    for (const auto& [id, task] : tasks_) {
        result.push_back(*task);
    }
    
    return result;
}

std::shared_ptr<ScheduledTask> TaskScheduler::getTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<scheduler::TaskAuditEvent> TaskScheduler::getExecutionHistory(
    const std::string& task_id,
    size_t limit,
    size_t offset) const {
    
    if (!audit_manager_) {
        return {};
    }
    
    scheduler::AuditQueryParams params;
    if (!task_id.empty()) {
        params.task_id = task_id;
    }
    params.limit = limit;
    params.offset = offset;
    params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
    
    return audit_manager_->queryAuditEvents([[maybe_unused]] params);
}

// ===== Scheduler Loop =====

void TaskScheduler::schedulerLoop() {
    THEMIS_INFO("TaskScheduler loop started");
    
    while (running_.load()) {
        auto span = Tracer::startSpan("TaskScheduler.tick");
        auto now = std::chrono::system_clock::now();
        
        std::vector<std::shared_ptr<ScheduledTask>> tasks_to_execute;
        size_t pending_count = 0;  // tasks ready but over the concurrency limit
        
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            last_run_ = now;

            // Use the dynamically adjusted limit (or the static config value).
            const size_t effective_limit = config_.enable_dynamic_scaling
                ? dynamic_limit_.load()
                : config_.max_concurrent_tasks;
            // Track slots consumed by already-running tasks and by tasks selected
            // in this tick to avoid over-dispatching past effective_limit.
            size_t scheduled_count = active_task_threads_.load();
            
            for (auto& [id, task] : tasks_) {
                if (!task->enabled || task->running) {
                    continue;
                }
                
                if (shouldExecute(*task, now)) {
                    // Check concurrent task limit
                    if (scheduled_count >= effective_limit) {
                        THEMIS_DEBUG("Max concurrent tasks reached ({}), delaying task {}",
                                    effective_limit, id);
                        ++pending_count;
                        // Starvation prevention via aging: count consecutive skips
                        // so the task can be boosted in the next sort pass.
                        if (config_.aging_threshold > 0) {
                            ++task->consecutive_skips;
                            if (task->consecutive_skips >= config_.aging_threshold) {
                                THEMIS_INFO("TaskScheduler: aging boost triggered for task '{}' "
                                           "(skipped {} ticks, priority={})",
                                           id, task->consecutive_skips,
                                           static_cast<int>(task->priority));
                            }
                        }
                        continue;
                    }
                    
                    // Audit log cron trigger activation (for cron-based tasks)
                    if (config_.enable_audit_logging && audit_logger_ && 
                        task->trigger_type == ScheduledTask::TriggerType::CRON) {
                        nlohmann::json details = {
                            {"task_name", task->name},
                            {"trigger_type", "CRON"},
                            {"cron_expression", task->cron_expression}
                        };
                        
                        audit_logger_->logTaskSchedulerEvent(
                            utils::SecurityEventType::TASK_CRON_TRIGGERED,
                            id,
                            TaskScheduler::currentUserId(), // propagated from thread-local RequestContext
                            details
                        );
                    }
                    
                    tasks_to_execute.push_back(task);
                    ++scheduled_count;
                }
            }
        }

        // Adjust concurrency limit based on pending queue depth (no-op when scaling disabled).
        adjustConcurrencyLimit(pending_count);

        // Sort pending tasks by effective priority (HIGH first) before dispatch.
        // Aging: a task that has been skipped >= aging_threshold consecutive ticks
        // gets its base priority boosted by one level for the sort key, ensuring
        // it cannot be indefinitely starved by higher-priority tasks.
        const uint32_t aging_thr = config_.aging_threshold;
        auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
            const int base = static_cast<int>(t->priority);
            if (aging_thr > 0 && t->consecutive_skips >= aging_thr) {
                // Boost by 1 level (clamped to HIGH == 2).
                return std::min(base + 1, static_cast<int>(ScheduledTask::Priority::HIGH));
            }
            return base;
        };
        std::sort(tasks_to_execute.begin(), tasks_to_execute.end(),
            [&effectivePriority](const std::shared_ptr<ScheduledTask>& a,
                                 const std::shared_ptr<ScheduledTask>& b) {
                return effectivePriority(a) > effectivePriority(b);
            });

        // Execute tasks outside the lock
        for (auto& task : tasks_to_execute) {
            // Reset aging counter: this task is being dispatched.
            task->consecutive_skips = 0;
            task->running = true;
            active_task_threads_.fetch_add(1);
            
            // Launch task in separate thread
            std::thread task_thread([this, task]() {
                executeTask(task);
                active_task_threads_.fetch_sub(1);
            });
            
            // Store thread for cleanup
            {
                std::lock_guard<std::mutex> lock(running_mutex_);
                auto existing = running_task_threads_.find(task->id);
                if (existing != running_task_threads_.end()) {
                    if (existing->second.joinable()) {
                        existing->second.join();
                    }
                    running_task_threads_.erase(existing);
                }
                running_task_threads_[task->id] = std::move(task_thread);
            }
        }
        
        span.setAttribute("tasks_executed", static_cast<int64_t>(tasks_to_execute.size()));
        span.setAttribute("tasks_pending",  static_cast<int64_t>(pending_count));
        
        // Wait for next check interval or shutdown signal
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        cv_.wait_for(lock, config_.check_interval, [this] { return !running_.load(); });
    }
    
    THEMIS_INFO("TaskScheduler loop stopped");
}

void TaskScheduler::executeTask(std::shared_ptr<ScheduledTask> task) {
    auto span = Tracer::startSpan("TaskScheduler.executeTask");
    span.setAttribute("task_id", task->id);
    span.setAttribute("task_name", task->name);
    
    // ========================================================================
    // HARDENING: Execution Serialization (Phase 3 contract)
    // ========================================================================
    // Acquire per-task execution lock to serialize concurrent runs of the same task.
    // This ensures that the same task cannot run concurrently across scheduler loops
    // or manual execution paths.
    std::mutex& task_exec_lock = getTaskExecutionLock(task->id);
    std::lock_guard<std::mutex> exec_lock(task_exec_lock);
    
    auto start = std::chrono::steady_clock::now();
    const int64_t exec_timestamp_ms = getCurrentTimeMs();  // captured for result store
    
    // Create audit event for task start
    scheduler::TaskAuditEvent start_event;
    if (audit_manager_) {
        start_event.uuid = scheduler::generateUUID();
        start_event.timestamp = std::chrono::system_clock::now();
        start_event.task_id = task->id;
        start_event.task_name = task->name;
        start_event.task_description = task->description;
        start_event.event_type = scheduler::TaskEventType::TASK_STARTED;
        start_event.trigger_type = getTriggerTypeString([[maybe_unused]] task->trigger_type);
        setDefaultAuditContext([[maybe_unused]] start_event);
        
        audit_manager_->logAuditEvent([[maybe_unused]] start_event);
    }

    // Retry loop with strategy-based backoff (RetryPolicy)
    const ScheduledTask::RetryPolicy policy = effectiveRetryPolicy(*task);
    const size_t base_max_attempts = (policy.strategy == ScheduledTask::RetryStrategy::NONE)
                                         ? 1
                                         : 1 + policy.max_retries;
    size_t max_attempts = base_max_attempts;  // may be clamped by SLO adaptation
    std::string last_error;
    bool succeeded = false;
    nlohmann::json result;
    size_t attempts_made = 0;

    for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
        // Abort if scheduler is stopping
        if (!running_.load()) {
            break;
        }

        // Compute and apply delay before each retry (not before the first attempt)
        if (attempt > 0) {
            double delay_ms = static_cast<double>(computeRetryDelay(policy, attempt - 1).count());

            // SLO-based adaptive retry: clamp delay / skip retry if budget exhausted.
            {
                auto now_elapsed = std::chrono::steady_clock::now();
                double elapsed_so_far =
                    std::chrono::duration<double, std::milli>(now_elapsed - start).count();
                if (!applySloAdaptation(*task, elapsed_so_far, delay_ms, max_attempts)) {
                    THEMIS_INFO("Task {} SLO budget exhausted after {:.0f}ms; "
                                "skipping {} remaining retries",
                                task->id, elapsed_so_far,
                                max_attempts - attempt);
                    break;
                }
            }

            THEMIS_INFO("Retrying task {} (attempt {}/{}) after {}ms [strategy={}]",
                        task->id, attempt + 1, max_attempts,
                        static_cast<int64_t>(delay_ms),
                        static_cast<int>(policy.strategy));

            if (delay_ms > 0.0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int64_t>(delay_ms)));
            }
            if (!running_.load()) break;
        }

        ++attempts_made;
        try {
            if (task->type == ScheduledTask::TaskType::AQL_QUERY) {
                span.setAttribute("task_type", "aql");
                result = executeAqlQuery(task->aql_query);
            } else {
                span.setAttribute("task_type", "function");
                result = executeFunction(task->function_name, task->parameters);
            }
            succeeded = true;
            break;  // Success – no more retries needed
        } catch (const std::exception& e) {
            last_error = e.what();
            // Honor conditional should_retry if provided
            if (policy.should_retry && !policy.should_retry(last_error)) {
                THEMIS_INFO("Task {} retry skipped by should_retry policy: {}",
                            task->id, last_error);
                break;
            }
            if (attempt + 1 < max_attempts) {
                THEMIS_WARN("Task {} attempt {}/{} failed: {} (will retry)",
                            task->id, attempt + 1, max_attempts, e.what());
            }
        } catch (...) {
            last_error = "unknown non-exception thrown";
            THEMIS_ERROR("Task {} threw non-exception type; no retry", task->id);
            break;  // Non-std exceptions are never retried
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    if (succeeded) {
        // Update statistics
        task->last_run_ms = getCurrentTimeMs();
        task->total_executions++;
        task->successful_executions++;
        task->last_success_time = std::chrono::system_clock::now();
        task->last_error_category = ScheduledTask::ErrorCategory::NONE;  // Reset on success
        
        // Update moving average of execution time
        task->avg_execution_time_ms = 
            (task->avg_execution_time_ms * (task->total_executions - 1) + elapsed_ms) 
            / task->total_executions;
        
        updateNextRun(*task);
        total_executions_++;
        span.setAttribute("execution_time_ms", elapsed_ms);
        
        THEMIS_DEBUG("Executed task {} ({}): {:.2f}ms",
                     task->id, task->name, elapsed_ms);
        
        // Log audit event for successful completion
        if (audit_manager_) {
            scheduler::TaskAuditEvent completion_event;
            completion_event.uuid = scheduler::generateUUID();
            completion_event.timestamp = std::chrono::system_clock::now();
            completion_event.duration_ms = elapsed_ms;
            completion_event.task_id = task->id;
            completion_event.task_name = task->name;
            completion_event.task_description = task->description;
            completion_event.event_type = scheduler::TaskEventType::TASK_COMPLETED;
            completion_event.trigger_type = start_event.trigger_type;
            setDefaultAuditContext([[maybe_unused]] completion_event);
            completion_event.success = true;
            
            // Resource usage (basic metrics)
            completion_event.resource_usage.execution_time_ms = elapsed_ms;
            completion_event.resource_usage.cpu_time_ms = elapsed_ms; // Approximate
            
            // Extract result metrics if available
            if (result.is_object()) {
                if (result.contains("rows")) {
                    completion_event.resource_usage.result_rows = result["rows"].get<uint64_t>();
                }
                if (result.contains("affected")) {
                    completion_event.resource_usage.affected_rows = result["affected"].get<uint64_t>();
                }
            }
            
            audit_manager_->logAuditEvent([[maybe_unused]] completion_event);
        }
        
        if (task->on_success) {
            task->on_success(task->id, result);
        }

        // Resolve any active failure alert for this task
        resolveTaskFailureAlert(task->id);
    } else {
        // All attempts failed
        task->total_executions++;  // Mirror executeTaskNow/executeDAG behaviour
        task->failed_executions++;
        task->last_error = last_error;
        task->last_error_category = categorizeError(last_error);
        task->last_failure_time = std::chrono::system_clock::now();
        failed_executions_++;
        
        updateNextRun(*task);
        
        span.recordError(last_error);
        THEMIS_ERROR("Task {} failed after {} attempt(s): {}",
                     task->id, max_attempts, last_error);
        
        // Log audit event for failure
        if (audit_manager_) {
            scheduler::TaskAuditEvent failure_event;
            failure_event.uuid = scheduler::generateUUID();
            failure_event.timestamp = std::chrono::system_clock::now();
            failure_event.duration_ms = elapsed_ms;
            failure_event.task_id = task->id;
            failure_event.task_name = task->name;
            failure_event.task_description = task->description;
            failure_event.event_type = scheduler::TaskEventType::TASK_FAILED;
            failure_event.trigger_type = start_event.trigger_type;
            setDefaultAuditContext([[maybe_unused]] failure_event);
            failure_event.success = false;
            failure_event.error_message = last_error;
            failure_event.error_type = "EXECUTION_ERROR";
            failure_event.metadata["attempts_made"] = attempts_made;
            
            // Resource usage
            failure_event.resource_usage.execution_time_ms = elapsed_ms;
            failure_event.resource_usage.cpu_time_ms = elapsed_ms;
            
            auto anomaly_metrics = audit_manager_->logAuditEvent([[maybe_unused]] failure_event);
            
            // If anomaly detected in failures, log as security event
            if (anomaly_metrics.is_anomalous && anomaly_metrics.failure_rate_score > 0.7) {
                scheduler::TaskSecurityEvent security_event;
                security_event.uuid = scheduler::generateUUID();
                security_event.timestamp = std::chrono::system_clock::now();
                security_event.task_id = task->id;
                security_event.task_name = task->name;
                security_event.event_type = scheduler::TaskSecurityEventType::EXCESSIVE_FAILURES;
                security_event.severity = "HIGH";
                setDefaultAuditContext([[maybe_unused]] security_event);
                security_event.violation_type = "excessive_failures";
                security_event.description = "Task showing excessive failure rate: " + anomaly_metrics.description;
                security_event.details["anomaly_score"] = anomaly_metrics.overall_score;
                security_event.details["failure_rate_score"] = anomaly_metrics.failure_rate_score;
                security_event.blocked = false;
                security_event.action_taken = "logged_for_review";
                
                audit_manager_->logSecurityEvent([[maybe_unused]] security_event);
            }
        }
        
        if (task->on_failure) {
            task->on_failure(task->id, last_error);
        }

        // Fire failure alert
        fireTaskFailureAlert(*task, last_error);

        span.recordError(last_error);
    }

    // Check SLA breach (applies regardless of success/failure)
    if (task->sla_deadline.has_value() &&
        elapsed_ms > static_cast<double>(task->sla_deadline->count())) {
        fireTaskSlaBreachAlert(*task, elapsed_ms);
    }

    // SLO compliance tracking (Phase 5: SLO-based adaptive retry).
    // Track SLA violations in a rolling window so applySloAdaptation() can
    // reduce retry attempts when the compliance rate drops below threshold.
    if (task->slo_retry_config.has_value() && task->sla_deadline.has_value()) {
        const auto& slo = *task->slo_retry_config;
        if (slo.slo_aware && slo.slo_history_window > 0) {
            const bool violated = elapsed_ms >
                static_cast<double>(task->sla_deadline->count());
            // Slide the window when it fills up: reset counters proportionally.
            if (task->slo_window_count >= slo.slo_history_window) {
                const double ratio =
                    static_cast<double>(task->slo_violations) /
                    static_cast<double>(task->slo_window_count);
                task->slo_window_count  = slo.slo_history_window / 2;
                task->slo_violations    =
                    static_cast<size_t>(ratio * static_cast<double>(task->slo_window_count));
            }
            ++task->slo_window_count;
            if (violated) {
                ++task->slo_violations;
            }
        }
    }

    // Persist execution result to ThemisDB (if result store is enabled)
    if (result_store_) {
        scheduler::TaskExecutionResult exec_result;
        exec_result.task_id      = task->id;
        exec_result.task_name    = task->name;
        exec_result.timestamp_ms = exec_timestamp_ms;
        exec_result.duration_ms  = elapsed_ms;
        exec_result.success      = succeeded;
        exec_result.output       = succeeded ? result : nlohmann::json{};
        exec_result.error        = succeeded ? "" : last_error;
        result_store_->store(exec_result);
    }

    task->running = false;
}

nlohmann::json TaskScheduler::executeAqlQuery(const std::string& aql) {
    // ⚠️ SECURITY: AQL queries can read/write any data
    
    // Implement query validation and sanitization
    validateAqlQuery(aql);
    
    // Apply query complexity limits to prevent resource exhaustion
    enforceQueryComplexityLimits(aql);
    
    // Note: Row/resource limits should be enforced at query engine level
    // The query engine should have configurable limits for:
    // - Maximum execution time
    // - Maximum memory usage
    // - Maximum result set size
    // - Maximum number of operations
    
    auto span = Tracer::startSpan("TaskScheduler.executeAqlQuery");
    span.setAttribute("aql", aql);
    
    auto result = executeAql(aql, *query_engine_);
    
    if (!result) {
        throw std::runtime_error("AQL query failed: " + result.error().message());
    }
    
    return *result;
}

nlohmann::json TaskScheduler::executeFunction(const std::string& name, const nlohmann::json& params) {
    auto span = Tracer::startSpan("TaskScheduler.executeFunction");
    span.setAttribute("function_name", name);
    
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::runtime_error("Function not found: " + name);
    }

    if (config_.sandbox_execution) {
        modules::ModuleSandbox::Config sandbox_cfg;
        modules::ModuleSandbox sandbox(sandbox_cfg);
        if (!sandbox.launch(name)) {
            THEMIS_WARN("Sandbox launch failed for function '{}': {}; executing without sandbox",
                        name, sandbox.lastError());
            return it->second(params);
        }
        auto result = it->second(params);
        sandbox.shutdown();
        return result;
    }
    
    return it->second(params);
}

// ===== Scheduling Logic =====

bool TaskScheduler::shouldExecute(const ScheduledTask& task, 
                                   const std::chrono::system_clock::time_point& now) const {
    if (task.running && !config_.allow_task_overlap) {
        return false;  // Task already running
    }
    
    // Check based on trigger type
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        return shouldExecuteCron(task, now);
    } else if (task.trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        return now >= task.next_run;
    } else if ([[maybe_unused]] task.trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        // CDC events trigger tasks directly via callbacks
        return false;
    } else if (task.trigger_type == ScheduledTask::TriggerType::MANUAL) {
        // Manual tasks are only executed via executeTaskNow()
        return false;
    }
    
    return false;
}

void TaskScheduler::updateNextRun(ScheduledTask& task) {
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        // Calculate next run from cron expression
        auto it = cron_expressions_.find(task.id);
        if (it != cron_expressions_.end()) {
            auto next = it->second->getNextExecution(std::chrono::system_clock::now());
            if (next) {
                task.next_run = *next;
            }
        }
    } else if (task.trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        // Traditional interval-based scheduling
        task.next_run = std::chrono::system_clock::now() + task.interval;
    }
    // CDC_EVENT and MANUAL types don't have a next_run
}

// ===== Persistence =====

void TaskScheduler::saveTasks() {
    // ⚠️ SECURITY: Task definitions may contain sensitive data (queries, parameters)
    
    // Note: For production deployments, implement:
    // 1. Encryption of task definitions at rest using a secure key management system
    // 2. Proper file permissions (600 or 400) to restrict access to task files
    // 3. Consider using a secure key-value store (e.g., encrypted RocksDB) instead of plain JSON files
    
    // Simple JSON-based persistence
    nlohmann::json tasks_json = nlohmann::json::array();
    
    for (const auto& [id, task] : tasks_) {
        nlohmann::json task_json;
        task_json["id"] = task->id;
        task_json["name"] = task->name;
        task_json["description"] = task->description;
        task_json["type"] = task->type == ScheduledTask::TaskType::AQL_QUERY ? "aql" : "function";
        task_json["aql_query"] = task->aql_query;
        task_json["function_name"] = task->function_name;
        task_json["parameters"] = task->parameters;
        task_json["interval_ms"] = task->interval.count();
        task_json["enabled"] = task->enabled;
        
        // Save trigger configuration
        task_json["trigger_type"] = static_cast<int>(task->trigger_type);
        task_json["cron_expression"] = task->cron_expression;
        task_json["priority"] = static_cast<int>(task->priority);
        task_json["trigger_logic"] = static_cast<int>(task->trigger_logic);
        
        // Save CDC trigger config
        nlohmann::json cdc_json;
        cdc_json["key_prefix"] = task->cdc_trigger.key_prefix;
        cdc_json["event_types"] = nlohmann::json::array();
        for ([[maybe_unused]] int type : task->cdc_trigger.event_types) {
            cdc_json["event_types"].push_back([[maybe_unused]] type);
        }
        if (task->cdc_trigger.condition) {
            cdc_json["condition"] = *task->cdc_trigger.condition;
        }
        cdc_json["debounce_ms"] = task->cdc_trigger.debounce_ms;
        task_json["cdc_trigger"] = cdc_json;

        // Save retry policy (serialize strategy + parameters; cannot persist should_retry lambda)
        nlohmann::json retry_json;
        if (task->retry_policy) {
            const auto& rp = *task->retry_policy;
            retry_json["strategy"]           = static_cast<int>(rp.strategy);
            retry_json["max_retries"]        = rp.max_retries;
            retry_json["initial_delay_ms"]   = rp.initial_delay.count();
            retry_json["max_delay_ms"]       = rp.max_delay.count();
            retry_json["backoff_multiplier"] = rp.backoff_multiplier;
            retry_json["jitter_factor"]      = rp.jitter_factor;
            task_json["retry_policy"] = retry_json;
        } else {
            task_json["max_retries"] = task->max_retries;
        }

        // Save SLO-based adaptive retry config (Phase 5)
        if (task->slo_retry_config) {
            const auto& slo = *task->slo_retry_config;
            nlohmann::json slo_json;
            slo_json["slo_aware"]                   = slo.slo_aware;
            slo_json["slo_budget_fraction"]          = slo.slo_budget_fraction;
            slo_json["slo_compliance_threshold"]     = slo.slo_compliance_threshold;
            slo_json["min_retries_under_pressure"]   = slo.min_retries_under_pressure;
            slo_json["slo_history_window"]           = slo.slo_history_window;
            task_json["slo_retry_config"] = slo_json;
        }
        // Persist SLO compliance counters so the window survives restarts
        task_json["slo_violations"]    = task->slo_violations;
        task_json["slo_window_count"]  = task->slo_window_count;

        // Save dependency list
        task_json["dependencies"] = task->dependencies;
        
        tasks_json.push_back(task_json);
    }
    
    try {
        std::string filepath = config_.persistence_path + "/tasks.json";
        std::ofstream file(filepath);
        if (!file.good()) {
            throw std::runtime_error("Failed to open file for writing: " + filepath);
        }
        
        file << tasks_json.dump(2);
        file.close();
        
        // Set restrictive file permissions (owner read/write only)
        #ifndef _WIN32
        chmod(filepath.c_str(), S_IRUSR | S_IWUSR);
        #endif
        
        THEMIS_DEBUG("Saved {} tasks to disk with secure permissions", tasks_.size());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save tasks: {}", e.what());
    }

    // Persist anomaly detector baseline so statistics survive a restart
    if (audit_manager_) {
        try {
            std::string anomaly_path = config_.persistence_path + "/anomaly_stats.json";
            std::ofstream af(anomaly_path);
            if (af.good()) {
                af << audit_manager_->exportAnomalyStatistics().dump(2);
                af.close();
                #ifndef _WIN32
                chmod(anomaly_path.c_str(), S_IRUSR | S_IWUSR);
                #endif
                THEMIS_DEBUG("Saved anomaly detector statistics to disk");
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to save anomaly statistics: {}", e.what());
        }
    }
}

void TaskScheduler::loadTasks() {
    try {
        std::ifstream file(config_.persistence_path + "/tasks.json");
        if (!file.good()) {
            THEMIS_DEBUG("No persisted tasks found");
            return;
        }
        
        nlohmann::json tasks_json;
        file >> tasks_json;
        
        for (const auto& task_json : tasks_json) {
            ScheduledTask task;
            task.id = task_json.value("id", "");
            task.name = task_json.value("name", "");
            task.description = task_json.value("description", "");
            
            std::string type_str = task_json.value("type", "aql");
            task.type = (type_str == "aql") ? ScheduledTask::TaskType::AQL_QUERY 
                                            : ScheduledTask::TaskType::FUNCTION;
            
            task.aql_query = task_json.value("aql_query", "");
            task.function_name = task_json.value("function_name", "");
            task.parameters = task_json.value("parameters", nlohmann::json::object());
            task.interval = std::chrono::milliseconds(task_json.value("interval_ms", 300000));
            task.enabled = task_json.value("enabled", true);
            
            // Load trigger configuration (with defaults for backward compatibility)
            task.trigger_type = static_cast<ScheduledTask::TriggerType>(
                task_json.value("trigger_type", static_cast<int>(ScheduledTask::TriggerType::INTERVAL)));
            task.cron_expression = task_json.value("cron_expression", "");
            task.priority = static_cast<ScheduledTask::Priority>(
                task_json.value("priority", static_cast<int>(ScheduledTask::Priority::NORMAL)));
            task.trigger_logic = static_cast<ScheduledTask::TriggerLogic>(
                task_json.value("trigger_logic", static_cast<int>(ScheduledTask::TriggerLogic::OR)));
            
            // Load CDC trigger config
            if (task_json.contains("cdc_trigger")) {
                auto cdc_json = task_json["cdc_trigger"];
                task.cdc_trigger.key_prefix = cdc_json.value("key_prefix", "");
                task.cdc_trigger.debounce_ms = cdc_json.value("debounce_ms", 0);
                
                if ([[maybe_unused]] cdc_json.contains("event_types")) {
                    for ([[maybe_unused]] const auto& type : cdc_json["event_types"]) {
                        task.cdc_trigger.event_types.insert([[maybe_unused]] type.get<int>());
                    }
                }
                
                if (cdc_json.contains("condition")) {
                    task.cdc_trigger.condition = cdc_json["condition"].get<std::string>();
                }
            }

            // Restore retry policy (legacy max_retries if no retry_policy block)
            if (task_json.contains("retry_policy")) {
                const auto& rp_json = task_json["retry_policy"];
                ScheduledTask::RetryPolicy rp;
                rp.strategy = static_cast<ScheduledTask::RetryStrategy>(
                    rp_json.value("strategy",
                                  static_cast<int>(ScheduledTask::RetryStrategy::EXPONENTIAL_BACKOFF)));
                rp.max_retries        = rp_json.value("max_retries", size_t{3});
                rp.initial_delay      = std::chrono::milliseconds(rp_json.value("initial_delay_ms", int64_t{1000}));
                rp.max_delay          = std::chrono::milliseconds(rp_json.value("max_delay_ms", int64_t{30000}));
                rp.backoff_multiplier = rp_json.value("backoff_multiplier", 2.0);
                rp.jitter_factor      = rp_json.value("jitter_factor", 0.1);
                task.retry_policy = rp;
            } else {
                task.max_retries = task_json.value("max_retries", size_t{3});
            }

            // Restore SLO-based adaptive retry config (Phase 5)
            if (task_json.contains("slo_retry_config")) {
                const auto& slo_json = task_json["slo_retry_config"];
                ScheduledTask::SloRetryConfig slo;
                slo.slo_aware                 = slo_json.value("slo_aware", true);
                slo.slo_budget_fraction       = slo_json.value("slo_budget_fraction", 0.5);
                slo.slo_compliance_threshold  = slo_json.value("slo_compliance_threshold", 0.8);
                slo.min_retries_under_pressure = slo_json.value("min_retries_under_pressure", size_t{1});
                slo.slo_history_window        = slo_json.value("slo_history_window", size_t{20});
                task.slo_retry_config = slo;
            }
            // Restore SLO compliance counters
            task.slo_violations   = task_json.value("slo_violations",   size_t{0});
            task.slo_window_count = task_json.value("slo_window_count", size_t{0});

            // Restore dependency list (backward-compatible: absent = no deps)
            if (task_json.contains("dependencies")) {
                task.dependencies = task_json["dependencies"].get<std::vector<std::string>>();
            }
            
            registerTask(task);
        }
        
        THEMIS_INFO("Loaded {} tasks from disk", tasks_.size());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load tasks: {}", e.what());
    }

    // Restore anomaly detector baseline statistics
    if (audit_manager_) {
        try {
            std::ifstream af(config_.persistence_path + "/anomaly_stats.json");
            if (af.good()) {
                nlohmann::json anomaly_json;
                af >> anomaly_json;
                audit_manager_->importAnomalyStatistics(anomaly_json);
                THEMIS_DEBUG("Restored anomaly detector statistics from disk");
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to restore anomaly statistics (non-fatal): {}", e.what());
        }
    }
}

// ===== Helpers =====

int64_t TaskScheduler::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string TaskScheduler::generateTaskId(const ScheduledTask& task) const {
    std::ostringstream oss;
    
    if (!task.name.empty()) {
        // Use name as base
        oss << task.name;
        // Replace spaces with underscores
        std::string id = oss.str();
        std::replace(id.begin(), id.end(), ' ', '_');
        return id;
    } else if (!task.aql_query.empty()) {
        // Use hash of AQL query
        oss << "aql_task_" << std::hash<std::string>{}(task.aql_query);
        return oss.str();
    } else {
        // Use function name
        oss << "func_" << task.function_name;
        return oss.str();
    }
}

// ===== Security & Validation Helpers =====

void TaskScheduler::validateAqlQuery(const std::string& aql) const {
    if (aql.empty()) {
        throw std::invalid_argument("AQL query cannot be empty");
    }
    
    // Use AST-based injection detection for robust security
    security::AQLInjectionDetector detector;
    auto validation_result = detector.validateAQLAST(aql);
    
    if (!validation_result.is_safe) {
        THEMIS_ERROR("AQL injection detected: {}", validation_result.error_message);
        
        // Log detected patterns for security diagnostics
        for (const auto& pattern : validation_result.detected_patterns) {
            THEMIS_WARN("Injection pattern detected: {}", pattern);
        }
        
        // Log security event for AQL injection attempt
        if (audit_manager_) {
            scheduler::TaskSecurityEvent security_event;
            security_event.uuid = scheduler::generateUUID();
            security_event.timestamp = std::chrono::system_clock::now();
            security_event.event_type = scheduler::TaskSecurityEventType::AQL_INJECTION_DETECTED;
            security_event.severity = "CRITICAL";
            security_event.user_id = DEFAULT_AUDIT_USER;
            security_event.ip_address = DEFAULT_AUDIT_IP;
            security_event.violation_type = "aql_injection";
            security_event.description = "AQL injection detected: " + validation_result.error_message;
            security_event.details["detected_patterns"] = validation_result.detected_patterns;
            security_event.details["query_excerpt"] = aql.substr(0, std::min(size_t(100), aql.length()));
            security_event.blocked = true;
            security_event.action_taken = "query_rejected";
            
            audit_manager_->logSecurityEvent([[maybe_unused]] security_event);
        }
        
        throw std::invalid_argument("AQL query validation failed: " + validation_result.error_message);
    }
    
    // Check for reasonable length limit (prevent DoS)
    const size_t MAX_QUERY_LENGTH = 100000; // 100KB
    if (aql.length() > MAX_QUERY_LENGTH) {
        throw std::invalid_argument("AQL query exceeds maximum length of " + 
                                   std::to_string(MAX_QUERY_LENGTH) + " characters");
    }
}

void TaskScheduler::validateResourceLimits(const ScheduledTask& task) const {
    // Validate timeout
    const auto MAX_TIMEOUT = std::chrono::hours(24);
    const auto MIN_TIMEOUT = std::chrono::seconds(1);
    
    if (task.timeout > MAX_TIMEOUT) {
        auto max_timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MAX_TIMEOUT).count();
        
        // Log security event for excessive timeout
        if (audit_manager_) {
            scheduler::TaskSecurityEvent security_event;
            security_event.uuid = scheduler::generateUUID();
            security_event.timestamp = std::chrono::system_clock::now();
            security_event.task_id = task.id;
            security_event.task_name = task.name;
            security_event.event_type = scheduler::TaskSecurityEventType::RESOURCE_LIMIT_EXCEEDED;
            security_event.severity = "MEDIUM";
            security_event.user_id = DEFAULT_AUDIT_USER;
            security_event.ip_address = DEFAULT_AUDIT_IP;
            security_event.violation_type = "timeout_limit_exceeded";
            security_event.description = "Task timeout exceeds maximum allowed";
            security_event.details["requested_timeout_ms"] = task.timeout.count();
            security_event.details["max_timeout_ms"] = max_timeout_ms;
            security_event.blocked = true;
            security_event.action_taken = "task_rejected";
            
            audit_manager_->logSecurityEvent([[maybe_unused]] security_event);
        }
        
        throw std::invalid_argument("Task timeout exceeds maximum allowed: " + 
                                   std::to_string(max_timeout_ms) + " milliseconds");
    }
    
    if (task.timeout < MIN_TIMEOUT) {
        auto min_timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MIN_TIMEOUT).count();
        throw std::invalid_argument("Task timeout is too short. Minimum: " + 
                                   std::to_string(min_timeout_ms) + " milliseconds");
    }
    
    // Validate max_retries (legacy field and retry_policy.max_retries)
    const size_t MAX_RETRIES = 10;
    if (task.max_retries > MAX_RETRIES) {
        throw std::invalid_argument("Task max_retries exceeds maximum allowed: " + 
                                   std::to_string(MAX_RETRIES));
    }
    if (task.retry_policy && task.retry_policy->max_retries > MAX_RETRIES) {
        throw std::invalid_argument("Task retry_policy.max_retries exceeds maximum allowed: " +
                                   std::to_string(MAX_RETRIES));
    }
    
    // Validate interval
    const auto MIN_INTERVAL = std::chrono::seconds(1);
    if (task.interval < MIN_INTERVAL) {
        auto min_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MIN_INTERVAL).count();
        throw std::invalid_argument("Task interval is too short. Minimum: " + 
                                   std::to_string(min_interval_ms) + " milliseconds");
    }
}

ScheduledTask TaskScheduler::sanitizeTask(const ScheduledTask& task) const {
    ScheduledTask sanitized = task;
    
    // Sanitize string fields to prevent injection
    auto sanitizeString = [](const std::string& input) -> std::string {
        std::string output;
        output.reserve(input.size());
        
        for (char c : input) {
            // Remove null bytes and other control characters except newline and tab
            if (c == '\0' || (c < 32 && c != '\n' && c != '\t')) {
                continue;
            }
            output += c;
        }
        
        return output;
    };
    
    sanitized.id = sanitizeString(task.id);
    sanitized.name = sanitizeString(task.name);
    sanitized.description = sanitizeString(task.description);
    sanitized.function_name = sanitizeString(task.function_name);
    
    // Limit field lengths
    const size_t MAX_NAME_LENGTH = 255;
    const size_t MAX_DESC_LENGTH = 1000;
    
    if (sanitized.name.length() > MAX_NAME_LENGTH) {
        sanitized.name = sanitized.name.substr(0, MAX_NAME_LENGTH);
    }
    
    if (sanitized.description.length() > MAX_DESC_LENGTH) {
        sanitized.description = sanitized.description.substr(0, MAX_DESC_LENGTH);
    }
    
    return sanitized;
}

void TaskScheduler::enforceQueryComplexityLimits(const std::string& aql) const {
    // Basic complexity checks to prevent resource exhaustion
    // Note: These are heuristic checks and may have false positives
    
    // Count nested levels (approximate complexity)
    int nesting_level = 0;
    int max_nesting = 0;
    
    for (char c : aql) {
        if (c == '(' || c == '{' || c == '[') {
            nesting_level++;
            max_nesting = std::max(max_nesting, nesting_level);
        } else if (c == ')' || c == '}' || c == ']') {
            nesting_level--;
        }
    }
    
    const int MAX_NESTING_LEVEL = 20;
    if (max_nesting > MAX_NESTING_LEVEL) {
        throw std::invalid_argument("Query nesting level exceeds maximum allowed: " + 
                                   std::to_string(MAX_NESTING_LEVEL));
    }
    
    // Count number of FOR loops (heuristic - may have false positives in comments/strings)
    // This is acceptable as overly complex queries in comments would still indicate
    // potential issues. The limit is set high enough to allow legitimate queries.
    size_t for_count = 0;
    size_t pos = 0;
    std::string aql_upper = aql;
    std::transform(aql_upper.begin(), aql_upper.end(), aql_upper.begin(), ::toupper);
    
    while ((pos = aql_upper.find("FOR ", pos)) != std::string::npos) {
        // Check word boundary: must be at start or preceded by non-alphanumeric
        bool valid_start = (pos == 0 || !std::isalnum(static_cast<unsigned char>(aql_upper[pos - 1])));
        if (valid_start) {
            for_count++;
        }
        pos += 4;
    }
    
    const size_t MAX_FOR_LOOPS = 5;
    if (for_count > MAX_FOR_LOOPS) {
        throw std::invalid_argument("Query contains too many FOR loops. Maximum: " + 
                                   std::to_string(MAX_FOR_LOOPS));
    }
}

bool TaskScheduler::checkRateLimit(const std::string& task_id) {
    // Simple rate limiting implementation
    // In production, use a more sophisticated rate limiter (e.g., token bucket, sliding window)
    
    static std::mutex rate_limit_mutex;
    static std::map<std::string, std::deque<std::chrono::steady_clock::time_point>> execution_times;
    
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    
    const auto now = std::chrono::steady_clock::now();
    const auto window = std::chrono::minutes(1);
    const size_t max_executions = 10; // Max 10 manual executions per minute per task
    
    auto& times = execution_times[task_id];
    
    // Remove old entries outside the time window
    while (!times.empty() && (now - times.front()) > window) {
        times.pop_front();
    }
    
    // Check if limit is exceeded
    if (times.size() >= max_executions) {
        // Log security event for rate limit exceeded
        if (audit_manager_) {
            scheduler::TaskSecurityEvent security_event;
            security_event.uuid = scheduler::generateUUID();
            security_event.timestamp = std::chrono::system_clock::now();
            security_event.task_id = task_id;
            security_event.event_type = scheduler::TaskSecurityEventType::RATE_LIMIT_EXCEEDED;
            security_event.severity = "MEDIUM";
            security_event.user_id = DEFAULT_AUDIT_USER;
            security_event.ip_address = DEFAULT_AUDIT_IP;
            security_event.violation_type = "manual_execution_rate_limit";
            security_event.description = "Task execution rate limit exceeded";
            security_event.details["executions_in_window"] = times.size();
            security_event.details["max_executions"] = max_executions;
            security_event.details["window_minutes"] = 1;
            security_event.blocked = true;
            security_event.action_taken = "execution_denied";
            
            audit_manager_->logSecurityEvent([[maybe_unused]] security_event);
        }
        
        return false;
    }
    
    // Add current execution time
    times.push_back(now);
    
    return true;
}

// ===== Cron Expression Management =====

void TaskScheduler::validateCronExpression(const std::string& expression) const {
    if (expression.empty()) {
        throw std::invalid_argument("Cron expression cannot be empty");
    }
    
    auto validation = CronExpression::validate(expression);
    if (!validation.is_valid) {
        throw std::invalid_argument("Invalid cron expression: " + validation.error_message);
    }
}

std::shared_ptr<CronExpression> TaskScheduler::getCronExpression(const std::string& task_id) {
    auto it = cron_expressions_.find(task_id);
    if (it != cron_expressions_.end()) {
        return it->second;
    }
    return nullptr;
}

void TaskScheduler::updateCronExpression(const std::string& task_id, const std::string& expression) {
    auto parsed = CronExpression::parse(expression);
    if (!parsed) {
        throw std::invalid_argument("Failed to parse cron expression: " + expression);
    }
    cron_expressions_[task_id] = std::make_shared<CronExpression>(*parsed);
}

bool TaskScheduler::shouldExecuteCron(const ScheduledTask& task, 
                                       const std::chrono::system_clock::time_point& now) const {
    auto it = cron_expressions_.find(task.id);
    if (it == cron_expressions_.end()) {
        return false;
    }
    return it->second->matches(now);
}

// ===== CDC Trigger Management =====

void TaskScheduler::validateCDCTrigger(const ScheduledTask::CDCTrigger& trigger) const {
    if (trigger.key_prefix.empty()) {
        throw std::invalid_argument("CDC trigger key_prefix cannot be empty");
    }
    
    if ([[maybe_unused]] trigger.event_types.empty()) {
        throw std::invalid_argument([[maybe_unused]] "CDC trigger must specify at least one event type");
    }
    
    // Validate event types are in valid range (0-3)
    for ([[maybe_unused]] int type : trigger.event_types) {
        if (type < 0 || type > 3) {
            throw std::invalid_argument([[maybe_unused]] "Invalid CDC event type: " + std::to_string(type));
        }
    }
}

void TaskScheduler::setupEventTrigger([[maybe_unused]] std::shared_ptr<ScheduledTask> task) {
    if ([[maybe_unused]] !event_trigger_manager_) {
        THEMIS_ERROR([[maybe_unused]] "Cannot setup CDC trigger without EventTriggerManager");
        return;
    }
    
    // Convert task CDC config to EventTrigger config
    CDCTriggerConfig config;
    config.key_prefix = task->cdc_trigger.key_prefix;
    
    // Convert event types from int to ChangeEventType
    for ([[maybe_unused]] int type_int : task->cdc_trigger.event_types) {
        config.event_types.insert([[maybe_unused]] static_cast<Changefeed::ChangeEventType>(type_int));
    }
    
    config.condition = task->cdc_trigger.condition;
    config.debounce_ms = task->cdc_trigger.debounce_ms;
    
    // Create callback that triggers task execution via onCDCEvent
    auto callback = [this, task_id = task->id]([[maybe_unused]] const Changefeed::ChangeEvent& event) {
        // Retrieve task inside the callback to avoid capturing by value
        std::shared_ptr<ScheduledTask> task_ptr;
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            auto it = tasks_.find(task_id);
            if (it == tasks_.end()) {
                THEMIS_DEBUG("Task {} no longer exists, skipping execution", task_id);
                return;
            }
            task_ptr = it->second;
        }
        onCDCEvent(task_ptr, event);
    };
    
    // Register trigger with manager
    event_trigger_manager_->registerTrigger(task->id, config, std::move(callback));
}

void TaskScheduler::removeEventTrigger([[maybe_unused]] const std::string& task_id) {
    if ([[maybe_unused]] event_trigger_manager_) {
        event_trigger_manager_->unregisterTrigger([[maybe_unused]] task_id);
    }
}

void TaskScheduler::onCDCEvent(std::shared_ptr<ScheduledTask> task,
                               const Changefeed::ChangeEvent& event) {
    THEMIS_DEBUG("CDC event triggered task: {} (key={}, type={})",
                task->id, event.key, static_cast<int>([[maybe_unused]] event.type));

    // Audit log CDC trigger activation
    if (config_.enable_audit_logging && audit_logger_) {
        nlohmann::json details = {
            {"task_name", task->name},
            {"trigger_type", "CDC_EVENT"},
            {"cdc_key", event.key},
            {"cdc_event_type", static_cast<int>(event.type)},
            {"cdc_key_prefix", task->cdc_trigger.key_prefix}
        };

        audit_logger_->logTaskSchedulerEvent(
            utils::SecurityEventType::TASK_CDC_TRIGGERED,
            task->id,
            TaskScheduler::currentUserId(), // propagated from thread-local RequestContext
            details
        );
    }

    // Check if task is enabled
    if (!task->enabled) {
        THEMIS_DEBUG("Task {} is disabled, skipping execution", task->id);
        return;
    }

    // Check if scheduler has been stopped
    if (!running_.load()) {
        THEMIS_DEBUG("Task {} CDC event ignored: scheduler is not running", task->id);
        return;
    }

    // Check-and-set running state (guard against concurrent triggers)
    if (!config_.allow_task_overlap) {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        if (task->running) {
            THEMIS_DEBUG("Task {} is already running, skipping execution", task->id);
            return;
        }
        task->running = true;
    } else {
        task->running = true;
    }

    // Execute task asynchronously
    std::thread task_thread([this, task]() {
        executeTask(task);

        // Remove from running threads
        {
            std::lock_guard<std::mutex> lock(running_mutex_);
            running_task_threads_.erase(task->id);
        }
    });

    // Store thread for cleanup
    {
        std::lock_guard<std::mutex> lock(running_mutex_);
        running_task_threads_[task->id] = std::move(task_thread);
    }
}

// ===== Update shouldExecute and updateNextRun =====

// ===== Result Store Public APIs =====

std::vector<scheduler::TaskExecutionResult> TaskScheduler::getTaskResults(
        const std::string& task_id, size_t limit) const {
    if (!result_store_) {
        return {};
    }
    return result_store_->getResults(task_id, limit);
}

std::optional<scheduler::TaskExecutionResult> TaskScheduler::getLatestTaskResult(
        const std::string& task_id) const {
    if (!result_store_) {
        return std::nullopt;
    }
    return result_store_->getLatestResult(task_id);
}

// ===== Alertmanager Integration =====

void TaskScheduler::setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager) {
    std::unique_lock<std::shared_mutex> lock(alert_mutex_);
    alertmanager_ = std::move(alertmanager);
}

std::shared_ptr<observability::Alertmanager> TaskScheduler::getAlertmanager() const {
    std::shared_lock<std::shared_mutex> lock(alert_mutex_);
    return alertmanager_;
}

/*static*/ std::string TaskScheduler::makeTaskAlertId(const std::string& task_id,
                                                       const std::string& alert_type) {
    return "scheduler_task_" + alert_type + "_" + task_id;
}

void TaskScheduler::fireTaskFailureAlert(const ScheduledTask& task, const std::string& error) {
    // Take a copy of the alertmanager pointer under the lock, then release the lock
    // before calling sendAlert() to avoid holding the mutex during potentially blocking I/O.
    std::shared_ptr<observability::Alertmanager> am;
    {
        std::unique_lock<std::shared_mutex> lock(alert_mutex_);
        am = alertmanager_;
    }
    if (!am) {
        return;
    }

    const std::string alert_id = makeTaskAlertId(task.id, "failure");

    observability::Alert alert;
    alert.alert_id   = alert_id;
    alert.alert_name = "TaskFailure";
    alert.severity   = observability::AlertSeverity::ERROR;
    alert.status     = observability::AlertStatus::FIRING;
    alert.message    = "Task \"" + task.name + "\" (id=" + task.id + ") failed: " + error;

    alert.labels["component"]  = "scheduler";
    alert.labels["task_id"]    = task.id;
    alert.labels["task_name"]  = task.name;
    alert.labels["alertname"]  = "TaskFailure";
    alert.labels["severity"]   = "error";

    alert.annotations["error"]          = error;
    alert.annotations["failed_executions"] =
        std::to_string(task.failed_executions);
    alert.annotations["total_executions"] =
        std::to_string(task.total_executions);

    // sendAlert() may involve network I/O; called outside the lock
    auto result = am->sendAlert(alert);
    if (result) {
        std::unique_lock<std::shared_mutex> lock(alert_mutex_);
        active_failure_alert_ids_[task.id] = alert_id;
        THEMIS_WARN("Task failure alert fired for task {} ({}): {}", task.id, task.name, error);
    } else {
        THEMIS_ERROR("Failed to send task failure alert for task {}: {}",
                     task.id, result.error().message());
    }
}

void TaskScheduler::fireTaskSlaBreachAlert(const ScheduledTask& task, double elapsed_ms) {
    if (!task.sla_deadline.has_value()) {
        return;  // Caller should have checked, but guard defensively
    }

    // Take a copy of the alertmanager pointer under the lock, then release the lock
    // before calling sendAlert() to avoid holding the mutex during potentially blocking I/O.
    std::shared_ptr<observability::Alertmanager> am;
    {
        std::unique_lock<std::shared_mutex> lock(alert_mutex_);
        am = alertmanager_;
    }
    if (!am) {
        return;
    }

    const std::string alert_id = makeTaskAlertId(task.id, "sla_breach");
    const double sla_ms = static_cast<double>(task.sla_deadline->count());

    observability::Alert alert;
    alert.alert_id   = alert_id;
    alert.alert_name = "TaskSlaBreached";
    alert.severity   = observability::AlertSeverity::WARNING;
    alert.status     = observability::AlertStatus::FIRING;

    std::ostringstream msg;
    msg << "Task \"" << task.name << "\" (id=" << task.id << ") exceeded SLA deadline: "
        << "elapsed=" << static_cast<int64_t>(elapsed_ms) << "ms, "
        << "sla_deadline=" << static_cast<int64_t>(sla_ms) << "ms";
    alert.message = msg.str();

    alert.labels["component"]  = "scheduler";
    alert.labels["task_id"]    = task.id;
    alert.labels["task_name"]  = task.name;
    alert.labels["alertname"]  = "TaskSlaBreached";
    alert.labels["severity"]   = "warning";

    alert.annotations["elapsed_ms"]      = std::to_string(static_cast<int64_t>(elapsed_ms));
    alert.annotations["sla_deadline_ms"] = std::to_string(static_cast<int64_t>(sla_ms));

    // sendAlert() may involve network I/O; called outside the lock
    auto result = am->sendAlert(alert);
    if (result) {
        THEMIS_WARN("SLA breach alert fired for task {} ({}): elapsed={:.0f}ms, deadline={}ms",
                    task.id, task.name, elapsed_ms, static_cast<int64_t>(sla_ms));
    } else {
        THEMIS_ERROR("Failed to send SLA breach alert for task {}: {}",
                     task.id, result.error().message());
    }
}

void TaskScheduler::resolveTaskFailureAlert(const std::string& task_id) {
    // Take a copy of the alertmanager pointer and the alert ID under the lock,
    // then release the lock before calling resolveAlert() (potential I/O).
    std::shared_ptr<observability::Alertmanager> am;
    std::string alert_id;
    {
        std::unique_lock<std::shared_mutex> lock(alert_mutex_);
        if (!alertmanager_) {
            return;
        }

        auto it = active_failure_alert_ids_.find(task_id);
        if (it == active_failure_alert_ids_.end()) {
            return;  // No active failure alert for this task
        }

        am       = alertmanager_;
        alert_id = it->second;
        active_failure_alert_ids_.erase(it);
    }

    // resolveAlert() may involve network I/O; called outside the lock
    auto result = am->resolveAlert(alert_id);
    if (result) {
        THEMIS_INFO("Task failure alert resolved for task {} (alert_id={})", task_id, alert_id);
    } else {
        THEMIS_WARN("Failed to resolve task failure alert for task {}: {}",
                    task_id, result.error().message());
    }
}

// ===== Dynamic scaling (Issue #2269) =====

size_t TaskScheduler::getQueueDepth() const noexcept {
    return queue_depth_.load();
}

size_t TaskScheduler::getDynamicConcurrencyLimit() const noexcept {
    return dynamic_limit_.load();
}

void TaskScheduler::adjustConcurrencyLimit([[maybe_unused]] size_t pending_count) noexcept {
    if (!config_.enable_dynamic_scaling) return;

    queue_depth_.store(pending_count);

    const size_t floor_limit = std::max(config_.min_concurrent_tasks, size_t{1});
    const size_t ceil_limit  = std::max(config_.max_concurrent_tasks_ceil, floor_limit);
    size_t cur = dynamic_limit_.load();

    if (pending_count >= config_.scale_up_queue_depth) {
        // Scale up by 1 slot, capped at ceiling
        idle_ticks_.store(0);
        size_t next = std::min(cur + 1, ceil_limit);
        if (next != cur) {
            dynamic_limit_.store(next);
            THEMIS_INFO("TaskScheduler: scaled up concurrency limit {} → {} (queue_depth={})",
                        cur, next, pending_count);
        }
    } else if (pending_count == 0) {
        size_t idle = idle_ticks_.fetch_add(1) + 1;
        if (idle >= config_.scale_down_idle_ticks && cur > floor_limit) {
            size_t next = cur - 1;
            dynamic_limit_.store(next);
            idle_ticks_.store(0);
            THEMIS_INFO("TaskScheduler: scaled down concurrency limit {} → {} (idle_ticks={})",
                        cur, next, idle);
        }
    } else {
        // Pending > 0 but below scale_up_queue_depth – stay at current limit
        idle_ticks_.store(0);
    }
}

// ===== Task Execution Serialization (Phase 3 Hardening) =====

std::mutex& TaskScheduler::getTaskExecutionLock(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(task_locks_mutex_);
    
    auto it = task_execution_locks_.find(task_id);
    if (it == task_execution_locks_.end()) {
        // Lazily create a new mutex for this task
        task_execution_locks_[task_id] = std::make_unique<std::mutex>();
        it = task_execution_locks_.find(task_id);
    }
    
    return *it->second;
}

} // namespace themis


