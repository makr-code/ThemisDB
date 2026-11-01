> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-10-31 -->
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
| `database_maintenance_orchestrator.h` | `DatabaseMaintenanceOrchestrator`, `TenantMaintenanceConfig`, `HealthProbe` | Schedule/job lifecycle, policy configuration, and module health registration |
| `maintenance_task.h` | `MaintenanceTaskType`, `MaintenanceJobState`, `OrchestratorJob` | Task tokens and job-state/data representation |
| `maintenance_task_handler_impls.h` | `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler` | Optional handler implementations; delegated work executes only when registered |
| `i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` | Task handler interface |
### 2.2 Scheduling and Locking

| Header | Public Type | Purpose |
|--------|------------|---------|
| `maintenance_schedule.h` | `MaintenanceScheduleEntry`, `ScheduleFrequency`, `MaintenanceTaskDependency` | Schedule data, frequency presets, and optional task dependencies |
| `maintenance_schedule_store.h` | `MaintenanceScheduleStore` | Optional JSON persistence through `IStorageEngine` |
| `i_distributed_lock.h` | `IDistributedLock` | Distributed lock interface for task exclusivity |
### 2.3 Health and Reporting

| Header | Public Type | Purpose |
|--------|------------|---------|
| `maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal`, `DispatchOutcome` | Aggregate health and bounded dispatch diagnostics |

---

## 3. Namespace Layout

All public types reside in the `themis::maintenance` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/maintenance/` expose the **stable public API**; internal types live in `src/maintenance/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- `maintenance_api_contract.h` declares compatibility helper error, descriptor, and snapshot types for focused tests/benchmarks. It explicitly does not define runtime guarantees; the orchestrator uses `MaintenanceScheduleEntry`, `MaintenanceHealthReport`, and repository `Result`/`Error` values.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- **TODO/Assumption:** The legacy **Graph** layer association is not verified
  against current maintenance ownership; see the public-header roadmap.
