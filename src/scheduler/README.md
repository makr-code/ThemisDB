# ThemisDB Scheduler Module

## Module Purpose

The Scheduler module provides ThemisDB's task scheduling and automation implementation. It enables cron-like periodic execution of AQL queries and custom functions for data processing, maintenance, backup, retention, and analytics workflows. The module includes a generic task scheduler and a specialized hybrid retention manager for time-series data lifecycle management.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `scheduler.cpp` | Task scheduling engine with thread pool |
| `job_queue.cpp` | Priority-based job queue management |
| `cron_parser.cpp` | Cron expression parsing (planned) |

## Current Delivery Status

**Maturity:** 🔴 Alpha — Task and job scheduling infrastructure operational; production scheduling with cron expressions and priorities in progress.

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

**Out of Scope:**
- Distributed coordination (future enhancement)
- Cron expression parsing (simple intervals only, v1.5.0)
- Task dependencies and DAG execution (future)
- Authentication/authorization logic (handled by auth module)
- Query parsing (handled by query module)
- Storage operations (handled by storage module)

## Key Components

### TaskScheduler
**Location:** `task_scheduler.cpp`, `../include/scheduler/task_scheduler.h`

Core scheduler implementation providing periodic task execution with comprehensive security controls and distributed tracing integration.

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
