# Architecture - Maintenance Module

---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The maintenance module provides an orchestration layer for schedule definition, job execution, optional persistence/reload, and handler-driven task dispatch.

## Main Execution Planes

1. Schedule orchestration plane
- schedule lifecycle operations and run coordination
- job state tracking and execution sequencing

2. Persistence and recovery plane
- optional maintenance schedule storage and reload behavior
- valid entries are restored; malformed JSON entries are skipped and scan errors are surfaced by the store

3. Registry and setup plane
- creation of four disabled default schedules
- maintenance bootstrap and integration hooks

## Core Contracts

| Contract | Behavior |
|---|---|
| schedule contract | deterministic create/update/delete/list and run behavior |
| execution contract | per-schedule in-flight guard, optional lock/quota/window checks, tracking, and error propagation |
| persistence contract | write-through schedule mutations and partial reload of valid records |
| registry contract | deterministic default setup registration behavior |

## Failure Semantics

- invalid schedule input and failed writes return errors; malformed stored JSON is skipped, while a failed prefix scan is logged and startup can continue.
- a missing delegated handler yields a task-level skip (`SKIPPED_NO_HANDLER`), not a schedule-creation rejection.
- a failure from an available handler or missing direct index dependency marks the task failed; prior task side effects are not rolled back.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| scheduler | `include/scheduler/task_scheduler.h` | Registers and removes recurring cron tasks |
| index maintenance | `include/storage/index_maintenance.h` | Handles selected index task tokens through `IndexMaintenanceManager::triggerMaintenanceCheck()` |
| storage | `include/themis/base/interfaces/storage_interface.h` | Schedule-store persistence through `IStorageEngine` |
| observability | `include/observability/metrics_collector.h`, `include/utils/audit_logger.h` | Emits metrics and optional maintenance audit events |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/server/maintenance_api_handler.h`, `include/maintenance/database_maintenance_orchestrator.h` | Translates HTTP requests to orchestrator calls; route authorization is owned by the HTTP server |

## Integration Points

### Critical Integration: DatabaseMaintenanceOrchestrator ↔ Server API Handler
**Files:** `include/maintenance/database_maintenance_orchestrator.h`, `include/server/maintenance_api_handler.h` ↔ `src/server/`
**Contract:** `MaintenanceApiHandler` translates HTTP requests into orchestrator calls; the orchestrator owns the in-memory schedule state and dispatches task work. The handler does not own schedule state. Authorization is enforced by HTTP route registration, not by the handler or orchestrator.
**Execution:** Scheduled callbacks call `executeSchedule`; `triggerNow` starts a detached worker thread. Orchestrator state access is guarded by its mutexes; this is not a separate serialized dispatch queue.
**Failure Mode:** A missing delegated task handler produces `SKIPPED_NO_HANDLER` at task level. Direct index tasks fail when `IndexMaintenanceManager` is unavailable. Neither condition rejects schedule creation solely because the capability is absent.

### Task dispatch ↔ owning module
**Contract:** `INDEX_REBUILD`, `INDEX_REORGANIZE`, `STATISTICS_UPDATE`, `ORPHAN_CLEANUP`, `FRAGMENTATION_MONITORING`, `VECTOR_REINDEX`, and `CONSISTENCY_CHECK` are handled directly using `IndexMaintenanceManager` (the first four and vector re-index trigger `triggerMaintenanceCheck`; the latter two report active index-job counts). Other task types, including `STORAGE_COMPACTION` and `MVCC_CLEANUP`, require a registered `IMaintenanceTaskHandler`.
**Boundary:** `StorageCompactionHandler` calls `CompactionManager::compactAll()` and `MvccCleanupHandler` calls `MVCCStore::gcAllBefore()`. These are optional handler implementations, not direct storage/transaction calls in `DatabaseMaintenanceOrchestrator`.
**Failure Mode:** Missing handlers create a task-level skip; delegated errors and unavailable direct index dependencies are reported as task failures. A later task failure does not roll back earlier side effects.

- Verified files:
  - src/maintenance/database_maintenance_orchestrator.cpp
  - src/maintenance/maintenance_schedule_store.cpp
  - src/maintenance/maintenance_registry.cpp
- Verified architecture claims:
  - explicit orchestration, persistence, and registry planes
  - deterministic failure boundaries for schedule and handler paths
  - module-local ownership of maintenance orchestration behavior