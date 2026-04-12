<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-06 | Branch: copilot/add-centralized-maintenance-orchestrator -->

# Maintenance Module Roadmap

## Current Status
v1.0.0 – `DatabaseMaintenanceOrchestrator` implemented with full schedule CRUD,
maintenance window enforcement, audit logging, observability metrics, job
management, and aggregated health reporting.

## Completed ✅

- [x] `DatabaseMaintenanceOrchestrator` – central coordinator for all DB maintenance
  - Schedule CRUD: POST / GET / PUT / PATCH / DELETE
  - Cron-based scheduling via existing `TaskScheduler`
  - Sequential DAG execution of tasks with `halt_on_task_failure` gate
  - Maintenance window enforcement (SKIPPED when outside UTC hour window)
  - Audit logging via `AuditLogger::logEvent()` for all CRUD and job events
  - Metrics via `MetricsCollector` – 11 counters and histograms
  - Per-module health probe registry → aggregated `MaintenanceHealthReport`
  - Job lifecycle: PENDING → RUNNING → SUCCEEDED/FAILED/CANCELLED/SKIPPED
  - 24-hour job retention with automatic pruning
- [x] `maintenance_task.h` – `MaintenanceTaskType` (19 types), `OrchestratorJob`, `MaintenanceJobState`
- [x] `maintenance_schedule.h` – `MaintenanceScheduleEntry` with `toJson()`, `fromJson()`, `applyPatch()`
- [x] `maintenance_health_report.h` – `MaintenanceHealthReport`, `ModuleHealthSignal`, `ModuleHealthStatus`
- [x] `maintenance_registry.cpp` – default daily/weekly/monthly/quarterly schedule bundles
- [x] `MaintenanceApiHandler` – HTTP handler wiring all 11 REST endpoints
- [x] HTTP endpoints in `http_server.cpp` (16 Route cases)
- [x] Unit tests: 40+ tests covering CRUD, validation, JSON round-trips, health, jobs,
  window enforcement, halt-on-failure, SKIPPED state

## In Progress 🚧
*(none)*

## Planned Features 📋

### Short-term (v1.1.0)
- [x] Persist schedules to RocksDB so they survive server restarts (Target: v1.1.0)
  - `MaintenanceScheduleStore` class wrapping `IStorageEngine` API; key prefix `maint_sched::{id}`
  - Schedules reload on `start()` from RocksDB before cron registration
  - Write-through persistence in `createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule`
  - Corrupt schedule JSON → log WARN and skip that entry; all valid entries loaded
  - Restart-persistence integration tests + `MaintenanceScheduleStore` unit tests added
- [x] `POST /api/v1/maintenance/schedules/{id}/run` – window override flag `{"force": true}` (Target: v1.1.0)
  - Allows operator to bypass window enforcement for emergency maintenance
  - Audit log records `forced: true`

### Medium-term (v1.2.0)
- [x] DAG with explicit per-task predecessors (replace implicit list order) (Target: v1.2.0)
  - `MaintenanceTaskDependency` struct with `task_type` and `depends_on: [TaskType]` — `include/maintenance/maintenance_schedule.h` (commit de8a5ac414)
  - Topological sort via Kahn's algorithm in `DatabaseMaintenanceOrchestrator::resolveTaskOrder_()` — `include/maintenance/database_maintenance_orchestrator.h`
  - Cycle detection → rejects schedule creation; missing predecessor type → `ERR_UTIL_INVALID_ARGUMENT`
- [x] StorageCompaction integration (Target: v1.2.0)
  - `StorageCompactionHandler` in `include/maintenance/maintenance_task_handler_impls.h`: wraps `CompactionManager::compactAll()`
  - Register via `orchestrator.registerTaskHandler(STORAGE_COMPACTION, std::make_shared<StorageCompactionHandler>(mgr))`
- [ ] Replica consistency check integration (Target: v1.2.0)
  - Wire `REPLICA_VALIDATION` task to sharding/replica module once available
  - Health probe contributed by sharding module via `registerHealthProbe("replica", ...)`

### Long-term (v2.0.0)
- [ ] Multi-tenant schedule isolation – per-tenant windows and quotas (Target: v2.0.0)
- [ ] Distributed maintenance coordination via Raft – prevent two nodes running same schedule (Target: v2.0.0)
- [ ] Maintenance impact prediction – ML model to predict CPU/memory impact before execution (Target: v2.0.0)

## Production Readiness Checklist

- [x] Compilation: all `result.error().message()` calls correct
- [x] Thread safety: three separate mutexes for schedules, jobs, health probes
- [x] Graceful cancellation: running jobs check CANCELLED flag
- [x] Resource cleanup: 24-hour job TTL with automatic pruning
- [x] Audit trail: all state-changing operations logged
- [x] Observability: 11 Prometheus-compatible metrics
- [x] Maintenance windows: UTC hour-range check with midnight wrap-around
- [x] Input validation: name, tasks, cron_expression, window hours
- [x] Cascading failure control: `halt_on_task_failure`
- [x] HTTP RBAC: `maintenance:read` / `maintenance:write` / `maintenance:admin`
- [x] Schedule persistence (survives restart) – implemented v1.1.0 (`MaintenanceScheduleStore`, write-through CRUD, loadAll on start())
- [x] Explicit DAG dependency graph – implemented v1.2.0 (`MaintenanceTaskDependency`, Kahn's topological sort)

## Known Issues & Limitations

- Schedules are in-memory only (lost on server restart). Persistence to RocksDB is planned for v1.1.0.
- The `tasks` list implies a total order (first to last). Explicit dependency graphs (per-task `depends_on`) are planned for v1.2.0.
- Module-delegated tasks (`METRICS_COLLECTION`, `STORAGE_COMPACTION`, etc.) currently succeed immediately without calling the actual module. Wiring to real module methods requires each module to register a handler, which is documented in `docs/maintenance/MODULE_INTEGRATION_GUIDE.md`.
- `REPLICA_VALIDATION` and `MVCC_CLEANUP` tasks are not yet wired to real implementations; they are delegated to module health probes.

## Implementation Phases

### Phase 1: Core Infrastructure (Status: Completed ✅)
- [x] Headers: `maintenance_task.h`, `maintenance_schedule.h`, `maintenance_health_report.h`
- [x] `DatabaseMaintenanceOrchestrator` skeleton with schedule CRUD
- [x] `MaintenanceApiHandler` and HTTP route wiring
- [x] Build system: `cmake/ModularBuild.cmake` + test target in `tests/CMakeLists.txt`

### Phase 2: Scheduling & Execution (Status: Completed ✅)
- [x] `TaskScheduler` integration (cron registration via `registerFunction`/`registerTask`)
- [x] Sequential DAG execution with `halt_on_task_failure`
- [x] Job lifecycle management (PENDING/RUNNING/SUCCEEDED/FAILED/CANCELLED/SKIPPED)
- [x] 24-hour job retention + pruning

### Phase 3: Hardening & Audit (Status: Completed ✅)
- [x] Maintenance window enforcement (UTC hour range, midnight wrap-around)
- [x] Audit logging for all CRUD + job events via `AuditLogger`
- [x] MetricsCollector integration (11 metrics)
- [x] Bug fix: `result.error()` → `result.error().message()` in `executeTask()`
- [x] Removed duplicate implementation block

### Phase 4: Tests & Documentation (Status: Completed ✅)
- [x] 40+ unit tests (CRUD, validation, serialisation, health, jobs, window enforcement)
- [x] `docs/maintenance/ORCHESTRATOR_DESIGN.md`
- [x] `docs/maintenance/MAINTENANCE_SCHEDULE.md`
- [x] `docs/maintenance/MODULE_INTEGRATION_GUIDE.md`

### Phase 5: Persistence & Advanced DAG (Planned – v1.1.0 / v1.2.0)
- [ ] RocksDB persistence for schedules
- [ ] Explicit per-task dependency graph
- [ ] Module-specific task wiring: `StorageCompactionHandler` (compactAll), DAG execution

## Breaking Changes
*None – new module with no existing API contract.*
