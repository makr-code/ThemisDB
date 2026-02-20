/**
 * @file task_scheduler.h
 * @brief Generic task scheduler for ThemisDB with AQL query execution support
 * 
 * ⚠️ SECURITY RISK: This implementation allows arbitrary AQL query execution
 * and custom function calls on a schedule. It requires careful security controls:
 * - Authentication and authorization for task registration/modification
 * - Query validation and sanitization to prevent injection attacks
 * - Resource limits (CPU, memory, I/O) to prevent DoS
 * - Audit logging for all task operations
 * - Encrypted storage of task definitions containing sensitive data
 * - Isolation and sandboxing of task execution contexts
 * 
 * Provides a cron-like task scheduling system that can execute AQL queries
 * and functions on a schedule. Designed for post-processing operations such as:
 * - Data compression and batch optimization after RocksDB storage
 * - Periodic data aggregation and rollups
 * - Data cleanup and maintenance tasks
 * - Custom post-processing workflows
 */

#ifndef THEMIS_TASK_SCHEDULER_H
#define THEMIS_TASK_SCHEDULER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class QueryEngine;
class Changefeed;
class EventTriggerManager;
class CronExpression;

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
    
    // Statistics
    size_t total_executions = 0;
    size_t successful_executions = 0;
    size_t failed_executions = 0;
    double avg_execution_time_ms = 0.0;
    std::string last_error;
    std::chrono::system_clock::time_point last_success_time;
    std::chrono::system_clock::time_point last_failure_time;
    
    // Resource limits
    std::chrono::milliseconds timeout{std::chrono::minutes(10)};  // Execution timeout
    size_t max_retries = 3;                                        // Max retry attempts on failure (legacy; overridden by retry_policy when set)

    /**
     * @brief Retry strategy for failed task executions
     */
    enum class RetryStrategy {
        NONE,                // No retries (equivalent to max_retries = 0)
        FIXED_DELAY,         // Fixed delay between retries
        EXPONENTIAL_BACKOFF, // 1s, 2s, 4s, 8s, ... (capped by max_delay)
        LINEAR_BACKOFF,      // initial_delay, 2*initial_delay, 3*initial_delay, ...
        JITTER_BACKOFF       // Exponential backoff with ±jitter_factor random jitter
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

    // Hooks for notifications (optional)
    std::function<void(const std::string& task_id, const nlohmann::json& result)> on_success;
    std::function<void(const std::string& task_id, const std::string& error)> on_failure;
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
    };
    
    /**
     * @brief Construct a task scheduler
     * @param query_engine Query engine for executing AQL queries
     * @param config Scheduler configuration
     * @param changefeed Optional changefeed for CDC event triggers (nullptr = no CDC support)
     * @param audit_logger Optional audit logger for tamper-evident logging (nullptr = basic logging)
     * 
     * Note: The optional parameters maintain backward compatibility.
     * Existing code using TaskScheduler(query_engine, config) continues to work.
     * New code can add changefeed for CDC event trigger support and audit_logger for comprehensive auditing.
     */
    explicit TaskScheduler(QueryEngine* query_engine, 
                          const Config& config,
                          Changefeed* changefeed = nullptr,
                          std::shared_ptr<utils::AuditLogger> audit_logger = nullptr);
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

private:
    // Core components
    QueryEngine* query_engine_;
    Changefeed* changefeed_;
    utils::AuditLogger* audit_logger_;  // Optional audit logger for SIEM integration
    Config config_;
    
    // Audit and anomaly detection
    std::shared_ptr<scheduler::TaskAuditManager> audit_manager_;
    
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
    void onCDCEvent(std::shared_ptr<ScheduledTask> task, const void* event);
    
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
};

} // namespace themis

#endif // THEMIS_TASK_SCHEDULER_H
