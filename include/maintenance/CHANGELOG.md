<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Maintenance Module (Public Headers)

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/maintenance/CHANGELOG.md`.

## [Unreleased]
*(none)*

## [2.0.0] — 2026-04-13
### Added
- `database_maintenance_orchestrator.h`: `TenantMaintenanceConfig` struct,
  `setTenantMaintenanceConfig()`, `getTenantMaintenanceConfig()` — multi-tenant isolation
- `database_maintenance_orchestrator.h`: `MaintenanceScheduleEntry::tenant_id`,
  `OrchestratorJob::tenant_id`, `listSchedules(tenant_id_filter)` — tenant-scoped filtering
- `database_maintenance_orchestrator.h`: `MaintenanceScheduleEntry::lock_ttl_ms` — per-schedule distributed lock TTL
- `i_distributed_lock.h`: `IDistributedLock` interface + `InProcessDistributedLock` implementation
- `database_maintenance_orchestrator.h`: `setDistributedLock(shared_ptr<IDistributedLock>)`

## [1.2.0] — 2026-04-12
### Added
- `maintenance_schedule.h`: `MaintenanceTaskDependency` struct with `task_type` and `depends_on`
- `maintenance_schedule.h`: `MaintenanceScheduleEntry::task_dependencies` field
- `database_maintenance_orchestrator.h`: `resolveTaskExecutionOrder()` static helper (Kahn's sort + cycle detection)
- `database_maintenance_orchestrator.h`: `registerTaskHandler()`, `listTaskHandlers()`
- `database_maintenance_orchestrator.h`: `shared_mutex` for schedules, jobs, handlers, tenant_configs read paths
### Changed
- `database_maintenance_orchestrator.h`: `schedules_mutex_` and `jobs_mutex_` upgraded to `std::shared_mutex`

## [1.1.0] — 2026-04-10
### Added
- `maintenance_schedule_store.h`: `MaintenanceScheduleStore` RocksDB-backed CRUD
- `database_maintenance_orchestrator.h`: constructor gains optional `IStorageEngine*` parameter
- `database_maintenance_orchestrator.h`: `triggerNow(schedule_id, force=true)` force-run flag

## [1.0.0] — 2026-03-11
### Added
- `database_maintenance_orchestrator.h`: `DatabaseMaintenanceOrchestrator`, `OrchestratorJob`
- `i_maintenance_task_handler.h`: `IMaintenanceTaskHandler` base interface
- `maintenance_task_handler_impls.h`: `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler`
- `maintenance_schedule.h`: `MaintenanceScheduleEntry` with cron expression
- `maintenance_schedule_store.h`: `MaintenanceScheduleStore` CRUD
- `maintenance_task.h`: task and dependency types
- `maintenance_health_report.h`: `MaintenanceHealthReport`, `ModuleHealthSignal`
