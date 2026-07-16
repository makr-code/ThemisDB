> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-05-13 | Commit: HEAD -->

# Maintenance Module — Public Headers

**Module Path:** `include/maintenance/`
**Implementation Overview:** [`../../src/maintenance/README.md`](../../src/maintenance/README.md)

## Purpose

Public headers for the ThemisDB maintenance orchestration subsystem. The module
provides centralized scheduling, execution, and observability for recurring
database maintenance operations: storage compaction, MVCC garbage collection,
replica validation, index rebuild, health reporting, RocksDB schedule
persistence, DAG-based task dependency resolution, distributed lock
coordination, and multi-tenant maintenance isolation.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `database_maintenance_orchestrator.h` | `DatabaseMaintenanceOrchestrator`, `TenantMaintenanceConfig` | Central coordinator: schedule CRUD, cron execution, job tracking, health aggregation, distributed lock integration |
| `maintenance_task.h` | `MaintenanceTaskType` (19 types), `OrchestratorJob`, `MaintenanceJobState` | Core data types: task enum, job struct, job lifecycle states; used across all module headers |
| `maintenance_schedule.h` | `MaintenanceScheduleEntry`, `ScheduleFrequency`, `MaintenanceTaskDependency` | Durable schedule entity with JSON CRUD serialisation (`toJson`/`fromJson`/`applyPatch`) and DAG dependency declarations |
| `maintenance_schedule_store.h` | `MaintenanceScheduleStore` | RocksDB-backed write-through persistence for schedule entries; key prefix `maint_sched::` |
| `maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal`, `ModuleHealthStatus` | Per-module health signal aggregation returned by `GET /api/v1/maintenance/health` |
| `i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` | Pluggable interface for delegating task execution to module-specific handlers |
| `maintenance_task_handler_impls.h` | `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionTaskHandler` | Built-in handler implementations for STORAGE_COMPACTION, REPLICA_VALIDATION, and MVCC_CLEANUP |
| `i_distributed_lock.h` | `IDistributedLock`, `InProcessDistributedLock` | Strategy interface for TTL-based distributed locks; `InProcessDistributedLock` ships for single-node and test use |

## Public API Behavior

### Schedule CRUD (`DatabaseMaintenanceOrchestrator`)

**Validation rules enforced by `createSchedule` / `updateSchedule`:**
- `name` must be non-empty; an empty name returns an error.
- `tasks` must contain at least one `MaintenanceTaskType` entry.
- When `frequency == ScheduleFrequency::CUSTOM`, `cron_expression` must be
  non-empty.
- `window_start_hour` and `window_end_hour` must each be in `[0, 23]`.

**Full-replace vs. partial update:**
- `updateSchedule(id, entry)` — PUT semantics: replaces all mutable fields;
  `created_at_ms` and `created_by` are preserved from the original record.
- `patchSchedule(id, patch)` — PATCH semantics: only JSON fields present in
  `patch` are applied; all other fields retain their current values.

**Schedule enabling/disabling:**
- `enableSchedule(id)` / `disableSchedule(id)` toggle the `enabled` flag and
  immediately update the `TaskScheduler` registration.
- A disabled schedule is stored but never fired by the cron mechanism;
  `triggerNow(id)` bypasses this check.

### Job Lifecycle (`OrchestratorJob`)

```
PENDING → RUNNING → SUCCEEDED
                  → FAILED
                  → CANCELLED   (operator-initiated via cancelJob)
                  → SKIPPED     (outside maintenance window, or distributed lock held by peer)
```

- Completed jobs are retained in-memory for **24 hours** (configurable via
  `kJobRetentionMs`) and then pruned automatically.
- `triggerNow(id, force=true)` bypasses the UTC maintenance window check;
  the resulting `OrchestratorJob` has `forced = true`.
- When `halt_on_task_failure = true` on the schedule, any task that returns
  an error causes all subsequent tasks in the same job to be skipped.
- DAG task dependencies declared in `task_dependencies` are resolved via
  Kahn's topological sort before execution. A cycle or a reference to an
  unknown task type throws `std::invalid_argument`.

### Maintenance Window Enforcement

- The window is defined by `[window_start_hour, window_end_hour)` in UTC
  (both 0–23).
- If `enforce_window = true` (default) and the current UTC hour falls outside
  the window, the job state is set to `SKIPPED`.
- Per-tenant window overrides can be applied via `setTenantMaintenanceConfig`.
  When `TenantMaintenanceConfig::enforce_window = true`, the tenant window
  takes precedence over the per-schedule window.

### Distributed Lock (`IDistributedLock`)

- Inject an `IDistributedLock` implementation via `setDistributedLock(lock)`.
- Before each scheduled job fires, the orchestrator calls
  `lock->tryAcquire(schedule_id, ttl_ms)`.
- If `tryAcquire` returns `false`, the job state is set to `SKIPPED`; a
  DEBUG-level log message records which peer holds the lock.
- TTL is derived from `MaintenanceScheduleEntry::lock_ttl_ms` when non-zero;
  otherwise it is computed as the window duration plus a 30-second safety
  margin.
- Pass `nullptr` to `setDistributedLock` to disable distributed coordination
  (single-node or test deployments).

### Health Probes (`registerHealthProbe`)

- Modules register a `HealthProbe` callable once at startup.
- Probes are called **synchronously** during `getHealthReport()` — they must
  complete in under 10 ms and must not block on I/O or take locks.
- The overall `MaintenanceHealthReport::overall_status` is the worst
  `ModuleHealthStatus` across all registered probes (`OK < DEGRADED < CRITICAL`).

### RocksDB Schedule Persistence (`MaintenanceScheduleStore`)

- Every `createSchedule`, `updateSchedule`, `patchSchedule`, and
  `deleteSchedule` call is written through to RocksDB synchronously before the
  caller's mutex is released.
- On `start()`, all stored schedules are reloaded from the `maint_sched::`
  prefix.  Entries with corrupt or unparseable JSON are skipped with a
  WARN-level log; all valid entries are loaded.
- Pass `storage = nullptr` to the orchestrator constructor for in-memory-only
  operation (suitable for tests).

## Configuration and Limits

| Parameter | Default | Description |
|-----------|---------|-------------|
| `window_start_hour` | `2` | Inclusive start of maintenance window (UTC hour, 0–23) |
| `window_end_hour` | `6` | Exclusive end of maintenance window (UTC hour, 0–23) |
| `enabled` | `true` | Whether the schedule fires automatically |
| `enforce_window` | `true` | Abort job when started outside the defined window |
| `halt_on_task_failure` | `false` | Stop subsequent tasks if any task fails |
| `lock_ttl_ms` | `0` (auto) | Distributed lock TTL; 0 = window duration + 30 s |
| `TenantMaintenanceConfig::max_concurrent_jobs` | `0` (unlimited) | Per-tenant concurrent job cap |
| `kJobRetentionMs` | 24 h | How long completed jobs are kept in memory |

## Installation

This module is included as part of ThemisDB. Add the include directory to your
target:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

### Basic orchestrator setup

```cpp
#include "maintenance/database_maintenance_orchestrator.h"

// Construct (storage=nullptr → in-memory only)
DatabaseMaintenanceOrchestrator orchestrator(
    &scheduler,          // TaskScheduler* (required)
    index_maintenance,   // std::shared_ptr<IndexMaintenanceManager> (optional)
    audit_logger,        // std::shared_ptr<utils::AuditLogger> (optional)
    nullptr              // IStorageEngine* (nullptr = no persistence)
);
orchestrator.start();

// Create a nightly schedule
MaintenanceScheduleEntry schedule;
schedule.name              = "Nightly Cleanup";
schedule.frequency         = ScheduleFrequency::DAILY;
schedule.window_start_hour = 2;
schedule.window_end_hour   = 5;
schedule.tasks             = { MaintenanceTaskType::MVCC_CLEANUP,
                               MaintenanceTaskType::STORAGE_COMPACTION };
schedule.halt_on_task_failure = true;

auto result = orchestrator.createSchedule(schedule);
if (!result) { /* handle error */ }

// Trigger ad-hoc run (force = bypass window)
auto job = orchestrator.triggerNow(result->id, /*force=*/true);

// Query health
MaintenanceHealthReport report = orchestrator.getHealthReport();
```

### DAG task dependency example

```cpp
#include "maintenance/maintenance_schedule.h"

MaintenanceScheduleEntry entry;
entry.name  = "Ordered Cleanup";
entry.tasks = { MaintenanceTaskType::STORAGE_COMPACTION,
                MaintenanceTaskType::MVCC_CLEANUP };

// Ensure MVCC_CLEANUP runs before STORAGE_COMPACTION
MaintenanceTaskDependency dep;
dep.task_type  = MaintenanceTaskType::STORAGE_COMPACTION;
dep.depends_on = { MaintenanceTaskType::MVCC_CLEANUP };
entry.task_dependencies = { dep };
```

### Registering a custom task handler

```cpp
#include "maintenance/i_maintenance_task_handler.h"
#include "maintenance/database_maintenance_orchestrator.h"

class MyIndexHandler : public IMaintenanceTaskHandler {
public:
    Result<std::string> execute(const std::string& job_id,
                                MaintenanceTaskType) override {
        // … perform index rebuild …
        return Ok(std::string("Index rebuilt successfully"));
    }
    std::string handlerName() const override { return "MyIndexHandler"; }
};

orchestrator.registerTaskHandler(
    MaintenanceTaskType::INDEX_REBUILD,
    std::make_shared<MyIndexHandler>());
```

### Registering a health probe

```cpp
#include "maintenance/database_maintenance_orchestrator.h"

orchestrator.registerHealthProbe("my_module", []() -> ModuleHealthSignal {
    ModuleHealthSignal sig;
    sig.module_name  = "my_module";
    sig.status       = ModuleHealthStatus::OK;
    sig.message      = "All systems nominal";
    sig.checked_at_ms = /* current Unix ms */;
    return sig;
});
```

### Distributed lock setup

```cpp
#include "maintenance/i_distributed_lock.h"

// Single-node / test: use InProcessDistributedLock
auto lock = std::make_shared<InProcessDistributedLock>("node-1");
orchestrator.setDistributedLock(lock);

// Production: inject your Raft-backed IDistributedLock implementation
// orchestrator.setDistributedLock(raft_lock_impl);
```

### Multi-tenant configuration

```cpp
TenantMaintenanceConfig cfg;
cfg.enforce_window    = true;
cfg.window_start_hour = 3;
cfg.window_end_hour   = 5;
cfg.max_concurrent_jobs = 2;

orchestrator.setTenantMaintenanceConfig("tenant-acme", cfg);
```

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|--------------|------------|
| `createSchedule` returns error `"name must not be empty"` | `MaintenanceScheduleEntry::name` is empty | Set a non-empty human-readable `name` |
| `createSchedule` returns error `"tasks must not be empty"` | `tasks` vector is empty | Add at least one `MaintenanceTaskType` to `tasks` |
| `createSchedule` returns error `"cron_expression must not be empty"` | `frequency == CUSTOM` but `cron_expression` is empty | Provide a valid 5-field cron expression |
| Job state is `SKIPPED` immediately | Current UTC hour is outside `[window_start_hour, window_end_hour)` | Use `triggerNow(id, force=true)` to bypass the window, or adjust the window hours |
| Job state is `SKIPPED` with log "lock held by peer" | Another node holds the distributed lock | Expected in multi-node clusters; only one node runs each schedule at a time |
| `resolveTaskExecutionOrder` throws `std::invalid_argument` | A cycle exists in `task_dependencies`, or a dependency references a task not listed in `tasks` | Verify DAG structure: all dependency targets must appear in `tasks`; break any cycles |
| Health probe data is stale | Probe callable is slow (> 10 ms) or blocks | Keep probes fast and non-blocking; return cached signals if live queries are expensive |
| Schedules lost after server restart | `IStorageEngine` was not provided to the orchestrator | Pass a non-null `IStorageEngine*` (e.g., `StorageEngine`) to enable RocksDB persistence |
| `listJobs` returns no completed jobs | 24-hour retention window elapsed | Completed jobs are pruned after `kJobRetentionMs` (24 h); query sooner or persist externally |

## Related Docs

- Implementation overview: [`../../src/maintenance/README.md`](../../src/maintenance/README.md)
- Architecture: [`../../src/maintenance/ARCHITECTURE.md`](../../src/maintenance/ARCHITECTURE.md)
- Roadmap: [`../../src/maintenance/ROADMAP.md`](../../src/maintenance/ROADMAP.md)
- Future enhancements: [`../../src/maintenance/FUTURE_ENHANCEMENTS.md`](../../src/maintenance/FUTURE_ENHANCEMENTS.md)
- Changelog: [`../../src/maintenance/CHANGELOG.md`](../../src/maintenance/CHANGELOG.md)
- Security: [`../../src/maintenance/SECURITY.md`](../../src/maintenance/SECURITY.md)
- Orchestrator design: [`../../docs/maintenance/ORCHESTRATOR_DESIGN.md`](../../docs/maintenance/ORCHESTRATOR_DESIGN.md)
- Module integration guide: [`../../docs/maintenance/MODULE_INTEGRATION_GUIDE.md`](../../docs/maintenance/MODULE_INTEGRATION_GUIDE.md)
- Module index (DE): [`../../docs/de/maintenance/README.md`](../../docs/de/maintenance/README.md)
- Cross-module roadmap: [`../../ROADMAP.md`](../../ROADMAP.md)
- Cross-module future enhancements: [`../../FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md)
