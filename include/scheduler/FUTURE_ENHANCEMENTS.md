<!-- Status: current | validated: 2026-04-06 -->
# Scheduler Module - Future Enhancements

## Scope

- API-level enhancements to `include/scheduler/` headers
- Task DAG interface (`TaskGraph`, `graph.addTask`, `scheduler.registerTaskGraph`)
- Priority queue API (`TaskPriority` enum, priority-slot configuration)
- Distributed task coordination interface (`DistributedTaskScheduler`, `RaftCluster`)
- Resource quota API (`TenantQuota`, `TaskResourceLimits`)
- Cron expression parser API (`CronParser`, `CronSchedule`)

## Design Constraints

- [ ] Task DAG is immutable after submission to the scheduler
- [ ] Priority API is lock-free; no mutexes on the hot enqueue path
- [ ] Resource quota enforced before task start (pre-admission check)
- [ ] No cross-tenant task visibility — tasks namespaced by `tenant_id`
- [ ] All public header types are copy-constructible and move-constructible
- [ ] Distributed coordination headers require `THEMIS_DISTRIBUTED` compile flag

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `TaskGraph` | Workflow orchestration, ETL pipelines | Immutable after `registerTaskGraph()` |
| `TaskPriority` / priority-slot config | Query engine, maintenance tasks | Lock-free priority queue |
| `DistributedTaskScheduler` | Cluster deployments | Requires `RaftCluster` dependency |
| `TenantQuota` | Multi-tenant SaaS deployments | Pre-admission quota enforcement |
| `TaskResourceLimits` | Resource-aware schedulers | Applied via cgroups on Linux |
| `CronParser` | User-defined scheduled tasks | Standard 6-field cron syntax |

## Planned Features

### Distributed Task Coordination with Raft
**Priority:** High
**Target Version:** v1.7.0

Enable task scheduling across a distributed cluster with leader election and failover.

**Features:**
- Leader-based task coordination (only leader schedules tasks)
- Automatic failover when leader fails
- Task state replicated via Raft log
- Distributed task execution across cluster nodes
- Load balancing of task execution
- Consistent task scheduling across replicas

**Implementation:**
```cpp
// Future API
RaftCluster cluster({"node1:7000", "node2:7000", "node3:7000"});
DistributedTaskScheduler scheduler(query_engine, cluster);

// Tasks registered on leader, replicated to followers
scheduler.registerTask(task);

// If leader fails, follower elected and continues scheduling
// No duplicate task execution during failover
```

**Benefits:**
- High availability for critical scheduled tasks
- Eliminates single point of failure
- Automatic recovery from scheduler crashes
- Consistent task execution across cluster

**Challenges:**
- Clock synchronization across nodes (NTP required)
- Network partition handling
- Task result aggregation
- State transfer for new replicas

---

### Cron Expression Parser
**Priority:** Medium
**Target Version:** v1.6.0

Replace simple interval-based scheduling with full cron expression support.

**Cron Syntax:**
```
┌─────── second (0-59)
│ ┌───── minute (0-59)
│ │ ┌─── hour (0-23)
│ │ │ ┌─ day of month (1-31)
│ │ │ │ ┌ month (1-12)
│ │ │ │ │ ┌ day of week (0-6, Sunday=0)
│ │ │ │ │ │
* * * * * *
```

**Examples:**
```cpp
// Every 5 minutes
task.cron_expression = "0 */5 * * * *";

// Every day at 2:30 AM
task.cron_expression = "0 30 2 * * *";

// Every Monday at 9:00 AM
task.cron_expression = "0 0 9 * * 1";

// First day of every month at midnight
task.cron_expression = "0 0 0 1 * *";

// Weekdays at 6 PM
task.cron_expression = "0 0 18 * * 1-5";
```

**Implementation:**
```cpp
class CronParser {
public:
    // Parse cron expression
    static CronSchedule parse(const std::string& expr);

    // Calculate next run time
    static std::chrono::system_clock::time_point
        nextRun(const CronSchedule& schedule,
                const std::chrono::system_clock::time_point& from);
};

struct ScheduledTask {
    // ... existing fields ...
    std::optional<std::string> cron_expression;  // Alternative to interval
};
```

**Benefits:**
- More expressive scheduling (e.g., "first Monday of month")
- Industry-standard syntax familiar to users
- Timezone-aware scheduling
- Better support for calendar-based tasks

---

### Task Dependencies and DAG Execution
**Priority:** High
**Target Version:** v1.7.0

Support directed acyclic graphs (DAGs) of dependent tasks.

**Features:**
- Tasks can depend on other tasks
- Topological sorting for execution order
- Parallel execution of independent tasks
- Cascading failures (optional)
- Conditional execution based on predecessor results

**API:**
```cpp
// Define tasks
auto extract = ScheduledTask{/* ... */};
auto transform = ScheduledTask{/* ... */};
auto load = ScheduledTask{/* ... */};

// Set up dependencies
TaskGraph graph;
graph.addTask(extract);
graph.addTask(transform, {extract.id});  // Depends on extract
graph.addTask(load, {transform.id});     // Depends on transform

// Execute DAG
scheduler.registerTaskGraph(graph);
scheduler.executeGraph(graph.id);  // Respects dependencies
```

**Execution Model:**
```
extract
  ↓
transform_1 ─┐
             ├→ load
transform_2 ─┘
```

**Benefits:**
- ETL pipeline orchestration
- Complex data processing workflows
- Automatic parallelization
- Clear dependency visualization
- Retry logic for failed dependencies

**Use Cases:**
- Data ingestion → processing → aggregation → reporting
- Backup → verification → cleanup
- Index rebuild → replication → validation

---

### Priority-Based Scheduling
**Priority:** Medium
**Target Version:** v1.7.0

Assign priorities to tasks for resource contention resolution.

> **Partial implementation (v1.7.0):** `schedulerLoop()` now sorts `tasks_to_execute` by
> `ScheduledTask::priority` (HIGH → NORMAL → LOW) before dispatch. Full lock-free priority
> queue, starvation prevention via aging, and per-slot reservation are still planned below.

**Priority Levels:**
```cpp
enum class TaskPriority {
    CRITICAL = 0,   // System maintenance, backups
    HIGH = 1,       // Important analytics
    NORMAL = 2,     // Regular processing
    LOW = 3,        // Background cleanup
    BATCH = 4       // Bulk operations
};

struct ScheduledTask {
    // ... existing fields ...
    TaskPriority priority = TaskPriority::NORMAL;
};
```

**Scheduling Algorithm:**
- [x] Priority-ordered dispatch — `tasks_to_execute` sorted by `priority` DESC before thread launch
- [ ] Full lock-free priority queue to replace the pre-sort approach
- [ ] CRITICAL tasks preempt BATCH tasks (configurable)
- [ ] Starvation prevention via aging (boost priority after N idle ticks)
- [ ] Dynamic priority adjustment based on wait time

**Resource Allocation:**
```cpp
TaskScheduler::Config config;
config.max_concurrent_tasks = 8;
config.priority_slots = {
    {TaskPriority::CRITICAL, 2},  // Reserve 2 slots for critical
    {TaskPriority::HIGH, 3},      // Reserve 3 for high
    {TaskPriority::NORMAL, 2},    // 2 for normal
    {TaskPriority::LOW, 1}        // 1 for low/batch
};
```

**Benefits:**
- SLA compliance for critical tasks
- Better resource utilization
- Predictable performance for high-priority operations
- Graceful degradation under load

---

### Dynamic Resource Allocation
**Priority:** High
**Target Version:** v1.8.0

Adjust CPU, memory, and I/O limits per task dynamically.

**Resource Controls:**
```cpp
struct TaskResourceLimits {
    // CPU limits
    double cpu_quota = 1.0;              // 1.0 = 1 CPU core
    std::chrono::milliseconds cpu_period{100};

    // Memory limits
    size_t memory_limit_mb = 1024;       // Max 1GB RAM
    size_t memory_soft_limit_mb = 768;   // Warning threshold

    // I/O limits
    size_t disk_read_mbps = 100;         // Max 100 MB/s reads
    size_t disk_write_mbps = 50;         // Max 50 MB/s writes
    size_t iops_limit = 1000;            // Max 1K IOPS

    // Network limits (if remote)
    size_t network_bandwidth_mbps = 100;
};

struct ScheduledTask {
    // ... existing fields ...
    TaskResourceLimits resource_limits;
};
```

**Implementation via cgroups (Linux):**
```cpp
class ResourceController {
    void applyLimits(pid_t task_pid, const TaskResourceLimits& limits);
    void updateLimits(pid_t task_pid, const TaskResourceLimits& new_limits);
    ResourceUsage getCurrentUsage(pid_t task_pid);
};
```

**Dynamic Adjustment:**
- Monitor task resource usage
- Increase limits if task is throttled and resources available
- Decrease limits if task monopolizes resources
- Preempt tasks exceeding limits

**Benefits:**
- Prevent task resource starvation
- Protect against runaway tasks
- Better multi-tenancy support
- Fair resource sharing

---

### ~~Advanced Retry Policies~~ ✅ Implemented (v0.0.32)
**Priority:** Medium
**Target Version:** v1.6.0 → Delivered in v0.0.32

Sophisticated retry logic beyond simple max_retries counter.

**Retry Strategies:**
```cpp
enum class RetryStrategy {
    FIXED_DELAY,        // Fixed interval between retries
    EXPONENTIAL_BACKOFF,// 1s, 2s, 4s, 8s, ...
    LINEAR_BACKOFF,     // 1s, 2s, 3s, 4s, ...
    FIBONACCI_BACKOFF,  // 1s, 1s, 2s, 3s, 5s, 8s, ...
    JITTERED_BACKOFF    // Exponential + random jitter
};

struct RetryPolicy {
    RetryStrategy strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    size_t max_retries = 3;
    std::chrono::milliseconds initial_delay{1000};
    std::chrono::milliseconds max_delay{60000};
    double backoff_multiplier = 2.0;
    double jitter_factor = 0.1;  // ±10% random jitter

    // Conditional retry
    std::function<bool(const std::string& error)> should_retry;
};

struct ScheduledTask {
    // ... existing fields ...
    RetryPolicy retry_policy;
};
```

**Example:**
```cpp
// Retry with exponential backoff + jitter
task.retry_policy.strategy = RetryStrategy::JITTERED_BACKOFF;
task.retry_policy.max_retries = 5;
task.retry_policy.initial_delay = std::chrono::seconds(1);
task.retry_policy.max_delay = std::chrono::minutes(5);

// Only retry on transient errors
task.retry_policy.should_retry = [](const std::string& error) {
    return error.find("timeout") != std::string::npos ||
           error.find("connection") != std::string::npos;
};
```

**Benefits:**
- Handles transient failures gracefully
- Prevents thundering herd with jitter
- Configurable per task type
- Better error recovery

---

### Task Versioning and Rollback
**Priority:** Low
**Target Version:** v1.9.0

Version task definitions and support rollback to previous versions.

**Versioning Model:**
```cpp
struct TaskVersion {
    int version = 1;
    std::chrono::system_clock::time_point created_at;
    std::string created_by;  // User who created version
    std::string description;  // Change description
    ScheduledTask definition;
};

class TaskVersionManager {
public:
    // Create new version
    int createVersion(const std::string& task_id,
                      const ScheduledTask& new_def,
                      const std::string& description);

    // List versions
    std::vector<TaskVersion> listVersions(const std::string& task_id);

    // Rollback to previous version
    void rollback(const std::string& task_id, int version);

    // Compare versions
    TaskDiff diff(const std::string& task_id, int v1, int v2);
};
```

**Use Cases:**
- Track task definition changes over time
- Audit who changed what and when
- Quick rollback if new version causes issues
- A/B testing of task configurations

**Storage:**
- Store versions in separate collection/table
- Keep last N versions (configurable)
- Immutable version history

---

### Task Templates and Parameterization
**Priority:** Medium
**Target Version:** v1.8.0

Reusable task templates with parameter substitution.

**Template Definition:**
```cpp
TaskTemplate backup_template;
backup_template.name = "Incremental Backup Template";
backup_template.type = ScheduledTask::TaskType::FUNCTION;
backup_template.function_name = "backup_incremental";
backup_template.parameters_schema = {
    {"destination", {{"type", "string"}, {"required", true}}},
    {"compression", {{"type", "string"}, {"default", "zstd"}}},
    {"encryption", {{"type", "bool"}, {"default", true}}}
};

// Instantiate template
auto task = backup_template.instantiate({
    {"destination", "/backups/daily"},
    {"compression", "lz4"}
});
task.interval = std::chrono::hours(24);

scheduler.registerTask(task);
```

**Benefits:**
- Reduce duplication across similar tasks
- Enforce parameter schemas
- Easier task management
- Centralized template updates

---

### Observability Enhancements
**Priority:** High
**Target Version:** v1.7.0

Comprehensive monitoring and debugging capabilities.

**Metrics Export:**
```cpp
// Prometheus metrics
scheduler_tasks_total{status="success|failure"}
scheduler_task_duration_seconds{task_id="...", quantile="0.5|0.95|0.99"}
scheduler_tasks_running{priority="..."}
scheduler_tasks_queued
scheduler_resource_usage{resource="cpu|memory|disk"}
```

**Detailed Logging:**
- Structured logs with task context
- Debug mode with full AQL query logging
- Execution plans for complex tasks
- Performance traces

**Real-Time Monitoring:**
```cpp
// WebSocket subscription for task events
ws://server/api/v1/scheduler/events

Events:
- task.registered
- task.started
- task.completed
- task.failed
- task.retrying
- task.cancelled
```

**Visualization:**
- Grafana dashboard with task timeline
- Gantt chart for task execution
- Dependency graph visualization
- Resource utilization heatmaps

---

### Task Checkpointing and Resume
**Priority:** Medium
**Target Version:** v1.8.0

Support for long-running tasks with checkpointing.

**Checkpoint API:**
```cpp
// Task function with checkpointing
scheduler.registerFunction("process_large_dataset",
    [](const nlohmann::json& params, TaskCheckpointer& checkpointer) -> nlohmann::json {
        auto checkpoint = checkpointer.loadCheckpoint();
        size_t start_index = checkpoint.value("index", 0);

        for (size_t i = start_index; i < dataset.size(); i++) {
            processItem(dataset[i]);

            // Checkpoint every 1000 items
            if (i % 1000 == 0) {
                checkpointer.saveCheckpoint({{"index", i}});
            }
        }

        return {{"status", "success"}, {"processed", dataset.size()}};
    }
);
```

**Features:**
- Periodic checkpoint persistence
- Automatic resume on failure
- Incremental progress tracking
- External interrupt handling

**Benefits:**
- Fault tolerance for long-running tasks
- Graceful shutdown without data loss
- Progress visibility
- Reduced re-processing on failures

---

### Multi-Tenancy Support
**Priority:** High
**Target Version:** v1.7.0

Isolate tasks by tenant with resource quotas.

**Tenant Configuration:**
```cpp
struct TenantQuota {
    std::string tenant_id;
    size_t max_tasks = 100;
    size_t max_concurrent_tasks = 2;
    size_t max_cpu_quota = 2.0;      // 2 CPU cores
    size_t max_memory_mb = 4096;     // 4GB RAM
    size_t max_executions_per_hour = 1000;
};

TaskScheduler::Config config;
config.enable_multi_tenancy = true;
config.tenant_quotas = {
    {"tenant_a", {.max_tasks = 50, .max_concurrent_tasks = 1}},
    {"tenant_b", {.max_tasks = 200, .max_concurrent_tasks = 5}}
};
```

**Isolation:**
- Separate task namespaces per tenant
- Resource quota enforcement
- Task execution limits
- Billing/accounting per tenant

**Benefits:**
- SaaS deployment support
- Fair resource sharing
- Prevent noisy neighbor problem
- Tenant-specific policies

---

## Research and Exploration

### Machine Learning for Task Optimization
**Timeline:** 2025+

Apply ML to optimize task scheduling decisions.

**Potential Applications:**
- Predict task execution time based on historical data
- Recommend optimal scheduling intervals
- Auto-tune resource limits
- Detect anomalous task behavior
- Forecast resource requirements

**Models:**
- Time series forecasting (LSTM, Prophet) for execution time
- Classification for failure prediction
- Clustering for similar task detection
- Reinforcement learning for scheduling optimization

---

### Event-Driven Task Execution
**Timeline:** v1.9.0

Trigger tasks based on events rather than schedules.

**Event Sources:**
- Database mutations (CDC)
- External webhooks
- Message queue (Kafka, RabbitMQ)
- File system changes (inotify)
- Custom event streams

**Example:**
```cpp
EventTriggeredTask task;
task.event_source = "cdc://users";
task.event_filter = "operation == 'INSERT' AND status == 'active'";
task.function_name = "send_welcome_email";
task.debounce_ms = 1000;  // Batch events within 1s
```

**Benefits:**
- Real-time reactive processing
- Lower latency than polling
- More efficient resource usage
- Complex event processing (CEP)

---

### Serverless Task Execution
**Timeline:** 2025+

Execute tasks in serverless functions (AWS Lambda, Azure Functions).

**Architecture:**
- Task scheduler dispatches to serverless platform
- Automatic scaling based on load
- Pay-per-execution pricing
- Zero infrastructure management

**Use Cases:**
- Burst workloads with high variability
- Infrequent but resource-intensive tasks
- Multi-cloud deployments
- Cost optimization for idle tasks

---

## Community Requests

Track highly requested features from users:

1. **Calendar-aware scheduling** (holidays, business days)
2. **Task chaining** (run task B after task A completes)
3. **Manual approval gates** (pause for human approval)
4. **Task simulation** (dry-run mode)
5. **Task import/export** (backup task definitions)
6. **Audit trail** (detailed history of task changes)
7. **Task ownership** (assign tasks to users/teams)
8. **SLA tracking** (alerts when tasks miss deadlines)

## Contributing

Interested in implementing these features? See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

Feature requests and design discussions: https://github.com/ThemisDB/ThemisDB/discussions

## Test Strategy

- Unit tests for `CronParser::nextRun()` covering edge cases (DST, leap years, month boundaries)
- Property-based tests for DAG topological sort correctness with randomly generated graphs
- Concurrency tests for lock-free priority enqueue under high parallelism (≥ 10 threads)
- Integration tests for `DistributedTaskScheduler` failover with simulated leader crash
- Quota enforcement tests verifying pre-admission rejection when `TenantQuota` limits are exceeded
- Regression tests for starvation prevention (priority aging mechanism)

## Performance Targets

- Task enqueue ≤ 100 µs (p99) under 10,000 concurrent enqueue ops/s
- DAG dependency resolution ≤ 1 ms for a 100-node DAG
- Cron `nextRun()` computation ≤ 10 µs per call
- Priority queue insert/pop O(log N) with N ≤ 100,000 tasks
- Resource quota admission check ≤ 50 µs per task

## Security / Reliability

- Task isolation via per-tenant `TenantQuota` — no cross-tenant task visibility
- Resource limits enforced before task start to prevent runaway resource consumption
- DAG immutability after submission prevents race-condition task graph mutations
- Distributed coordinator uses Raft-based consensus; no split-brain task duplication
- Retry policies with jitter prevent thundering-herd effects on transient failures


---

## Scientific References

For the full IEEE-formatted scientific reference list backing the planned features above, see:
→ [`src/scheduler/FUTURE_ENHANCEMENTS.md` – Scientific References (IEEE Format)](../../src/scheduler/FUTURE_ENHANCEMENTS.md#scientific-references-ieee-format)
