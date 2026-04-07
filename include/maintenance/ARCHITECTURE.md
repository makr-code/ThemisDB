<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/maintenance/ -->

# Maintenance Module — Public Header Architecture
**Version:** 1.0.0  
**Module Path:** `include/maintenance/`  
**Implementation:** `../../src/maintenance/`

---

## Overview

The Maintenance module provides public headers for scheduled database maintenance orchestration: storage compaction, replica validation, MVCC cleanup, and function-based maintenance tasks managed via cron-based scheduling with health reporting.

## Design Principles

- **Orchestrator Pattern** — `DatabaseMaintenanceOrchestrator` is the single entry point; all task handlers implement `IMaintenanceTaskHandler`.
- **Pluggable Handlers** — `FunctionMaintenanceTaskHandler` allows arbitrary callables as maintenance tasks.
- **Health Reporting** — `MaintenanceHealthReport` aggregates signals from all registered modules.
- **Schedule CRUD** — Full schedule lifecycle (POST/GET/PUT/PATCH/DELETE) with `MaintenanceScheduleStore`.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `database_maintenance_orchestrator.h` | `DatabaseMaintenanceOrchestrator`, `OrchestratorJob` | Central maintenance coordinator |
| `i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` | Base interface for task handlers |
| `maintenance_task_handler_impls.h` | `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler` | Built-in task handler implementations |
| `maintenance_task.h` | `MaintenanceTaskDependency` | Task definition and dependency model |
| `maintenance_schedule.h` | `MaintenanceScheduleEntry` | Cron-based schedule entry |
| `maintenance_schedule_store.h` | `MaintenanceScheduleStore` | Schedule persistence and CRUD |
| `maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal` | Aggregated health reporting |

## References

- Implementation: `../../src/maintenance/`
- `TaskScheduler` and `IndexMaintenanceManager` interfaces: `../../src/maintenance/`
