> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/maintenance/ARCHITECTURE.md -->

# Maintenance Module — Public Header Architecture

**Module Path:** `include/maintenance/`  
**Implementation:** `../../src/maintenance/`  
**Canonical architecture doc:** [`../../src/maintenance/ARCHITECTURE.md`](../../src/maintenance/ARCHITECTURE.md)

---

## 1. Overview

`include/maintenance/` defines the **public database maintenance task scheduling, distributed lock coordination, task handler dispatch, and health reporting API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/maintenance/ARCHITECTURE.md`](../../src/maintenance/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Maintenance Orchestration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `database_maintenance_orchestrator.h` | `DatabaseMaintenanceOrchestrator` | Coordinates maintenance tasks across shards |
| `maintenance_task.h` | `MaintenanceTask` | Maintenance task descriptor |
| `maintenance_task_handler_impls.h` | `MaintenanceTaskHandlerImpls` | Built-in task handler implementations |
| `i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` | Task handler interface |
### 2.2 Scheduling and Locking

| Header | Public Type | Purpose |
|--------|------------|---------|
| `maintenance_schedule.h` | `MaintenanceSchedule` | Cron-based maintenance schedule |
| `maintenance_schedule_store.h` | `MaintenanceScheduleStore` | Persistent schedule storage |
| `i_distributed_lock.h` | `IDistributedLock` | Distributed lock interface for task exclusivity |
### 2.3 Health and Reporting

| Header | Public Type | Purpose |
|--------|------------|---------|
| `maintenance_health_report.h` | `MaintenanceHealthReport` | Post-maintenance health status report |

---

## 3. Namespace Layout

All public types reside in the `themis::maintenance` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/maintenance/` expose the **stable public API**; internal types live in `src/maintenance/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
