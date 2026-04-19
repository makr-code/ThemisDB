<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ROADMAP.md · ../../src/maintenance/ -->

# Maintenance Module — Public Header Architecture
**Version:** 2.0.0
**Module Path:** `include/maintenance/`
**Implementation:** `../../src/maintenance/`

---

## Overview

The Maintenance module provides public headers for scheduled database maintenance orchestration: storage compaction, replica validation, MVCC cleanup, and function-based maintenance tasks managed via cron-based scheduling with health reporting, RocksDB persistence, DAG dependency graph, distributed lock coordination, and multi-tenant isolation.

## Design Principles

- **Orchestrator Pattern** — `DatabaseMaintenanceOrchestrator` is the single entry point; all task handlers implement `IMaintenanceTaskHandler`.
- **Pluggable Handlers** — `FunctionMaintenanceTaskHandler` allows arbitrary callables as maintenance tasks; `registerTaskHandler()` wires module-specific handlers.
- **Health Reporting** — `MaintenanceHealthReport` aggregates signals from all registered modules.
- **Schedule CRUD** — Full schedule lifecycle (POST/GET/PUT/PATCH/DELETE) with `MaintenanceScheduleStore` (RocksDB-backed).
- **Distributed Coordination** — `IDistributedLock` injection prevents two cluster nodes from executing the same schedule concurrently.
- **Multi-Tenant Isolation** — `TenantMaintenanceConfig` provides per-tenant maintenance windows and job quotas.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `database_maintenance_orchestrator.h` | `DatabaseMaintenanceOrchestrator`, `OrchestratorJob`, `TenantMaintenanceConfig` | Central maintenance coordinator |
| `i_distributed_lock.h` | `IDistributedLock`, `InProcessDistributedLock` | Distributed lock interface for cluster coordination |
| `i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` | Base interface for task handlers |
| `maintenance_task_handler_impls.h` | `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler` | Built-in task handler implementations |
| `maintenance_task.h` | `MaintenanceTaskDependency`, `MaintenanceTaskType`, `OrchestratorJob` | Task definition and dependency model |
| `maintenance_schedule.h` | `MaintenanceScheduleEntry`, `MaintenanceTaskDependency` | Cron-based schedule entry with DAG support |
| `maintenance_schedule_store.h` | `MaintenanceScheduleStore` | RocksDB-backed schedule persistence and CRUD |
| `maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal` | Aggregated health reporting |

## References

- Implementation: `../../src/maintenance/`
- `TaskScheduler` and `IndexMaintenanceManager` interfaces: `../../src/maintenance/`
