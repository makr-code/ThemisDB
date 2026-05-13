> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB Scheduler Module Headers

## Module Purpose

The Scheduler headers define the public interfaces for ThemisDB's task scheduling and automation system. These headers expose the scheduler's capabilities for cron-like task execution, periodic maintenance, backup scheduling, data retention policies, and distributed coordination without requiring clients to depend on implementation details.

## Scope

**In Scope:**
- Task scheduler interface definitions and abstractions
- Cron-like scheduling with configurable intervals
- AQL query execution and custom function scheduling
- Task lifecycle management (register, enable, disable, execute)
- Hybrid retention manager interfaces (3-stage data lifecycle)
- Task persistence and recovery APIs
- Task statistics and monitoring interfaces
- Priority-based scheduling abstractions
- Task dependency and DAG execution definitions

**Out of Scope:**
- Distributed coordination implementation details (use `DistributedTaskCoordinator` from this module's `distributed_task_coordinator.h`)
- Query execution logic (handled by query module)
- Storage operations (handled by storage module)
- Authentication and authorization (handled by auth module)
- Network protocols (handled by server module)

## Header Entry Points

| Header | Public responsibility |
|---|---|
| `task_scheduler.h` | Core scheduler lifecycle, task registration, DAG execution, metrics/export, retry/SLA controls |
| `hybrid_retention_manager.h` | Retention orchestration for Stage1/2/3 execution and status reporting |
| `distributed_task_coordinator.h` | Cluster leader election and scheduler role activation/deactivation |
| `external_scheduler_adapter.h` | Conversion and synchronization to/from Kubernetes CronJob and Airflow DAG specs |
| `event_trigger.h` | CDC/event trigger registration, callback handling, circuit breaker |
| `task_audit_manager.h` | Audit-event recording, filtered querying, and export |
| `task_audit_event.h` | Audit/security event structures and anomaly payload fields |
| `task_result_store.h` | Persistent per-task output storage and latest/history retrieval |
| `task_anomaly_detector.h` | Statistical anomaly detection for task execution patterns |

## Key Components

### Task Scheduler
**Location:** `task_scheduler.h`

Generic task scheduler for periodic execution of AQL queries and custom functions.

**Purpose:**
- Provides cron-like task scheduling system
- Enables automated data processing workflows
- Supports periodic maintenance operations
- Decouples scheduled operations from core server logic
- Enables backup, indexing, and cleanup automation

**Key Features:**
```cpp
class TaskScheduler {
public:
    struct Config {
        size_t max_concurrent_tasks = 4;
        std::chrono::milliseconds check_interval{std::chrono::seconds(10)};
        bool persist_tasks = false;
        std::string persistence_path = "data/tasks";
        bool allow_task_overlap = false;

        // Audit and anomaly detection
        bool enable_audit_logging = true;
        bool enable_anomaly_detection = true;
        bool enable_gdpr_mode = false;

        // Result store
        bool   enable_result_store = false;
        size_t result_store_max_results_per_task = 100;

        // Dynamic concurrency scaling (Issue #2269)
        bool   enable_dynamic_scaling    = false;
        size_t min_concurrent_tasks      = 1;
        size_t max_concurrent_tasks_ceil = 16;
        size_t scale_up_queue_depth      = 2;
        size_t scale_down_idle_ticks     = 3;
    };

    // Full constructor with optional CDC, audit-logger, and result-storage support
    explicit TaskScheduler(
        QueryEngine* query_engine,
        const Config& config,
        Changefeed* changefeed       = nullptr,
        std::shared_ptr<utils::AuditLogger> audit_logger = nullptr,
        RocksDBWrapper* result_storage = nullptr
    );

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;

    // Task management
    std::string registerTask(const ScheduledTask& task);
    void unregisterTask(const std::string& task_id);
    void enableTask(const std::string& task_id);
    void disableTask(const std::string& task_id);
    void updateTask(const ScheduledTask& task);

    // Manual and DAG execution
    nlohmann::json executeTaskNow(const std::string& task_id);
    DagExecutionResult executeDAG(const std::vector<std::string>& task_ids);

    // Function registration
    void registerFunction(const std::string& name, TaskFunction func);
    void unregisterFunction(const std::string& name);

    // Monitoring
    Stats getStats() const;
    std::string exportMetrics() const;  // Prometheus text format
    std::vector<ScheduledTask> listTasks() const;
    std::shared_ptr<ScheduledTask> getTask(const std::string& task_id) const;

    // Audit history
    std::shared_ptr<scheduler::TaskAuditManager> getAuditManager() const;
    std::vector<scheduler::TaskAuditEvent> getExecutionHistory(
        const std::string& task_id = "", size_t limit = 100, size_t offset = 0) const;

    // Result store
    std::vector<scheduler::TaskExecutionResult> getTaskResults(
        const std::string& task_id, size_t limit = 10) const;
    std::optional<scheduler::TaskExecutionResult> getLatestTaskResult(
        const std::string& task_id) const;

    // Alertmanager integration
    void setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager);
    std::shared_ptr<observability::Alertmanager> getAlertmanager() const;

    // Dynamic scaling accessors
    size_t getQueueDepth() const noexcept;
    size_t getDynamicConcurrencyLimit() const noexcept;
};
```

**Task Structure:**
```cpp
struct ScheduledTask {
    std::string id;
    std::string name;
    std::string description;

    enum class TaskType {
        AQL_QUERY,      // Execute an AQL query
        FUNCTION        // Execute a registered function
    } type = TaskType::AQL_QUERY;

    std::string aql_query;              // AQL query (if type == AQL_QUERY)
    std::string function_name;           // Function name (if type == FUNCTION)
    nlohmann::json parameters;           // Task parameters

    // Scheduling
    enum class TriggerType {
        CRON,      // Cron-based scheduling
        INTERVAL,  // Fixed interval
        CDC_EVENT, // CDC event-based trigger
        WEBHOOK,   // External HTTP events
        MANUAL     // Only manual execution
    } trigger_type = TriggerType::INTERVAL;

    std::string cron_expression;         // e.g. "0 2 * * *" (if CRON)
    std::chrono::milliseconds interval;  // Fixed interval (if INTERVAL)
    std::chrono::system_clock::time_point next_run;
    bool enabled = true;
    bool running = false;

    // Statistics
    size_t total_executions = 0;
    size_t successful_executions = 0;
    size_t failed_executions = 0;
    double avg_execution_time_ms = 0.0;
    std::string last_error;

    // Resource and SLA limits
    std::chrono::milliseconds timeout{std::chrono::minutes(10)};
    size_t max_retries = 3;                                // legacy; overridden by retry_policy
    std::optional<std::chrono::milliseconds> sla_deadline; // alert when exceeded

    // Advanced retry configuration
    std::optional<RetryPolicy> retry_policy;

    // DAG dependencies
    std::vector<std::string> dependencies;         // IDs of prerequisite tasks
    std::function<bool(const std::map<std::string, nlohmann::json>&)> branch_condition;

    // Callbacks
    std::function<void(const std::string&, const nlohmann::json&)> on_success;
    std::function<void(const std::string&, const std::string&)> on_failure;
};
```

**Design Patterns:**
- **Observer Pattern**: Task callbacks for success/failure notifications
- **Command Pattern**: Encapsulates scheduled operations as task objects
- **Template Method**: Scheduler loop with extensible task execution
- **Strategy Pattern**: Pluggable task types (AQL vs function)

**Thread Safety:**
- All public methods are thread-safe
- Internal locking protects task registry and execution state
- Tasks execute in separate threads (configurable concurrency)
- Graceful shutdown waits for running tasks to complete

**Security Considerations:**
⚠️ **CRITICAL SECURITY RISKS**:
- **Arbitrary Code Execution**: Tasks can execute any AQL query or registered function
- **SQL Injection-like Attacks**: Malicious AQL queries can access/modify any data
- **Resource Exhaustion**: Tasks can consume excessive CPU, memory, or I/O
- **Privilege Escalation**: Tasks run with scheduler's permissions
- **Data Exfiltration**: Scheduled queries can leak sensitive data

**Required Security Controls:**
1. **Authentication/Authorization**: Only authorized users can register/modify tasks
2. **Query Validation**: AST-based AQL injection detection with pattern matching
3. **Resource Limits**: CPU time, memory, I/O quotas per task
4. **Audit Logging**: Log all task operations (create, modify, execute, delete)
5. **Isolation**: Sandboxed execution contexts for tasks
6. **Encryption**: Encrypt task definitions at rest if they contain sensitive data
7. **Rate Limiting**: Prevent task flooding and abuse

### Hybrid Retention Manager
**Location:** `hybrid_retention_manager.h`

Three-stage hybrid data retention system combining compression, variance-based downsampling, and time-based aggregation.

**Purpose:**
- Automates data lifecycle management
- Reduces storage costs by 99.9% for time-series data
- Preserves anomalies and important events
- Maintains analytical capability while reducing granularity

**Three-Stage Strategy:**
```
Stage 1: Gorilla Compression (0-7 days)
  - Lossless compression of hot data
  - 10-20x compression ratio
  - Full resolution preserved
  - Daily execution

Stage 2: Adaptive Retention (7-365 days)
  - Variance-based downsampling
  - High CV (>20%): 1-minute resolution
  - Medium CV (5-20%): 15-minute resolution
  - Low CV (<5%): 1-hour resolution
  - Preserves anomalies via 3-sigma detection
  - Every 12 hours

Stage 3: Time-Based Retention (>365 days)
  - Daily aggregates for cold data
  - 1440x reduction (1s → 1d)
  - Daily execution
```

**Configuration Structure:**
```cpp
struct HybridRetentionConfig {
    struct Stage1Config {
        bool enabled = true;
        std::chrono::hours duration{24 * 7};
        std::chrono::hours check_interval{24};
        std::string metric_pattern = "*";
    } stage1;

    struct Stage2Config {
        bool enabled = true;
        std::chrono::hours min_age{24 * 7};
        std::chrono::hours max_age{24 * 365};
        std::chrono::hours check_interval{12};
        double low_cv_threshold = 5.0;
        double medium_cv_threshold = 20.0;
        std::string low_cv_resolution = "1h";
        std::string medium_cv_resolution = "15m";
        std::string high_cv_resolution = "1m";
        bool detect_anomalies = true;
        double anomaly_sigma_threshold = 3.0;
    } stage2;

    struct Stage3Config {
        bool enabled = true;
        std::chrono::hours min_age{24 * 365};
        std::chrono::hours check_interval{24};
        std::string target_resolution = "1d";
    } stage3;

    bool auto_cleanup = true;
    bool verify_aggregates = true;
    std::string source_table = "timeseries";
    std::string adaptive_table = "timeseries_adaptive";
    std::string longterm_table = "timeseries_longterm";
};
```

**Class Interface:**
```cpp
class HybridRetentionManager {
public:
    HybridRetentionManager(
        QueryEngine* query_engine,
        TSStore* tsstore,
        TaskScheduler* scheduler,
        const HybridRetentionConfig& config = {}
    );

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;

    // Configuration
    void updateConfig(const HybridRetentionConfig& config);
    HybridRetentionConfig getConfig() const;

    // Manual execution
    void executeStage1();  // Run Gorilla compression now
    void executeStage2();  // Run adaptive retention now
    void executeStage3();  // Run time-based retention now
    void executeAll();     // Run all stages now

    // Statistics
    HybridRetentionStats getStats() const;
    void resetStats();
    nlohmann::json getStatusReport() const;
};
```

**Statistics Structure:**
```cpp
struct HybridRetentionStats {
    struct {
        size_t compressions_total = 0;
        size_t compressions_failed = 0;
        double avg_compression_ratio = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage1;

    struct {
        size_t aggregations_total = 0;
        size_t aggregations_failed = 0;
        size_t anomalies_preserved = 0;
        double avg_storage_reduction = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage2;

    struct {
        size_t aggregations_total = 0;
        size_t aggregations_failed = 0;
        double avg_storage_reduction = 0.0;
        std::chrono::system_clock::time_point last_run;
    } stage3;

    size_t total_storage_bytes_saved = 0;
    double overall_storage_reduction_percent = 0.0;
};
```

**Benefits:**
- 99.9% storage reduction for time-series workloads
- Anomaly preservation maintains analytical value
- Fully automated lifecycle management
- Configurable thresholds per workload
- No data loss for important events

**Thread Safety:**
- All public methods are thread-safe
- Internal locking protects configuration and statistics
- Stage executions delegate to TaskScheduler's thread pool

## Integration Points

### With Query Module
- **TaskScheduler** executes AQL queries via QueryEngine
- Validates queries using AST-based injection detection
- Applies resource limits at query execution time

### With Storage Module
- **HybridRetentionManager** uses TSStore for time-series access
- Coordinates with RocksDBWrapper for data lifecycle
- Integrates with Gorilla compression from timeseries module

### With Observability Module
- All operations emit distributed traces (OpenTelemetry spans)
- Task execution metrics tracked in Statistics structures
- Audit logging for security-sensitive operations

### With Server Module
- Task scheduler API handlers expose REST endpoints
- WebSocket subscriptions for real-time task status
- Authentication context passed to scheduler operations

## Usage Examples

### Cron-Based Scheduling
```cpp
ScheduledTask nightly_task;
nightly_task.name = "Nightly Cleanup";
nightly_task.type = ScheduledTask::TaskType::AQL_QUERY;
nightly_task.aql_query = "FOR d IN logs FILTER d.timestamp < DATE_SUB(NOW(), 30, 'day') REMOVE d IN logs";
nightly_task.trigger_type = ScheduledTask::TriggerType::CRON;
nightly_task.cron_expression = "0 2 * * *";  // 02:00 every day

scheduler.registerTask(nightly_task);
```

### DAG Execution
```cpp
ScheduledTask extract, transform, load;
extract.id   = "etl_extract";
transform.id = "etl_transform";
load.id      = "etl_load";

transform.dependencies = {"etl_extract"};
load.dependencies      = {"etl_transform"};

scheduler.registerTask(extract);
scheduler.registerTask(transform);
scheduler.registerTask(load);

auto result = scheduler.executeDAG({"etl_extract", "etl_transform", "etl_load"});
// result.succeeded, result.failed, result.skipped, result.condition_skipped
```

### SLA Monitoring
```cpp
// Fire a TaskSlaBreached alert if the task takes longer than 60 s
task.sla_deadline = std::chrono::seconds(60);
scheduler.setAlertmanager(alertmanager);
```

### Dynamic Concurrency Scaling
```cpp
TaskScheduler::Config cfg;
cfg.enable_dynamic_scaling    = true;
cfg.max_concurrent_tasks      = 4;   // initial limit
cfg.min_concurrent_tasks      = 2;   // floor
cfg.max_concurrent_tasks_ceil = 16;  // ceiling
cfg.scale_up_queue_depth      = 3;   // grow when ≥3 tasks pending
cfg.scale_down_idle_ticks     = 5;   // shrink after 5 idle ticks

TaskScheduler scheduler(query_engine, cfg);
// Query current state
size_t depth = scheduler.getQueueDepth();
size_t limit = scheduler.getDynamicConcurrencyLimit();
```

### Execution History
```cpp
// Retrieve last 50 audit events for a specific task
auto history = scheduler.getExecutionHistory("my_task_id", 50);
for (const auto& evt : history) {
    std::cout << evt.task_id << " " << evt.success << "\n";
}
```

### Schedule Data Compression
```cpp
ScheduledTask compression_task;
compression_task.name = "Compress Old Data";
compression_task.type = ScheduledTask::TaskType::AQL_QUERY;
compression_task.aql_query =
    "FOR d IN timeseries "
    "FILTER d.timestamp < DATE_SUB(NOW(), 1, 'day') "
    "UPDATE d WITH { compressed: true } IN timeseries";
compression_task.interval = std::chrono::hours(6);

scheduler.registerTask(compression_task);
```

### Custom Function Registration
```cpp
// Register backup function
scheduler.registerFunction("backup_incremental",
    [&backup_mgr](const nlohmann::json& params) -> nlohmann::json {
        std::string dest = params["destination"].get<std::string>();
        backup_mgr.createIncrementalBackup(dest);
        return {{"status", "success"}, {"destination", dest}};
    }
);

// Schedule daily backups
ScheduledTask backup_task;
backup_task.name = "Daily Incremental Backup";
backup_task.type = ScheduledTask::TaskType::FUNCTION;
backup_task.function_name = "backup_incremental";
backup_task.parameters = {{"destination", "/backups/incremental"}};
backup_task.interval = std::chrono::hours(24);

scheduler.registerTask(backup_task);
```

### Hybrid Retention Setup
```cpp
HybridRetentionConfig config;
config.stage2.low_cv_threshold = 3.0;      // More aggressive downsampling
config.stage2.detect_anomalies = true;      // Preserve anomalies
config.auto_cleanup = true;                 // Auto-delete old data

HybridRetentionManager retention(
    query_engine, tsstore, scheduler, config
);
retention.start();

// Monitor savings
auto stats = retention.getStats();
std::cout << "Storage saved: "
          << stats.total_storage_bytes_saved / (1024*1024*1024)
          << " GB ("
          << stats.overall_storage_reduction_percent
          << "%)" << std::endl;
```

## Performance Characteristics

### Task Scheduler
- **Overhead per task**: ~1KB memory
- **Scheduler tick**: 10s by default (configurable)
- **Concurrent tasks**: 4 by default (configurable)
- **Task startup latency**: <100ms
- **Persistence overhead**: ~1ms per task (if enabled)

### Hybrid Retention Manager
- **Stage 1 execution**: 5-30 minutes (depends on data volume)
- **Stage 2 execution**: 10-60 minutes (variance calculation + aggregation)
- **Stage 3 execution**: 5-20 minutes (daily aggregates)
- **Storage reduction**: 99.9% typical (timeseries with low CV)
- **CPU overhead**: <5% on average (spikes during execution)

## Configuration Options and Limits

Important `TaskScheduler::Config` controls:

- `max_concurrent_tasks`: static concurrency cap when dynamic scaling is disabled.
- `enable_dynamic_scaling`, `min_concurrent_tasks`, `max_concurrent_tasks_ceil`, `scale_up_queue_depth`, `scale_down_idle_ticks`: queue-depth based adaptive concurrency.
- `persist_tasks`, `persistence_path`: task durability and restart recovery.
- `enable_audit_logging`, `enable_anomaly_detection`, `enable_gdpr_mode`: observability/privacy controls for execution logs.
- `enable_result_store`, `result_store_max_results_per_task`: output persistence and retention bounds.
- `sandbox_execution`: wraps custom task functions in `ModuleSandbox` for stronger runtime isolation.

Operational boundaries:

- Legacy retries are bounded by `ScheduledTask::max_retries`; advanced behaviour uses `ScheduledTask::retry_policy`.
- SLA monitoring is opt-in via `ScheduledTask::sla_deadline`.
- `executeDAG(...)` validates task IDs and cycle-freedom before execution.

## Error Handling

**Task Execution Failures:**
- Caught exceptions logged with full stack trace
- Task statistics updated (failed_executions counter)
- Optional retry with exponential backoff (max_retries)
- on_failure callback invoked for notification

**Scheduler Failures:**
- Graceful degradation: continues with other tasks
- Persistent task state survives scheduler restart
- Configurable timeout prevents infinite execution
- Rate limiting prevents abuse of executeTaskNow()

**Validation Failures:**
- AQL injection detected: task registration rejected
- Invalid resource limits: exception thrown before registration
- Missing QueryEngine: constructor throws std::invalid_argument
- Persistence errors: logged but not fatal

## Monitoring and Debugging

**Task Statistics:**
```cpp
auto stats = scheduler.getStats();
std::cout << "Registered: " << stats.registered_tasks << std::endl;
std::cout << "Active: " << stats.active_tasks << std::endl;
std::cout << "Running: " << stats.running_tasks << std::endl;
std::cout << "Total executions: " << stats.total_executions << std::endl;
std::cout << "Failed: " << stats.failed_executions << std::endl;
```

**Per-Task Statistics:**
```cpp
auto task = scheduler.getTask("my_task_id");
if (task) {
    std::cout << "Executions: " << task->total_executions << std::endl;
    std::cout << "Success rate: "
              << (100.0 * task->successful_executions / task->total_executions)
              << "%" << std::endl;
    std::cout << "Avg duration: " << task->avg_execution_time_ms << "ms" << std::endl;
    std::cout << "Last error: " << task->last_error << std::endl;
}
```

**Distributed Tracing:**
- All task executions create OpenTelemetry spans
- Spans include task_id, task_name, task_type attributes
- Execution time recorded as span duration
- Errors recorded via span.recordError()

## Testing Support

**Mock Implementations:**
- TaskScheduler can use mock QueryEngine for unit tests
- HybridRetentionManager can use mock TSStore
- Custom functions can be injected for testing
- Time-based logic testable via manual executeTaskNow()

**Integration Testing:**
- Start scheduler with test configuration
- Register test tasks with short intervals
- Verify task execution via statistics
- Test graceful shutdown and restart

## Related Documentation

- [Storage Module Headers](../storage/README.md) - TSStore and persistence
- [Query Module Headers](../query/README.md) - AQL execution
- [Timeseries Module Headers](../timeseries/README.md) - Gorilla compression
- [Server Module Headers](../server/README.md) - API handlers
- [Security Module Headers](../security/README.md) - AQL injection detection
- [Scheduler Implementation Docs](../../src/scheduler/README.md) - Runtime architecture and component mapping
- [Scheduler Architecture](../../src/scheduler/ARCHITECTURE.md) - Internal design details
- [Scheduler Roadmap](../../src/scheduler/ROADMAP.md) - Phase plan and delivery status
- [Scheduler Future Enhancements](../../src/scheduler/FUTURE_ENHANCEMENTS.md) - Planned work
- [Scheduler Docs (DE)](../../docs/de/scheduler/README.md) - Secondary German overview

## Version History

- **v1.0.0**: Initial TaskScheduler implementation
  - Basic cron-like scheduling
  - AQL query and function execution
  - Task persistence

- **v1.3.0**: Added HybridRetentionManager
  - Three-stage retention strategy
  - Gorilla compression integration
  - Variance-based downsampling

- **v1.4.0**: Security enhancements
  - AST-based AQL injection detection
  - Query complexity limits
  - Rate limiting for manual execution
  - Enhanced audit logging

- **v1.5.0**: Full cron support, distributed coordination, and DAG execution
  - Complete cron expression parsing (wildcards, ranges, lists, steps, name aliases, @-specials, 6-field year)
  - Distributed task coordination across nodes with leader election
  - Task dependency DAG execution with conditional branching (`branch_condition`)
  - Workflow engine (multi-step DAG)
  - CDC event-driven task triggers
  - Task retry policies (FIXED_DELAY, EXPONENTIAL_BACKOFF, LINEAR_BACKOFF, JITTER_BACKOFF, FIBONACCI_BACKOFF)
  - Scheduled task output persistence (`TaskResultStore`)
  - Task execution history with searchable audit log (`TaskAuditManager`)
  - SLA monitoring — alert on task failure or SLA breach (`setAlertmanager`, `sla_deadline`)
  - Dynamic concurrency scaling based on queue depth (`enable_dynamic_scaling`, `getQueueDepth`, `getDynamicConcurrencyLimit`)
  - Integration with external schedulers (Kubernetes CronJob, Apache Airflow)
  - Prometheus metrics export (`exportMetrics()`)

## Future Enhancements

See [../../src/scheduler/FUTURE_ENHANCEMENTS.md](../../src/scheduler/FUTURE_ENHANCEMENTS.md) for planned features including:
- Task versioning and rollback
- Dynamic resource allocation (cgroups / per-task CPU + memory limits)
- Task checkpointing and resume for long-running tasks
- Multi-tenancy support with per-tenant resource quotas
- Task templates and parameterization
- Task result streaming for long-running AQL tasks

## Additional Header Files

The following headers are present in `include/scheduler/` and supplement the components documented above.

### distributed_task_coordinator.h
**Location:** `distributed_task_coordinator.h`

Coordinates task execution across a cluster: leader election for scheduler role, task ownership assignment, and cross-node heartbeat. Used internally by `TaskScheduler` to enforce a single active scheduler leader per cluster.

### event_trigger.h
**Location:** `event_trigger.h`

Defines `EventTrigger` and `EventTriggerManager` for firing tasks from CDC events with callback success/failure counters and circuit-breaker safeguards.

### external_scheduler_adapter.h
**Location:** `external_scheduler_adapter.h`

Adapter interface for integrating ThemisDB's scheduler with external systems (Kubernetes CronJob and Apache Airflow), including manifest conversion and import.

### task_anomaly_detector.h
**Location:** `task_anomaly_detector.h`

Detects anomalous task execution patterns (duration spikes, failure surges, and resource anomalies) and emits anomaly scores for monitoring pipelines.

### task_audit_event.h
**Location:** `task_audit_event.h`

`TaskAuditEvent` structure: immutable record of a single task lifecycle transition (registered, started, succeeded, failed, cancelled). Used by `TaskAuditManager`.

### task_audit_manager.h
**Location:** `task_audit_manager.h`

`TaskAuditManager`: persists and queries `TaskAuditEvent` records; supports GDPR redaction and retention policies.

### task_result_store.h
**Location:** `task_result_store.h`

`TaskResultStore` / `TaskExecutionResult`: persists per-task execution outputs (JSON result, duration, exit status) for later retrieval via `getTaskResults()`.

## Installation
