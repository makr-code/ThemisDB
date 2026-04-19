> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-15 | Commit: e963d4e9ba -->

# Maintenance Module Roadmap

## Current Status
v2.0.0 – `DatabaseMaintenanceOrchestrator` production-ready with full schedule
CRUD, cron-based scheduling, maintenance window enforcement, audit logging,
observability metrics, job management, aggregated health reporting, RocksDB
schedule persistence, DAG dependency execution, distributed maintenance
coordination via `IDistributedLock`, and multi-tenant schedule isolation.

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
- [x] Multi-tenant schedule isolation – per-tenant windows and quotas (Target: v2.0.0)
  - `MaintenanceScheduleEntry::tenant_id` (optional; empty = global/system schedule)
  - Per-tenant window enforcement via `TenantMaintenanceConfig::enforce_window`; configured via `setTenantMaintenanceConfig()`
  - Per-tenant concurrent job quota: `TenantMaintenanceConfig::max_concurrent_jobs`; enforced in `executeSchedule()`
  - `listSchedules(tenant_id_filter)`: filter schedules by tenant; `MaintenanceApiHandler::listSchedules(tenant_id)` passes filter
  - `OrchestratorJob::tenant_id` populated from parent schedule
  - 15 new tests (MT-01..MT-15) covering field round-trip, filter, window override, and quota enforcement
- [x] Distributed maintenance coordination via Raft – prevent two nodes running same schedule (Target: v2.0.0)
  - `IDistributedLock` interface + `InProcessDistributedLock` implementation in `include/maintenance/i_distributed_lock.h`
  - `DatabaseMaintenanceOrchestrator::setDistributedLock(shared_ptr<IDistributedLock>)` — inject via DI
  - Before each scheduled job: `tryAcquire(schedule_id, ttl_ms)`; SKIPPED + DEBUG log when lock held by peer
  - Lock TTL auto-derived from window duration + 30 s, or explicit `MaintenanceScheduleEntry::lock_ttl_ms`
  - RAII guard ensures lock release on every exit path (success, window skip, DAG error, cancellation)
- [ ] Raft-backed `IDistributedLock` implementation – production multi-node coordination (Target: v2.1.0)
  - In-process `InProcessDistributedLock` available; Raft-backed impl requires `src/replication/raft_v2.cpp` integration
- [ ] Maintenance impact prediction – ML model to predict CPU/memory impact before execution (Target: v3.0.0)

## Production Readiness Checklist

- [x] Compilation: all `result.error().message()` calls correct
- [x] Thread safety: `shared_mutex` for schedules, jobs, handlers, tenant_configs; `mutex` for health probes and dist lock
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
- [x] Distributed maintenance coordination – implemented v2.0.0 (`IDistributedLock`, `setDistributedLock()`, RAII lock guard, per-schedule TTL)
- [x] Multi-tenant schedule isolation – implemented v2.0.0 (`TenantMaintenanceConfig`, `setTenantMaintenanceConfig()`, per-tenant window + quota)
- [x] IMaintenanceTaskHandler registry – implemented v1.2.0 (`registerTaskHandler`, `listTaskHandlers`)
- [x] STORAGE_COMPACTION wired – implemented v1.2.0 (`StorageCompactionHandler` in `http_server.cpp`)
- [x] MVCC_CLEANUP wired – implemented v1.2.0 (`MvccCleanupHandler` in `http_server.cpp`)
- [ ] REPLICA_VALIDATION wired – pending (handler interface ready; sharding module wiring pending)

## Known Issues & Limitations

- `REPLICA_VALIDATION` tasks are not yet wired to real implementations; the `ReplicaValidationHandler` class is provided in `maintenance_task_handler_impls.h` but the sharding/replica module has not yet registered a handler. Tracking: `include/maintenance/ROADMAP.md` planned item.
- Raft-backed `IDistributedLock` implementation not yet available; use `InProcessDistributedLock` for single-node or test deployments.
- `MVCC_CLEANUP` is wired (2026-04-12): `MvccCleanupHandler` registered in `http_server.cpp` using the shared `mvcc_store_` member.
- `STORAGE_COMPACTION` is wired (2026-04-12): `StorageCompactionHandler` registered in `http_server.cpp` via `CompactionManager`.

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

### Phase 5: Persistence & Advanced DAG (Status: Completed ✅ — v1.1.0 / v1.2.0)
- [x] RocksDB persistence for schedules (`MaintenanceScheduleStore`, write-through CRUD, loadAll on start())
- [x] Explicit per-task dependency graph (`MaintenanceTaskDependency`, `resolveTaskExecutionOrder`, Kahn's topological sort)
- [x] Module-specific task wiring: `StorageCompactionHandler`, `MvccCleanupHandler`, `ReplicaValidationHandler` (handler classes); `registerTaskHandler()` registry

### Phase 6: Distributed Coordination & Multi-Tenancy (Status: Completed ✅ — v2.0.0)
- [x] `IDistributedLock` interface + `InProcessDistributedLock` implementation
- [x] `setDistributedLock()` DI injection; RAII lock guard in `executeSchedule()`
- [x] Multi-tenant schedule isolation: `TenantMaintenanceConfig`, `setTenantMaintenanceConfig()`, per-tenant window and quota enforcement
- [x] 15 multi-tenant tests (MT-01..MT-15) in `test_database_maintenance_orchestrator.cpp`

### Phase 7: Production Hardening (Planned — v2.1.0+)
- [ ] Raft-backed `IDistributedLock` implementation (integrate with `src/replication/raft_v2.cpp`)
- [ ] REPLICA_VALIDATION wired to sharding/replica module
- [ ] Maintenance impact prediction (ML model for CPU/memory forecasting)

## Breaking Changes
*None – new module with no existing API contract.*
