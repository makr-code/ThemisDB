<!-- Status: current | validated: 2026-04-06 -->
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
| `include/maintenance/maintenance_schedule.h` | Schedule entry with JSON serialization |
| `include/maintenance/maintenance_health_report.h` | Health report aggregation |
| `src/maintenance/database_maintenance_orchestrator.cpp` | Implementation |
| `src/maintenance/maintenance_registry.cpp` | Default schedule bundles |

## Key Classes

### `DatabaseMaintenanceOrchestrator`

Central coordinator for all maintenance scheduling and execution.

```cpp
#include "maintenance/database_maintenance_orchestrator.h"

// Construction (via dependency injection)
auto orchestrator = DatabaseMaintenanceOrchestrator(
    scheduler,           // TaskScheduler*
    index_maintenance,   // std::shared_ptr<IndexMaintenanceManager>
    audit_logger         // std::shared_ptr<utils::AuditLogger>
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

### `MaintenanceRegistry`

Provides pre-built schedule bundles for common maintenance patterns:

```cpp
#include "maintenance/maintenance_registry.h"

// Get default schedule bundles
auto daily_schedules   = MaintenanceRegistry::getDailySchedules();
auto weekly_schedules  = MaintenanceRegistry::getWeeklySchedules();
auto monthly_schedules = MaintenanceRegistry::getMonthlySchedules();
```

## Scope

**In Scope:**
- Schedule CRUD (create, read, update, patch, delete, enable, disable)
- Cron-based execution via `TaskScheduler`
- Maintenance window enforcement (UTC hour range)
- Sequential task execution with halt-on-failure
- Per-module health probe registry and aggregation
- Job lifecycle management (PENDING → RUNNING → SUCCEEDED/FAILED/CANCELLED/SKIPPED)
- 24-hour job retention with automatic pruning
- Audit logging and Prometheus-compatible metrics

**Out of Scope:**
- Schedule persistence (planned v1.1.0 — currently in-memory only)
- Explicit DAG task dependencies (planned v1.2.0 — currently total order)
- Distributed maintenance coordination (planned v2.0.0)

## Task Types (19)

```
INDEX_REBUILD         INDEX_OPTIMIZE        INDEX_CONSISTENCY_CHECK
STORAGE_COMPACTION    WAL_ARCHIVING         BACKUP_VERIFICATION
METRICS_COLLECTION    LOG_ROTATION          CACHE_WARM
DEAD_LETTER_DRAIN     REPLICA_VALIDATION    MVCC_CLEANUP
SCHEMA_VALIDATION     RETENTION_ENFORCEMENT STATISTICS_UPDATE
SECURITY_SCAN         AUDIT_LOG_FLUSH       BLOOM_FILTER_REBUILD
CUSTOM
```

## REST API

11 endpoints under `/api/v1/maintenance/`:

- `POST /schedules` — create schedule
- `GET /schedules` — list all
- `GET /schedules/{id}` — get by ID
- `PUT /schedules/{id}` — replace
- `PATCH /schedules/{id}` — partial update
- `DELETE /schedules/{id}` — delete
- `POST /schedules/{id}/enable` — enable
- `POST /schedules/{id}/disable` — disable
- `GET /jobs` — list recent jobs (last 24 hours)
- `GET /jobs/{id}` — get job details
- `GET /health` — aggregated health report

**RBAC:** `maintenance:read` · `maintenance:write` · `maintenance:admin`

## Health Probe Registration

Modules can register health probes to contribute to the aggregated health report:

```cpp
orchestrator.registerHealthProbe("my_module", []() -> ModuleHealthSignal {
    ModuleHealthSignal signal;
    signal.module_name = "my_module";
    signal.status = ModuleHealthStatus::HEALTHY;
    signal.message = "All systems nominal";
    return signal;
});
```

## Tests

40+ unit tests in `tests/test_maintenance_orchestrator.cpp` covering:
- Schedule CRUD and validation
- JSON round-trips (`toJson()` / `fromJson()` / `applyPatch()`)
- Maintenance window enforcement and SKIPPED state
- Job lifecycle (SUCCEEDED, FAILED, CANCELLED)
- `halt_on_task_failure` cascading behaviour
- Health probe registration and aggregation
- Metrics collection

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
