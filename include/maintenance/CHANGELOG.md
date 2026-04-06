<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Maintenance Module (Public Headers)

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
For implementation-level changes see `../../src/maintenance/CHANGELOG.md`.

## [Unreleased]
- RocksDB schedule persistence (v1.1.0)
- Force-run override `{"force": true}` (v1.1.0)
- Explicit DAG dependency graph (v1.2.0)
- `STORAGE_COMPACTION` wired to `CompactionManager` (v1.2.0)

## [1.0.0] — 2026-03-11
### Added
- `database_maintenance_orchestrator.h`: `DatabaseMaintenanceOrchestrator`, `OrchestratorJob`
- `i_maintenance_task_handler.h`: `IMaintenanceTaskHandler` base interface
- `maintenance_task_handler_impls.h`: `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler`
- `maintenance_schedule.h`: `MaintenanceScheduleEntry` with cron expression
- `maintenance_schedule_store.h`: `MaintenanceScheduleStore` CRUD
- `maintenance_task.h`: task and dependency types
- `maintenance_health_report.h`: `MaintenanceHealthReport`, `ModuleHealthSignal`
