/**
 * @file task_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 90/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <optional>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "cdc/changefeed.h"
#include "scheduler/scheduler_api_contract.h"
#include "scheduler/task_audit_event.h"
#include "scheduler/task_result_store.h"
#include "observability/alertmanager.h"

namespace themis {

// Forward declarations
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;
class EventTriggerManager;
class CronExpression;
class RocksDBWrapper;

namespace utils {
    class AuditLogger;
}

namespace scheduler {
    class TaskAuditManager;
}

/**
 * @brief Represents a scheduled task with AQL query or custom function
 */
struct ScheduledTask {
    std::string id;                           // Unique task identifier
    std::string name;                         // Human-readable name
    std::string description;                  // Task description
    
    // Task execution configuration
    enum class TaskType {
        AQL_QUERY,      // Execute an AQL query
        FUNCTION        // Execute a registered function
    } type = TaskType::AQL_QUERY;
    
    std::string aql_query;                    // AQL query to execute (if type == AQL_QUERY)
    std::string function_name;                // Function name (if type == FUNCTION)
    nlohmann::json parameters;                // Parameters for the task
    
    // Scheduling configuration
    enum class TriggerType {
        CRON,         // Cron-based scheduling
        INTERVAL,     // Fixed interval (existing)
        CDC_EVENT,    // CDC event-based trigger
        WEBHOOK,      // External HTTP events (future)
        MANUAL        // Only manual execution
    } trigger_type = TriggerType::INTERVAL;  // Default: maintain backward compatibility
    
    // CRON-based scheduling
    std::string cron_expression;  // e.g., "0 9-17 * * 1-5" for weekdays 9-17h
    
    // INTERVAL-based scheduling (existing)
    std::chrono::milliseconds interval{std::chrono::minutes(5)};  // Fixed interval
    std::chrono::system_clock::time_point next_run;                // Next scheduled run
    int64_t last_run_ms = 0;                                       // Timestamp of last run
    
    // CDC Event-based trigger configuration
    struct CDCTrigger {
        std::string key_prefix;               // Key prefix filter (e.g., "users:")
        std::set<int> event_types;            // Event types (0=PUT, 1=DELETE, etc.)
        std::optional<std::string> condition; // Optional AQL filter
        uint32_t debounce_ms = 0;            // Event debouncing in ms
    } cdc_trigger;
    
    // Hybrid trigger logic (when both time and event triggers are active)
    enum class TriggerLogic {
        OR,   // Execute on ANY trigger (time OR event)
        AND   // Execute only when BOTH triggers are satisfied
    } trigger_logic = TriggerLogic::OR;
    
    // Priority for event-based tasks
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2
    } priority = Priority::NORMAL;
    
    // Task state
    bool enabled = true;
    bool running = false;  // Currently executing
    bool allow_concurrent = false; // Allow overlapping executions (legacy compatibility)

    // ── Starvation prevention via aging ──────────────────────────────────
    /// How many consecutive scheduler ticks this task was ready-but-skipped
    /// (due to the concurrency limit being reached).  When this counter
    /// reaches the configured `aging_threshold` the scheduler temporarily
    /// treats the task as having NORMAL priority (LOW→NORMAL) or HIGH
    /// priority (NORMAL→HIGH) so that it cannot be starved indefinitely by
    /// higher-priority tasks.  The counter is reset to 0 when the task is
    /// dispatched or when the task is disabled.
    uint32_t consecutive_skips = 0;

    // ── Error categorization ──────────────────────────────────────────────
    /**
     * @brief Classification of the most recent execution failure.
     *
     * NONE          – no failure / task has never failed
     * TRANSIENT     – intermittent error; retry may succeed
     *                 (network timeout, temporary resource exhaustion)
     * PERMANENT     – error that will not recover without code/config change
     *                 (e.g. invalid AQL, function not found)
     * TIMEOUT       – the task exceeded its configured timeout
     * RESOURCE      – the task was rejected due to resource / rate limits
     * SECURITY      – rejected by security validation
     */
    enum class ErrorCategory {
        NONE,
        TRANSIENT,
        PERMANENT,
        TIMEOUT,
        RESOURCE,
        SECURITY
    };

    // Statistics
    size_t total_executions = 0;
    size_t successful_executions = 0;
    size_t failed_executions = 0;
    double avg_execution_time_ms = 0.0;
    std::string last_error;
    ErrorCategory last_error_category = ErrorCategory::NONE;
    std::chrono::system_clock::time_point last_success_time;
    std::chrono::system_clock::time_point last_failure_time;
    
    // Resource limits
    std::chrono::milliseconds timeout{std::chrono::minutes(10)};  // Execution timeout
    size_t max_retries = 3;                                        // Max retry attempts on failure (legacy; overridden by retry_policy when set)

    /**
     * @brief Optional SLA deadline for task execution.
     *
     * When set, an SLA breach alert is fired if the task execution time exceeds
     * this duration.  The task is not interrupted; the alert is purely a
     * notification.  Unset (nullopt) means no SLA monitoring for this task.
     */
    std::optional<std::chrono::milliseconds> sla_deadline;

    /**
     * @brief Retry strategy for failed task executions
     */
    enum class RetryStrategy {
        NONE,                // No retries (equivalent to max_retries = 0)
        FIXED_DELAY,         // Fixed delay between retries
        EXPONENTIAL_BACKOFF, // 1s, 2s, 4s, 8s, ... (capped by max_delay)
        LINEAR_BACKOFF,      // initial_delay, 2*initial_delay, 3*initial_delay, ...
        JITTER_BACKOFF,      // Exponential backoff with ±jitter_factor random jitter
        FIBONACCI_BACKOFF    // initial * fib(attempt+1): 1×, 1×, 2×, 3×, 5×, 8×, ... (capped by max_delay)
    };

    /**
     * @brief Per-task retry configuration
     *
     * When retry_policy is set, it takes precedence over max_retries.
     * Leave unset to use the legacy max_retries + exponential backoff behaviour.
     */
    struct RetryPolicy {
        RetryStrategy strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
        size_t max_retries = 3;
        std::chrono::milliseconds initial_delay{1000};  // Delay before first retry
        std::chrono::milliseconds max_delay{30000};     // Upper cap on delay
        double backoff_multiplier = 2.0;                // For exponential / linear
        double jitter_factor = 0.1;                     // ±fraction for jitter (0.1 = ±10%)

        // Optional: return false to skip retry for a given error message
        std::function<bool(const std::string& error)> should_retry;
    };

    std::optional<RetryPolicy> retry_policy;  // Advanced retry configuration (optional)

    /**
     * @brief SLO-based adaptive retry configuration (Phase 5, v1.9.0).
     *
     * When set together with @c sla_deadline, the scheduler adapts retry behaviour
     * to avoid spending the full SLA budget on retries and prevent cascading SLO
     * violations.
     *
     * Adaptive rules applied before each retry delay:
     *  1. If elapsed task time + computed retry delay > sla_deadline *
     *     slo_budget_fraction, the retry delay is clamped to the remaining SLA
     *     budget fraction (minimum 0 ms — i.e. retry immediately).
     *  2. If the remaining SLA budget (sla_deadline − elapsed) ≤ 0, all further
     *     retries are skipped immediately — the task has already exceeded its SLO.
     *  3. When the rolling SLO compliance rate (over the last @c slo_history_window
     *     executions) drops below @c slo_compliance_threshold, the effective
     *     max_retries is clamped to @c min_retries_under_pressure.
     *
     * Requires @c sla_deadline to be set; ignored (no-op) otherwise.
     */
    struct SloRetryConfig {
        /// Enable SLO-based retry adaptation.
        bool slo_aware = true;

        /// Maximum fraction of @c sla_deadline that may be consumed by retry
        /// delays.  E.g. 0.5 means retries can use at most 50 % of the SLA
        /// budget, leaving the rest for actual task execution.
        double slo_budget_fraction = 0.5;

        /// When the rolling SLO compliance rate drops below this threshold,
        /// max_retries is clamped to @c min_retries_under_pressure.
        double slo_compliance_threshold = 0.8;

        /// Minimum number of retries always allowed, even under SLO pressure.
        size_t min_retries_under_pressure = 1;

        /// Rolling window size (number of recent executions) used for compliance
        /// tracking.  Set to 0 to disable history-based retry reduction.
        size_t slo_history_window = 20;
    };

    /// Optional SLO-aware adaptive retry configuration.
    /// Requires @c sla_deadline to be set; silently ignored otherwise.
    std::optional<SloRetryConfig> slo_retry_config;

    // SLO compliance tracking (updated atomically after each execution; protected
    // by tasks_mutex_ in the scheduler).
    size_t slo_violations       = 0;  ///< Cumulative SLO violations in current window
    size_t slo_window_count     = 0;  ///< Total executions tracked in current window

    // Task dependency configuration
    std::vector<std::string> dependencies;  // IDs of tasks that must complete before this task runs

    // Hooks for notifications (optional)
    std::function<void(const std::string& task_id, const nlohmann::json& result)> on_success;
    std::function<void(const std::string& task_id, const std::string& error)> on_failure;

    /**
     * @brief Optional predicate for conditional branching in DAG execution.
     *
     * When set, this function is evaluated AFTER all dependency tasks complete
     * successfully and BEFORE this task is dispatched for execution.
     *
     * @param dep_results Map of { dependency_task_id -> result JSON } for every
     *                    dependency that succeeded in the current DAG execution.
     * @return true  – execute this task normally.
     * @return false – skip this task (conditional skip); the task appears in
     *                 DagExecutionResult::condition_skipped and its dependents
     *                 are also condition-skipped transitively.
     *
     * When not set the task executes whenever its dependencies complete
     * successfully (existing behaviour, fully backward-compatible).
     */
    std::function<bool(const std::map<std::string, nlohmann::json>& dep_results)> branch_condition;
};

/**
 * @brief Task scheduler for periodic execution of AQL queries and functions
 * 
 * ⚠️ SECURITY WARNING: This component executes arbitrary code and queries.
 * Production deployments MUST implement:
 * 1. RBAC - Only authorized users can register/modify tasks
 * 2. Query validation - Sanitize AQL queries before execution
 * 3. Resource limits - CPU time, memory, I/O quotas per task
 * 4. Audit logging - Log all task operations (create, modify, execute, delete)
 * 5. Isolation - Run tasks in sandboxed execution contexts
 * 6. Encryption - Encrypt task definitions at rest if they contain sensitive data
 * 7. Rate limiting - Prevent task flooding and abuse
 * 
 * Features:
 * - Cron-like scheduling with configurable intervals
 * - AQL query execution with full query engine integration
 * - Custom function registration and execution
 * - Task status monitoring and statistics
 * - Graceful shutdown with running task completion
 * - Concurrent task execution with resource limits
 * - Task persistence (optional, for recovery after restart)
 * 
 * Usage Example:
 * @code
 *   TaskScheduler scheduler(query_engine);
 *   
 *   // Schedule a data compression task
 *   ScheduledTask compression_task;
 *   compression_task.id = "compress_old_data";
 *   compression_task.name = "Compress Old Time Series Data";
 *   compression_task.type = ScheduledTask::TaskType::AQL_QUERY;
 *   compression_task.aql_query = "FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 1, 'day') "
 *                                 "UPDATE d WITH { compressed: true } IN timeseries";
 *   compression_task.interval = std::chrono::hours(6);
 *   
 *   scheduler.registerTask(compression_task);
 *   scheduler.start();
 *   
 *   // ... scheduler runs in background ...
 *   
 *   scheduler.stop();
 * @endcode
 * 
 * System Impact:
 * - CPU: Minimal overhead for scheduler loop, actual impact depends on scheduled tasks
 * - Memory: ~1KB per registered task, plus task execution memory
 * - I/O: No direct I/O overhead, depends on executed queries/functions
 * - Concurrency: Configurable max parallel tasks to limit resource usage
 * - Write Path: No direct impact on write path, tasks run asynchronously
 * 
 * ⚠️ SECURITY RISKS:
 * - Arbitrary code execution via custom functions
 * - SQL injection-like attacks via malicious AQL queries
 * - Resource exhaustion (CPU, memory, disk) from malicious tasks
 * - Privilege escalation if tasks run with elevated permissions
 * - Data exfiltration via scheduled queries
 * - DoS through task flooding or infinite loops
 * - Sensitive data exposure in task definitions or logs
 */
class TaskScheduler {
public:
    /**
     * @brief Configuration for the task scheduler
     */
    struct Config {
        size_t max_concurrent_tasks = 4;      // Max tasks running in parallel
        std::chrono::milliseconds check_interval{std::chrono::seconds(10)};  // Scheduler tick interval
        bool persist_tasks = false;            // Save tasks to disk for recovery
        std::string persistence_path = "data/tasks";  // Path for task persistence
        bool allow_task_overlap = false;       // Allow same task to run concurrently
        
        // Audit and anomaly detection
        bool enable_audit_logging = true;      // Enable comprehensive audit logging
        bool enable_anomaly_detection = true;  // Enable anomaly detection
        bool enable_gdpr_mode = false;         // Enable GDPR-compliant data masking
        /// Path for the audit log JSONL file. Empty string = use TaskAuditManager default.
        std::string audit_log_path;

        // Result store configuration
        bool   enable_result_store = false;         // Store task output in ThemisDB after each run
        size_t result_store_max_results_per_task = 100;  // Max results retained per task

        // Dynamic scaling (Issue #2269)
        // When enabled, max_concurrent_tasks acts as the initial (and minimum) limit.
        // The scheduler automatically scales up to max_concurrent_tasks_ceil when the
        // pending queue exceeds scale_up_queue_depth, and scales back down after
        // scale_down_idle_ticks consecutive ticks with no pending tasks.
        bool   enable_dynamic_scaling      = false; ///< Enable automatic concurrency scaling
        size_t min_concurrent_tasks        = 1;     ///< Minimum worker slots (floor for scaling)
        size_t max_concurrent_tasks_ceil   = 16;    ///< Maximum worker slots (ceiling for scaling)
        size_t scale_up_queue_depth        = 2;     ///< Pending tasks threshold to trigger scale-up
        size_t scale_down_idle_ticks       = 3;     ///< Consecutive idle ticks before scale-down

        // Sandboxed execution
        bool sandbox_execution = false; ///< When true, wrap user-provided task functions in ModuleSandbox

        // Starvation prevention via aging (Issue #1928 / scheduler/FUTURE_ENHANCEMENTS.md)
        // When a task is ready-but-skipped (concurrency limit reached) for
        // `aging_threshold` consecutive ticks its effective priority is boosted
        // by one level (LOW→NORMAL, NORMAL→HIGH) until it is dispatched.
        // Set to 0 to disable aging.
        uint32_t aging_threshold = 5; ///< Ticks before a skipped task's priority is boosted
    };

    /**
     * @brief Per-request authentication context propagated via thread-local storage.
     *
     * HTTP handler code sets this on the handler thread before calling scheduler
     * operations. The scheduler reads it when constructing audit events so that
     * audit trails correctly attribute operations to the requesting operator rather
     * than the system account.
     */
    struct RequestContext {
        std::string user_id;    ///< Authenticated user / service account
        std::string client_ip;  ///< Originating client IP address (may be empty)
        std::unordered_set<std::string> granted_permissions; ///< Effective permissions/scopes for this request
        std::unordered_set<std::string> roles;               ///< Effective roles/groups for this request
        std::string authorization_justification;             ///< Why access was granted (policy/scope decision)
    };

    /// Set the authentication context for the calling thread.
    /// Must be called before any scheduler method that performs audit logging.
    /// Thread-safe (each thread owns its own context slot).
    static void setRequestContext(const RequestContext& ctx) noexcept;

    /// Clear the authentication context for the calling thread.
    static void clearRequestContext() noexcept;

    /// Return the user ID from the thread-local request context, or @p fallback.
    static std::string currentUserId(const char* fallback = "system") noexcept;

    /// Return the client IP from the thread-local request context (empty if not set).
    static std::string currentClientIp() noexcept;
    static bool hasPermission(const std::string& permission) noexcept;
    static bool hasRole(const std::string& role) noexcept;
    static std::string currentAuthorizationJustification(const char* fallback = "") noexcept;
    
    /**
     * @brief Construct a task scheduler
     * @param query_engine   Query engine for executing AQL queries
     * @param config         Scheduler configuration
     * @param changefeed     Optional changefeed for CDC event triggers (nullptr = no CDC support)
     * @param audit_logger   Optional audit logger for tamper-evident logging (nullptr = basic logging)
     * @param result_storage Optional RocksDB instance used to persist task execution results.
     *                       Required when config.enable_result_store == true.
     *                       Must outlive this TaskScheduler instance.
     *
     * Note: The optional parameters maintain backward compatibility.
     * Existing code using TaskScheduler(query_engine, config) continues to work.
     * New code can add changefeed for CDC event trigger support, audit_logger for comprehensive
     * auditing, and result_storage for persistent task output storage.
     */
    explicit TaskScheduler(QueryEngine* query_engine, 
                          const Config& config,
                          Changefeed* changefeed = nullptr,
                          std::shared_ptr<utils::AuditLogger> audit_logger = nullptr,
                          RocksDBWrapper* result_storage = nullptr);
    ~TaskScheduler();
    
    // Lifecycle management
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Task registration and management
    /**
     * @brief Register a new scheduled task
     * @param task Task configuration
     * @return Task ID
     * 
     * ⚠️ SECURITY: This method MUST be protected by authentication and authorization.
     * Validate and sanitize all task inputs before registration.
     */
    std::string registerTask(const ScheduledTask& task);
    
    /**
     * @brief Unregister a task
     * @param task_id Task ID to remove
     */
    void unregisterTask(const std::string& task_id);
    
    /**
     * @brief Enable a disabled task
     * @param task_id Task ID to enable
     */
    void enableTask(const std::string& task_id);
    
    /**
     * @brief Disable a task (will not be executed until re-enabled)
     * @param task_id Task ID to disable
     */
    void disableTask(const std::string& task_id);
    
    /**
     * @brief Update an existing task
     * @param task Updated task configuration (ID must match existing task)
     * 
     * ⚠️ SECURITY: This method MUST be protected by authentication and authorization.
     * Verify user has permission to modify the specified task.
     */
    void updateTask(const ScheduledTask& task);
    
    // Manual execution
    /**
     * @brief Execute a task immediately (out-of-schedule)
     * @param task_id Task ID to execute
     * @return Execution result as JSON
     * 
     * ⚠️ SECURITY: This method MUST be protected by authentication and authorization.
     * Can be abused for DoS attacks or unauthorized data access.
     */
    nlohmann::json executeTaskNow(const std::string& task_id);

    /**
     * @brief Result of a DAG execution
     */
    struct DagExecutionResult {
        std::map<std::string, nlohmann::json> succeeded;  // task_id -> result
        std::map<std::string, std::string>    failed;     // task_id -> error message
        std::vector<std::string>              skipped;    // task_ids skipped due to failed deps
        std::vector<std::string>              condition_skipped;  // task_ids skipped because branch_condition returned false (or a transitive dep was condition-skipped)
    };

    /**
     * @brief Execute a set of registered tasks respecting their dependency order.
     *
     * Tasks are executed in topological order derived from each task's
     * `dependencies` list.  Tasks whose dependencies have all succeeded are
     * dispatched in parallel (up to max_concurrent_tasks).  If a task fails,
     * all tasks that (transitively) depend on it are skipped rather than
     * executed.  If a task's `branch_condition` predicate returns false, the
     * task is condition-skipped and its dependents are condition-skipped
     * transitively (reported in DagExecutionResult::condition_skipped).
     *
     * @param task_ids  IDs of the tasks to include in this DAG execution.
     *                  Tasks not in this set are ignored even if they appear
     *                  in a dependency list.
     * @return DagExecutionResult with per-task outcomes (succeeded, failed,
     *         skipped, condition_skipped).
     * @throws std::invalid_argument if task_ids contains an unknown task ID.
     * @throws std::runtime_error    if the dependency graph contains a cycle.
     *
     * ⚠️ SECURITY: This method MUST be protected by authentication and authorization.
     */
    DagExecutionResult executeDAG(const std::vector<std::string>& task_ids);
    
    // Function registration (for custom post-processing logic)
    using TaskFunction = std::function<nlohmann::json(const nlohmann::json& params)>;
    
    /**
     * @brief Register a custom function that can be called by tasks
     * @param name Function name
     * @param func Function implementation
     * 
     * ⚠️ SECURITY CRITICAL: This allows arbitrary code execution.
     * Only allow registration by system administrators.
     * Functions should be sandboxed and resource-limited.
     */
    void registerFunction(const std::string& name, TaskFunction func);
    
    /**
     * @brief Unregister a custom function
     * @param name Function name
     */
    void unregisterFunction(const std::string& name);
    
    // Status and statistics
    struct Stats {
        size_t registered_tasks = 0;
        size_t active_tasks = 0;
        size_t running_tasks = 0;
        size_t total_executions = 0;
        size_t failed_executions = 0;
        std::chrono::system_clock::time_point last_run;
        std::chrono::system_clock::time_point next_run;
    };
    
    /**
     * @brief Get scheduler statistics
     */
    Stats getStats() const;

    /**
     * @brief Export current scheduler metrics in Prometheus text format
     *
     * Returns a string in Prometheus exposition format (text/plain; version=0.0.4)
     * suitable for scraping by a Prometheus server or any compatible monitoring tool.
     *
     * Metrics exported:
     *   - themis_scheduler_tasks_registered        (gauge)
     *   - themis_scheduler_tasks_active            (gauge)
     *   - themis_scheduler_tasks_running           (gauge)
     *   - themis_scheduler_executions_total        (counter, label: status=success|failure)
     *   - themis_scheduler_task_executions_total   (counter, per task, labels: task_id, task_name, status)
     *   - themis_scheduler_task_execution_duration_ms (gauge, per task)
     *   - themis_scheduler_task_last_run_timestamp (gauge, per task, unix seconds)
     *
     * @return Prometheus text exposition string (never empty)
     */
    std::string exportMetrics() const;

    /**
     * @brief List all registered tasks
     */
    std::vector<ScheduledTask> listTasks() const;
    
    /**
     * @brief Get details of a specific task
     * @param task_id Task ID
     * @return Task details or nullptr if not found
     */
    std::shared_ptr<ScheduledTask> getTask(const std::string& task_id) const;
    
    /**
     * @brief Get audit manager for querying audit events
     * @return Shared pointer to audit manager (may be nullptr if audit logging disabled)
     */
    std::shared_ptr<scheduler::TaskAuditManager> getAuditManager() const {
        return audit_manager_;
    }

    /**
     * @brief Get execution history for a specific task (or all tasks)
     *
     * Convenience wrapper around TaskAuditManager::queryAuditEvents() that
     * pre-populates the task_id filter and sensible defaults for browsing
     * the searchable audit log.
     *
     * @param task_id  Task ID to filter on (empty string = all tasks)
     * @param limit    Maximum number of results to return (default 100)
     * @param offset   Pagination offset (default 0)
     * @return Vector of audit events ordered by timestamp descending,
     *         or empty vector if audit logging is disabled
     */
    std::vector<scheduler::TaskAuditEvent> getExecutionHistory(
        const std::string& task_id = "",
        size_t limit = 100,
        size_t offset = 0) const;

    /**
     * @brief Retrieve recent execution results for a task from the result store.
     *
     * Returns up to `limit` results, newest first.
     * Returns an empty vector if result storage is disabled or no results exist.
     *
     * @param task_id  Task identifier.
     * @param limit    Maximum number of records to return (default: 10).
     */
    std::vector<scheduler::TaskExecutionResult> getTaskResults(
        const std::string& task_id, size_t limit = 10) const;

    /**
     * @brief Retrieve the most-recent execution result for a task.
     *
     * Returns std::nullopt if result storage is disabled or no results exist.
     *
     * @param task_id  Task identifier.
     */
    std::optional<scheduler::TaskExecutionResult> getLatestTaskResult(
        const std::string& task_id) const;

    /**
     * @brief Set the alertmanager for dispatching task failure and SLA breach alerts.
     *
     * When set, the scheduler fires an alert via the alertmanager whenever:
     * - A task fails all execution attempts (TaskFailure alert).
     * - A task execution exceeds its configured sla_deadline (TaskSlaBreached alert).
     * A previously-fired failure alert is automatically resolved when the same
     * task subsequently succeeds.
     *
     * Pass nullptr to disable alertmanager integration (default behaviour).
     */
    void setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager);

    /**
     * @brief Get the currently configured alertmanager (may be nullptr).
     */
    std::shared_ptr<observability::Alertmanager> getAlertmanager() const;

    /**
     * @brief Get the number of tasks that were ready to run on the last scheduler
     *        tick but could not be dispatched because the concurrency limit was
     *        reached.
     *
     * Always returns 0 when @c enable_dynamic_scaling is false.
     */
    size_t getQueueDepth() const noexcept;

    /**
     * @brief Get the current effective max-concurrent-tasks limit.
     *
     * When @c enable_dynamic_scaling is false this equals
     * @c Config::max_concurrent_tasks.  When scaling is enabled it reflects
     * the dynamically adjusted value in the range
     * [@c min_concurrent_tasks, @c max_concurrent_tasks_ceil].
     */
    size_t getDynamicConcurrencyLimit() const noexcept;

private:
    // Core components
    QueryEngine* query_engine_;
    Changefeed* changefeed_;
    utils::AuditLogger* audit_logger_;  // Optional audit logger for SIEM integration
    Config config_;
    
    // Audit and anomaly detection
    std::shared_ptr<scheduler::TaskAuditManager> audit_manager_;

    // Alertmanager for task failure and SLA breach alerts (optional)
    std::shared_ptr<observability::Alertmanager> alertmanager_;
    mutable std::shared_mutex alert_mutex_;
    // Tracks active failure alert IDs per task: task_id -> alert_id
    std::map<std::string, std::string> active_failure_alert_ids_;

    // Dynamic scaling state (Issue #2269)
    std::atomic<size_t> dynamic_limit_{4};     // Current effective concurrency limit
    std::atomic<size_t> queue_depth_{0};       // Pending tasks on last tick
    std::atomic<size_t> idle_ticks_{0};        // Consecutive ticks with no pending tasks

    // Execution result store (optional – only active when enable_result_store == true)
    std::unique_ptr<scheduler::TaskResultStore> result_store_;
    
    // Function registry
    std::map<std::string, TaskFunction> functions_;
    
    // Task storage
    std::map<std::string, std::shared_ptr<ScheduledTask>> tasks_;
    mutable std::mutex tasks_mutex_;
    
    // Event trigger manager (for CDC events)
    std::unique_ptr<EventTriggerManager> event_trigger_manager_;
    
    // Cron expression cache
    std::map<std::string, std::shared_ptr<CronExpression>> cron_expressions_;
    
    // Scheduler thread
    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;
    std::condition_variable cv_;
    
    // Running tasks tracking
    std::map<std::string, std::thread> running_task_threads_;
    mutable std::mutex running_mutex_;
    std::atomic<size_t> active_task_threads_{0};
    
    // Per-task execution serialization locks (Phase 3 hardening)
    // Prevents concurrent execution of the same task. Lazily initialized.
    mutable std::map<std::string, std::unique_ptr<std::mutex>> task_execution_locks_;
    mutable std::mutex task_locks_mutex_;  ///< Protects task_execution_locks_ map itself
    
    // Statistics
    std::atomic<size_t> total_executions_{0};
    std::atomic<size_t> failed_executions_{0};
    std::chrono::system_clock::time_point last_run_;
    
    // Scheduler loop
    void schedulerLoop();
    
    // Task execution
    void executeTask(std::shared_ptr<ScheduledTask> task);
    nlohmann::json executeAqlQuery(const std::string& aql);
    nlohmann::json executeFunction(const std::string& name, const nlohmann::json& params);
    
    // Scheduling logic
    bool shouldExecute(const ScheduledTask& task, const std::chrono::system_clock::time_point& now) const;
    void updateNextRun(ScheduledTask& task);
    bool shouldExecuteCron(const ScheduledTask& task, const std::chrono::system_clock::time_point& now) const;
    
    // Event trigger management
    void setupEventTrigger(std::shared_ptr<ScheduledTask> task);
    void removeEventTrigger(const std::string& task_id);
    void onCDCEvent(std::shared_ptr<ScheduledTask> task, const Changefeed::ChangeEvent& event);
    
    // Cron expression management
    std::shared_ptr<CronExpression> getCronExpression(const std::string& task_id);
    void updateCronExpression(const std::string& task_id, const std::string& expression);
    
    // Persistence (optional)
    void saveTasks();
    void loadTasks();
    
    // Helpers
    int64_t getCurrentTimeMs() const;
    std::string generateTaskId(const ScheduledTask& task) const;
    
    // Security & Validation helpers
    void validateAqlQuery(const std::string& aql) const;
    void validateResourceLimits(const ScheduledTask& task) const;
    void validateCronExpression(const std::string& expression) const;
    void validateCDCTrigger(const ScheduledTask::CDCTrigger& trigger) const;
    ScheduledTask sanitizeTask(const ScheduledTask& task) const;
    void enforceQueryComplexityLimits(const std::string& aql) const;
    bool checkRateLimit(const std::string& task_id);  // Non-const since it logs security events

    // DAG execution helpers
    // Returns tasks in topological order (dependencies first).
    // Throws std::runtime_error if a cycle is detected.
    std::vector<std::string> topologicalSort(
        const std::vector<std::string>& task_ids,
        const std::map<std::string, std::vector<std::string>>& adj) const;

    // Alertmanager helpers
    void fireTaskFailureAlert(const ScheduledTask& task, const std::string& error);
    void fireTaskSlaBreachAlert(const ScheduledTask& task, double elapsed_ms);
    void resolveTaskFailureAlert(const std::string& task_id);
    static std::string makeTaskAlertId(const std::string& task_id, const std::string& alert_type);

    // Dynamic scaling helper (Issue #2269)
    void adjustConcurrencyLimit(size_t pending_count) noexcept;
    
    // Task execution serialization (Phase 3 hardening)
    // Returns the per-task execution lock, creating it lazily if needed.
    std::mutex& getTaskExecutionLock(const std::string& task_id);
};

} // namespace themis

