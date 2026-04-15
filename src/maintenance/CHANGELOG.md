<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Maintenance Module

All notable changes to the Maintenance module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
*(none — see Planned section in ROADMAP.md)*

## [2.0.0] — 2026-04-13
### Added
- `IDistributedLock` interface + `InProcessDistributedLock` implementation
  (`include/maintenance/i_distributed_lock.h`) for single-process and
  single-node deployments; production multi-node coordination via Raft injection
- `setDistributedLock(shared_ptr<IDistributedLock>)` on orchestrator — RAII lock
  guard in `executeSchedule()` prevents two cluster nodes running same schedule
- `MaintenanceScheduleEntry::lock_ttl_ms` — per-schedule distributed lock TTL
  override; auto-computed from window duration + 30 s when 0
- Multi-tenant schedule isolation: `TenantMaintenanceConfig` struct with
  `enforce_window`, `window_start_hour`, `window_end_hour`, `max_concurrent_jobs`
- `setTenantMaintenanceConfig()` / `getTenantMaintenanceConfig()` — thread-safe
  per-tenant configuration injection; persisted in `tenant_configs_mutex_` / `shared_mutex`
- `MaintenanceScheduleEntry::tenant_id` — optional tenant identifier; empty = global/system
- `OrchestratorJob::tenant_id` — populated from parent schedule in `triggerNow()` and
  `registerWithScheduler()`
- `listSchedules(tenant_id_filter)` — filter schedules by tenant; API handler passes filter
- 15 new unit tests MT-01..MT-15 for multi-tenant isolation in
  `tests/test_database_maintenance_orchestrator.cpp`

## [1.2.0] — 2026-04-12
### Added
- `MaintenanceTaskDependency` struct — explicit per-task `depends_on` list
  (`include/maintenance/maintenance_schedule.h`)
- `MaintenanceScheduleEntry::task_dependencies` — optional DAG declaration;
  topological sort used when non-empty
- `DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder()` — static helper
  using Kahn's algorithm; detects cycles and missing predecessor references
- `IMaintenanceTaskHandler` registry: `registerTaskHandler()` / `listTaskHandlers()`
  (`include/maintenance/i_maintenance_task_handler.h`,
   `include/maintenance/maintenance_task_handler_impls.h`)
- `StorageCompactionHandler` wired to `CompactionManager::compactAll()` in
  `http_server.cpp` (Issue #4587)
- `MvccCleanupHandler` registered via shared `mvcc_store_` in `http_server.cpp`
  (Issue #4586)
- `GET /api/v1/maintenance/task-handlers` endpoint listing registered handlers
- `schedules_mutex_` and `jobs_mutex_` upgraded from `std::mutex` to
  `std::shared_mutex`; read operations use `std::shared_lock`
- `handlers_mutex_` and `tenant_configs_mutex_` added as `std::shared_mutex`

## [1.1.0] — 2026-04-10
### Added
- `MaintenanceScheduleStore` — RocksDB-backed schedule persistence
  (`include/maintenance/maintenance_schedule_store.h`,
   `src/maintenance/maintenance_schedule_store.cpp`)
  - Key format: `maint_sched::{id}` (UTF-8 JSON)
  - `loadAll()` called on `start()` before cron registration; corrupt JSON → WARN + skip
  - Write-through persistence in all CRUD mutations (`createSchedule`,
    `updateSchedule`, `patchSchedule`, `deleteSchedule`)
- `DatabaseMaintenanceOrchestrator` constructor gains optional `IStorageEngine*`
  parameter; `nullptr` → in-memory-only (default, backwards-compatible)
- Force-run override: `triggerNow(schedule_id, force=true)` bypasses UTC window check
- `POST /api/v1/maintenance/schedules/{id}/run` with optional `{"force": true}` body;
  requires `maintenance:admin` scope for force flag
- `forced: true` recorded in audit log entry when force is used

## [1.0.0] — 2026-03-11
### Added
- `DatabaseMaintenanceOrchestrator` — central coordinator for all DB maintenance
- Schedule CRUD: POST / GET / PUT / PATCH / DELETE with full validation
- Cron-based scheduling via `TaskScheduler::registerFunction()`
- Sequential task execution with `halt_on_task_failure` gate
- Maintenance window enforcement (UTC hour range, midnight wrap-around)
- Job lifecycle: PENDING → RUNNING → SUCCEEDED / FAILED / CANCELLED / SKIPPED
- 24-hour job retention with automatic pruning
- Per-module health probe registry → aggregated `MaintenanceHealthReport`
- Audit logging via `AuditLogger::logEvent()` for all CRUD and job events
- 11 Prometheus-compatible metrics via `MetricsCollector`
- `MaintenanceApiHandler` — 11 REST endpoints; 16 HTTP route cases in `http_server.cpp`
- `maintenance_registry.cpp` — default daily/weekly/monthly/quarterly schedule bundles
- 40+ unit tests covering CRUD, validation, JSON round-trips, window enforcement, jobs, health
- RBAC scopes: `maintenance:read` / `maintenance:write` / `maintenance:admin`
- `MaintenanceTaskType` enum with 19 task types
- Three-mutex thread safety model (schedules, jobs, health probes)
