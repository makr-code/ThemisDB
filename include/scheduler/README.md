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
- Distributed coordination implementation (future enhancement)
- Query execution logic (handled by query module)
- Storage operations (handled by storage module)
- Authentication and authorization (handled by auth module)
- Network protocols (handled by server module)

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
    };
    
    TaskScheduler(QueryEngine* query_engine, const Config& config);
    
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
    
    // Manual execution
    nlohmann::json executeTaskNow(const std::string& task_id);
    
    // Function registration
    void registerFunction(const std::string& name, TaskFunction func);
    void unregisterFunction(const std::string& name);
    
    // Monitoring
    Stats getStats() const;
    std::vector<ScheduledTask> listTasks() const;
    std::shared_ptr<ScheduledTask> getTask(const std::string& task_id) const;
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
    } type;
    
    std::string aql_query;              // AQL query (if type == AQL_QUERY)
    std::string function_name;           // Function name (if type == FUNCTION)
    nlohmann::json parameters;           // Task parameters
    
    // Scheduling
    std::chrono::milliseconds interval;  // Fixed interval
    std::chrono::system_clock::time_point next_run;
    bool enabled = true;
    bool running = false;
    
    // Statistics
    size_t total_executions = 0;
    size_t successful_executions = 0;
    size_t failed_executions = 0;
    double avg_execution_time_ms = 0.0;
    std::string last_error;
    
    // Resource limits
    std::chrono::milliseconds timeout;
    size_t max_retries = 3;
    
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

### Basic Task Registration
```cpp
TaskScheduler scheduler(query_engine, config);
scheduler.start();

// Schedule data compression
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

- **v1.5.0**: Performance improvements
  - Optimized scheduler loop
  - Reduced lock contention
  - Better task priority handling

## Future Enhancements

See [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) for planned features including:
- Distributed task coordination with Raft
- Cron expression parser (vs simple intervals)
- Task dependencies and DAG execution
- Priority-based scheduling
- Dynamic resource allocation
- Advanced retry policies
- Task versioning and rollback
