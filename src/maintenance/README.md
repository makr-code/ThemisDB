> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md -->

# ThemisDB Maintenance Module

## Module Purpose

The Maintenance module provides a centralized orchestration layer for all database
maintenance operations. It allows operators to define named maintenance schedules
with cron-based execution, maintenance window enforcement, task sequencing with
halt-on-failure semantics, and aggregated per-module health reporting.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `include/maintenance/database_maintenance_orchestrator.h` | Primary public API |
| `include/maintenance/maintenance_task.h` | Task types, job struct, job state enum |
| `include/maintenance/maintenance_schedule.h` | Schedule entry with JSON serialization and DAG dependency model |
| `include/maintenance/maintenance_schedule_store.h` | RocksDB-backed schedule persistence |
| `include/maintenance/maintenance_health_report.h` | Health report aggregation |
| `include/maintenance/i_maintenance_task_handler.h` | Task handler interface |
| `include/maintenance/maintenance_task_handler_impls.h` | Built-in handlers: storage compaction, replica validation, MVCC cleanup, function |
| `include/maintenance/i_distributed_lock.h` | Distributed lock interface + `InProcessDistributedLock` |
| `src/maintenance/database_maintenance_orchestrator.cpp` | Implementation |
| `src/maintenance/maintenance_registry.cpp` | Default schedule bundles |
| `src/maintenance/maintenance_schedule_store.cpp` | `MaintenanceScheduleStore` implementation |

## Key Classes

### `DatabaseMaintenanceOrchestrator`

Central coordinator for all maintenance scheduling and execution.

```cpp
#include "maintenance/database_maintenance_orchestrator.h"

// Construction (via dependency injection)
auto orchestrator = DatabaseMaintenanceOrchestrator(
    scheduler,           // TaskScheduler*
    index_maintenance,   // std::shared_ptr<IndexMaintenanceManager>
    audit_logger,        // std::shared_ptr<utils::AuditLogger>
    storage              // IStorageEngine* (nullptr = in-memory-only)
);

orchestrator.start();

// Create a schedule
MaintenanceScheduleEntry schedule;
schedule.id = "nightly-index-rebuild";
schedule.name = "Nightly Index Rebuild";
schedule.cron_expression = "0 2 * * *";   // 2:00 AM daily
schedule.window_start_hour = 1;
schedule.window_end_hour = 5;
schedule.tasks = { MaintenanceTaskType::INDEX_REBUILD, MaintenanceTaskType::STATISTICS_UPDATE };
schedule.halt_on_task_failure = true;
schedule.enabled = true;

auto result = orchestrator.createSchedule(schedule);

// List recent jobs
auto jobs = orchestrator.listJobs(50);

// Get aggregated health report
MaintenanceHealthReport health = orchestrator.getHealthReport();
```

### Default Schedule Bundles

`src/maintenance/maintenance_registry.cpp` provides standalone free functions in
`namespace themis::maintenance` that return pre-built `MaintenanceScheduleEntry` values:

```cpp
#include "maintenance/database_maintenance_orchestrator.h"
#include "maintenance/maintenance_schedule.h"

// Get default schedule entries
auto daily   = themis::maintenance::defaultDailySchedule();
auto weekly  = themis::maintenance::defaultWeeklySchedule();
auto monthly = themis::maintenance::defaultMonthlySchedule();
auto quarterly = themis::maintenance::defaultQuarterlySchedule();

// Register all defaults (disabled by default) and the IndexMaintenance health probe:
themis::maintenance::registerDefaultMaintenanceSetup(orchestrator, index_mgr);
```

| Function | Frequency | Tasks |
|----------|-----------|-------|
| `defaultDailySchedule()` | `DAILY` | `METRICS_COLLECTION`, `FRAGMENTATION_MONITORING`, `QUOTA_CHECK` |
| `defaultWeeklySchedule()` | `WEEKLY` | `CONSISTENCY_CHECK`, `REPLICA_VALIDATION`, `PERFORMANCE_ANALYSIS`, `MVCC_CLEANUP` |
| `defaultMonthlySchedule()` | `MONTHLY` | `FULL_CHECKDB`, `BACKUP_VERIFICATION`, `CAPACITY_TREND_ANALYSIS`, `INDEX_FRAGMENTATION_REPORT` |
| `defaultQuarterlySchedule()` | `QUARTERLY` | `DISASTER_RECOVERY_DRILL`, `BASELINE_UPDATE` |

> **Note:** There is no `MaintenanceRegistry` class and no `maintenance_registry.h` header.
> The free functions above are defined in `src/maintenance/maintenance_registry.cpp`.

## Scope

**In Scope:**
- Schedule CRUD (create, read, update, patch, delete, enable, disable)
- Cron-based execution via `TaskScheduler`
- Maintenance window enforcement (UTC hour range)
- Sequential task execution with halt-on-failure
- Explicit per-task DAG dependency graph with Kahn's topological sort and cycle detection
- RocksDB schedule persistence via `MaintenanceScheduleStore` (survives server restarts)
- Per-module health probe registry and aggregation
- Job lifecycle management (PENDING → RUNNING → SUCCEEDED/FAILED/CANCELLED/SKIPPED)
- 24-hour job retention with automatic pruning
- Audit logging and Prometheus-compatible metrics
- `IMaintenanceTaskHandler` registry — modules wire real execution logic via `registerTaskHandler()`
- `IDistributedLock` injection — prevents two cluster nodes from running the same schedule concurrently
- Multi-tenant schedule isolation — per-tenant maintenance windows and job quotas via `TenantMaintenanceConfig`

**Out of Scope:**
- Raft-backed distributed lock implementation (interface provided; Raft integration planned v2.1.0)
- REPLICA_VALIDATION wiring to sharding/replica module (handler class provided; startup wiring pending)
- Maintenance impact prediction (planned v3.0.0)

## Task Types (19)

Defined in `include/maintenance/maintenance_task.h` (`enum class MaintenanceTaskType`):

```
-- Daily --
METRICS_COLLECTION        FRAGMENTATION_MONITORING   QUOTA_CHECK

-- Weekly --
CONSISTENCY_CHECK         REPLICA_VALIDATION         PERFORMANCE_ANALYSIS
MVCC_CLEANUP

-- Monthly --
FULL_CHECKDB              BACKUP_VERIFICATION        CAPACITY_TREND_ANALYSIS
INDEX_FRAGMENTATION_REPORT

-- Quarterly --
DISASTER_RECOVERY_DRILL   BASELINE_UPDATE

-- On-demand --
INDEX_REBUILD             INDEX_REORGANIZE           STATISTICS_UPDATE
STORAGE_COMPACTION        ORPHAN_CLEANUP             VECTOR_REINDEX
```

## REST API

15 endpoints under `/api/v1/maintenance/`:

- `POST /schedules` — create schedule
- `GET /schedules` — list all (accepts `?tenant_id=` filter)
- `GET /schedules/{id}` — get by ID
- `PUT /schedules/{id}` — replace
- `PATCH /schedules/{id}` — partial update
- `DELETE /schedules/{id}` — delete
- `POST /schedules/{id}/enable` — enable
- `POST /schedules/{id}/disable` — disable
- `POST /schedules/{id}/run` — trigger now (optional `{"force": true}` body)
- `GET /jobs` — list recent jobs (last 24 hours)
- `GET /jobs/{id}` — get job details
- `POST /jobs/{id}/cancel` — cancel running job
- `GET /health` — aggregated health report
- `GET /task-handlers` — list registered task handlers
- `GET /status` — orchestrator status snapshot

**RBAC:** `maintenance:read` · `maintenance:write` · `maintenance:admin`

## Health Probe Registration

Modules can register health probes to contribute to the aggregated health report:

```cpp
orchestrator.registerHealthProbe("my_module", []() -> ModuleHealthSignal {
    ModuleHealthSignal signal;
    signal.module_name = "my_module";
    signal.status = ModuleHealthStatus::OK;
    signal.message = "All systems nominal";
    return signal;
});
```

## Tests

55+ unit tests in `tests/test_database_maintenance_orchestrator.cpp` covering:
- Schedule CRUD and validation
- JSON round-trips (`toJson()` / `fromJson()` / `applyPatch()`)
- Maintenance window enforcement and SKIPPED state
- Job lifecycle (SUCCEEDED, FAILED, CANCELLED)
- `halt_on_task_failure` cascading behaviour
- Health probe registration and aggregation
- Metrics collection
- RocksDB schedule persistence (restart-persistence round-trips)
- DAG dependency ordering, cycle detection, and missing predecessor rejection
- Force-run (`triggerNow(id, force=true)`)
- `IDistributedLock` integration (tryAcquire, skip on lock held by peer)
- Multi-tenant isolation (MT-01..MT-15): `tenant_id` round-trip, filter, window override, quota enforcement

## Wissenschaftliche Grundlagen

The following peer-reviewed sources form the scientific foundation of the Maintenance module.

### Database Maintenance and Self-Tuning

1. **Chaudhuri, S., & Weikum, G. (2000).**
   *Rethinking Database System Architecture: Towards a Self-Tuning RISC-Style Database System.*
   *Proceedings of the 26th International Conference on Very Large Data Bases (VLDB)*, 1–10.
   URL: https://dl.acm.org/doi/10.5555/645926.671577
   > Introduces the concept of self-tuning database components that monitor and adapt
   > internal parameters at runtime. Directly motivates the `MaintenanceOrchestrator`
   > adaptive scheduling model and the health-probe feedback loop in `health_probe.cpp`.

2. **Agrawal, S., Chaudhuri, S., Kollar, L., Marathe, A., Narasayya, V., & Syamala, M. (2004).**
   *Database Tuning Advisor for Microsoft SQL Server 2005.*
   *Proceedings of the 30th International Conference on Very Large Data Bases (VLDB)*, 1110–1121.
   URL: https://dl.acm.org/doi/10.5555/1316689.1316803
   > Describes automated index/statistics recommendation. Informs the
   > `REINDEX_HNSW` and `REBUILD_SECONDARY_INDEXES` task types and the
   > `halt_on_task_failure` cascading strategy.

### Scheduling Algorithms

3. **Liu, C. L., & Layland, J. W. (1973).**
   *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment.*
   *Journal of the ACM*, 20(1), 46–61.
   DOI: [10.1145/321738.321743](https://doi.org/10.1145/321738.321743)
   > Rate-Monotonic Scheduling (RMS) theory for periodic task sets. Informs the
   > maintenance-window priority model (`CRITICAL > HIGH > MEDIUM > LOW`) and the
   > `max_concurrent_tasks` admission-control bound in `TaskScheduler`.

4. **Silberschatz, A., Galvin, P. B., & Gagne, G. (2018).**
   *Operating System Concepts (10th ed.).*
   Wiley. ISBN: 978-1-119-32091-3.
   > Chapter 5 (CPU Scheduling) motivates the multi-level feedback queue used for
   > maintenance job priorities and the preemptive scheduling of `CRITICAL` tasks.

## Scientific References

1. Chaudhuri, S., & Weikum, G. (2000). **Rethinking Database System Architecture: Towards a Self-Tuning RISC-Style Database System**. *VLDB 2000*. https://dl.acm.org/doi/10.5555/645926.671577

2. Agrawal, S., et al. (2004). **Database Tuning Advisor for Microsoft SQL Server 2005**. *VLDB 2004*. https://dl.acm.org/doi/10.5555/1316689.1316803

3. Liu, C. L., & Layland, J. W. (1973). **Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment**. *Journal of the ACM*, 20(1), 46–61. https://doi.org/10.1145/321738.321743

4. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). **Operating System Concepts (10th ed.)**. Wiley. ISBN: 978-1-119-32091-3

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/maintenance/README.md`](../../include/maintenance/README.md) for the public API.
