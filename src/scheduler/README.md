# ThemisDB Scheduler Module

**Last Updated:** March 2026
**Version:** 1.5.0

## Module Purpose

The Scheduler module provides ThemisDB's task scheduling and automation implementation. It enables cron-like periodic execution of AQL queries and custom functions for data processing, maintenance, backup, retention, and analytics workflows. The module includes a generic task scheduler, a specialized hybrid retention manager for time-series data lifecycle management, distributed task coordination, DAG-based workflow execution, event-triggered tasks, external scheduler integration, audit logging with searchable history, and alerting on task failure or SLA breach.

## Relevant Interfaces

| Interface / File | Header | Role |
|-----------------|--------|------|
| `task_scheduler.cpp` | `include/scheduler/task_scheduler.h` | Task scheduling engine with thread pool, retry policies, DAG execution |
| `hybrid_retention_manager.cpp` | `include/scheduler/hybrid_retention_manager.h` | Three-stage time-series data lifecycle |
| `distributed_task_coordinator.cpp` | `include/scheduler/distributed_task_coordinator.h` | Cluster-wide leader election and task coordination |
| `event_trigger.cpp` | `include/scheduler/event_trigger.h` | CDC changefeed → task execution triggers |
| `external_scheduler_adapter.cpp` | `include/scheduler/external_scheduler_adapter.h` | Kubernetes CronJob and Apache Airflow integration |
| `task_audit_manager.cpp` | `include/scheduler/task_audit_manager.h` | Searchable audit log for task execution history |
| `task_audit_event.cpp` | `include/scheduler/task_audit_event.h` | Structured audit event types |
| `task_anomaly_detector.cpp` | `include/scheduler/task_anomaly_detector.h` | Statistical anomaly detection for task execution patterns |
| `task_result_store.cpp` | `include/scheduler/task_result_store.h` | Persistent storage of scheduled task output results |
| `../utils/cron_parser.cpp` | `../include/utils/cron_parser.h` | Full cron expression parsing (v1.5.0) |

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Full cron expression parsing (v1.5.0) complete; thread pool task scheduler, hybrid retention manager, distributed coordination, DAG execution, event triggers, audit logging, alerting, and external scheduler integration all production-ready.

## Scope

**In Scope:**
- Task scheduler implementation with thread pool
- AQL query execution via QueryEngine integration
- Custom function registration and execution
- Task persistence and recovery from disk
- Hybrid retention manager (3-stage data lifecycle)
- Task statistics, monitoring, and audit logging
- Security validation (AQL injection detection, resource limits)
- Rate limiting and resource management
- OpenTelemetry tracing integration
- **Full cron expression parsing** (wildcards, ranges, lists with embedded ranges/steps, start/step syntax, month/weekday name aliases, @-specials, 6-field year constraint, timezone-aware scheduling)
- Distributed task coordination with leader election
- Task dependency DAG execution with conditional branching
- Event-triggered tasks (CDC changefeed integration)
- Task retry policies (max attempts, exponential/Fibonacci/jitter back-off)
- Scheduled task output persistence (store results in ThemisDB via `TaskResultStore`)
- Task execution history with searchable audit log (`TaskAuditManager`)
- Alert on task failure or SLA breach (Alertmanager integration)
- External scheduler integration (Kubernetes CronJob, Apache Airflow)

**Out of Scope:**
- Authentication/authorization logic (handled by auth module)
- Query parsing (handled by query module)
- Storage operations (handled by storage module)
- Web UI frontend for task management (REST API exists; frontend planned)

## Key Components

### TaskScheduler
**Location:** `task_scheduler.cpp`, `../include/scheduler/task_scheduler.h`

Core scheduler implementation providing periodic task execution with comprehensive security controls and distributed tracing integration. Supports multiple retry strategies (FIXED_DELAY, EXPONENTIAL_BACKOFF, LINEAR_BACKOFF, JITTER_BACKOFF, FIBONACCI_BACKOFF), DAG-based task execution with conditional branching, SLA deadline monitoring, and Alertmanager integration for failure/SLA-breach notifications.

**Thread Safety:** All operations are thread-safe with internal locking.

**Performance:** <1% CPU overhead, 50-200ms task startup latency.

See full documentation in README for implementation details.

### HybridRetentionManager
**Location:** `hybrid_retention_manager.cpp`, `../include/scheduler/hybrid_retention_manager.h`

Three-stage data lifecycle management achieving 99.9% storage reduction for time-series data.

**Stages:**
1. Gorilla compression (0-7 days): 10-20x reduction
2. Adaptive retention (7-365 days): Variance-based downsampling
3. Time-based retention (>1 year): Daily aggregates

See full documentation in README for configuration and usage.

### DistributedTaskCoordinator
**Location:** `distributed_task_coordinator.cpp`, `../include/scheduler/distributed_task_coordinator.h`

Cluster-wide leader election and task coordination. Uses the `DistributedCoordinator` (sharding module) for leader election; activates or deactivates the local `TaskScheduler` depending on whether the node is the elected leader.

### EventTrigger / EventTriggerManager
**Location:** `event_trigger.cpp`, `../include/scheduler/event_trigger.h`

CDC changefeed-to-task execution bridge. Listens to a `Changefeed` stream and fires registered `ScheduledTask` callbacks when filter predicates match, with per-trigger circuit breaker and throttling controls.

### ExternalSchedulerAdapter
**Location:** `external_scheduler_adapter.cpp`, `../include/scheduler/external_scheduler_adapter.h`

Generates Kubernetes CronJob manifests and Apache Airflow DAG Python files from `ScheduledTask` definitions, enabling round-trip integration with external orchestration systems.

### TaskAuditManager
**Location:** `task_audit_manager.cpp`, `../include/scheduler/task_audit_manager.h`

Searchable audit log for all task execution events. Provides `queryAuditEvents(AuditQueryParams)` and `querySecurityEvents()` with filtering by task ID, time range, status, and user. Exports to JSONL for SIEM integration. Backed by `utils::AuditLogger` for tamper-evident logging.

### TaskResultStore
**Location:** `task_result_store.cpp`, `../include/scheduler/task_result_store.h`

Persists scheduled task execution output (`TaskExecutionResult`) to RocksDB. Results are retrievable by task ID and time range.

### TaskAnomalyDetector
**Location:** `task_anomaly_detector.cpp`, `../include/scheduler/task_anomaly_detector.h`

Statistical anomaly detection for task execution patterns. Raises alerts when task durations, failure rates, or queue depths deviate significantly from historical baselines.

### CronExpression Parser
**Location:** `../utils/cron_parser.cpp`, `../include/utils/cron_parser.h`

Full standard cron expression parser supporting all standard syntax elements:
- Wildcards `*`, ranges `1-5`, lists `1,3,5`, steps `*/15`, start/step `5/15`
- List items may contain ranges or steps: `1,3-5,*/10`
- Month name aliases: `JAN`–`DEC` (also full names, case-insensitive)
- Weekday name aliases: `SUN`–`SAT` (also full names, case-insensitive)
- Special expressions: `@daily`, `@hourly`, `@monthly`, `@weekly`, `@yearly`, `@reboot`
- Optional 6-field form with year constraint: `0 9 * * MON 2025`
- Timezone-aware `getNextExecution(from, tz_offset_seconds)` overload

## Wissenschaftliche Grundlagen

The following peer-reviewed papers and specifications form the scientific foundation of the Scheduler module's core algorithms and design decisions.

### [1] Gorilla: A Fast, Scalable, In-Memory Time Series Database
**Used in:** `HybridRetentionManager` – Stage 1 Gorilla compression (0–7 days, 10–20× reduction)

> Pelkonen, T., Franklin, S., Teller, J., Cavallaro, P., Huang, Q., Meza, J., & Veeraraghavan, K. (2015).
> **Gorilla: A Fast, Scalable, In-Memory Time Series Database.**
> *Proceedings of the VLDB Endowment*, 8(12), 1816–1827.
> DOI: [10.14778/2824032.2824078](https://doi.org/10.14778/2824032.2824078)
> URL: https://www.vldb.org/pvldb/vol8/p1816-teller.pdf

Introduces the Gorilla time-series compression algorithm using XOR delta-of-delta encoding for floating-point values and variable-length encoding for timestamps. Achieves 1.37 bytes per data point on average. The `GorillaEncoder` in `src/timeseries/gorilla.*` directly implements this algorithm.

---

### [2] Scheduling Multithreaded Computations by Work Stealing
**Used in:** `TaskScheduler` – thread pool with work-stealing dequeue for task dispatching

> Blumofe, R. D., & Leiserson, C. E. (1999).
> **Scheduling Multithreaded Computations by Work Stealing.**
> *Journal of the ACM (JACM)*, 46(5), 720–748.
> DOI: [10.1145/324133.324234](https://doi.org/10.1145/324133.324234)

Provides the theoretical foundation for work-stealing thread pool schedulers. Proves that a work-stealing scheduler achieves optimal time and space bounds for fully strict multithreaded computations. The TaskScheduler's thread pool design follows the work-stealing pattern to maximise CPU utilisation across concurrent task execution.

---

### [3] Dapper, a Large-Scale Distributed Systems Tracing Infrastructure
**Used in:** `TaskScheduler` – OpenTelemetry distributed tracing integration (`utils/tracing.h`)

> Sigelman, B. H., Barroso, L. A., Burrows, M., Stephenson, P., Plakal, M., Beaver, D., Jaspan, S., & Shanbhag, C. (2010).
> **Dapper, a Large-Scale Distributed Systems Tracing Infrastructure.**
> *Google Technical Report*.
> URL: https://research.google/pubs/pub36356/

The Dapper paper introduced the span/trace model that is the conceptual basis for all modern distributed tracing systems including OpenTelemetry. ThemisDB's `Tracer::startSpan` API directly follows the Dapper model of parent/child spans, baggage propagation, and sampling-based trace collection used throughout the scheduler and retention manager.

---

### [4] POSIX crontab Utility Specification (IEEE Std 1003.1-2017)
**Used in:** `CronExpression::parse()` – 5-field cron syntax definition and field semantics

> IEEE and The Open Group. (2018).
> **IEEE Std 1003.1-2017: Standard for Information Technology — Portable Operating System Interface (POSIX), Base Specifications, Issue 7 — crontab Utility.**
> The Open Group.
> URL: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html

Defines the canonical cron expression grammar (`minute hour day month weekday`) and the semantics of wildcards, ranges, lists, and steps. The `CronExpression` parser in `src/utils/cron_parser.cpp` implements this specification exactly, extending it with name aliases (JAN–DEC, MON–SUN), a start/step shorthand, an optional year field, and `@`-special shorthand expressions.

---

### [5] Downsampling Time Series for Visual Representation (LTTB)
**Used in:** `HybridRetentionManager` – Stage 2 variance-based adaptive retention (7–365 days)

> Steinarsson, S. (2013).
> **Downsampling Time Series for Visual Representation.**
> *M.Sc. thesis, School of Computer Science, Reykjavik University, Iceland*.
> URL: http://skemman.is/stream/get/1946/15343/37285/3/SS_MSthesis.pdf

Introduces the *Largest Triangle Three Buckets* (LTTB) algorithm for perceptually optimal time-series downsampling. The algorithm selects data points that maximise the area of the triangle formed by adjacent buckets, preserving visual features (peaks, troughs) and statistical variance. Stage 2 of the `HybridRetentionManager` applies variance-based point selection inspired by this approach to determine which data points to retain across the 7–365-day window.

---

## Related Documentation

- [Scheduler Headers](../include/scheduler/README.md) - Public API
- [Storage Module](../storage/README.md) - Data persistence
- [Query Module](../query/README.md) - AQL execution

## Future Enhancements

See [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) for roadmap.

## Scientific References

1. Liu, C. L., & Layland, J. W. (1973). **Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment**. *Journal of the ACM*, 20(1), 46–61. https://doi.org/10.1145/321738.321743

2. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). **Operating System Concepts (10th ed.)**. Wiley. ISBN: 978-1-119-32091-3

3. Corbett, J. C., Dean, J., Epstein, M., Fikes, A., Frost, C., Furman, J., … Woodford, D. (2013). **Spanner: Google's Globally Distributed Database**. *ACM Transactions on Computer Systems*, 31(3), 8:1–8:22. https://doi.org/10.1145/2491245

4. Quartz Scheduler Development Team. (2023). **Quartz Scheduler: Enterprise Job Scheduling**. Terracotta. http://www.quartz-scheduler.org/
