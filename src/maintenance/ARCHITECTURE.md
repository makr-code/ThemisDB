# Architecture - Maintenance Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The maintenance module provides a bounded orchestration layer for maintenance schedule definition, job execution, persistence/reload, and handler-driven task dispatch.

## Main Execution Planes

1. Schedule orchestration plane
- schedule lifecycle operations and run coordination
- job state tracking and execution sequencing

2. Persistence and recovery plane
- maintenance schedule storage and reload behavior
- deterministic restoration of persisted schedule state

3. Registry and setup plane
- default schedule bundle registration
- maintenance bootstrap and integration hooks

## Core Contracts

| Contract | Behavior |
|---|---|
| schedule contract | deterministic create/update/delete/list and run behavior |
| execution contract | bounded job dispatch, tracking, and error propagation |
| persistence contract | explicit save/load semantics for schedule state |
| registry contract | deterministic default setup registration behavior |

## Failure Semantics

- invalid schedule input or missing prerequisites fail with explicit outcomes.
- persistence failures are surfaced as explicit orchestration errors.
- unavailable handlers fail deterministically rather than silently bypassing execution.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/compaction_manager.h` | Triggers RocksDB compaction jobs as scheduled maintenance tasks |
| transaction | `include/transaction/mvcc_cleanup.h` | Invokes MVCC version-chain garbage collection during maintenance windows |
| observability | `include/observability/metrics_collector.h`, `include/observability/audit_logger.h` | Records job execution counters and emits maintenance-event audit entries |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/maintenance/maintenance_api_handler.h`, `include/maintenance/database_maintenance_orchestrator.h` | Exposes maintenance schedule CRUD and on-demand job trigger endpoints via the HTTP API |

## Integration Points

### Critical Integration: DatabaseMaintenanceOrchestrator ↔ Server API Handler
**Files:** `include/maintenance/database_maintenance_orchestrator.h`, `include/maintenance/maintenance_api_handler.h` ↔ `src/server/`
**Contract:** `MaintenanceApiHandler` translates HTTP requests into typed `OrchestratorCommand` calls; `DatabaseMaintenanceOrchestrator` owns the authoritative schedule state and dispatches handlers. All mutations are serialised through the orchestrator — the API handler holds no independent state.
**Thread Safety:** The orchestrator uses an internal job-dispatch queue; concurrent API calls are safely serialised.
**Failure Mode:** Handler unavailability propagates as `HANDLER_NOT_FOUND`; the orchestrator rejects the schedule entry rather than enqueuing an unexecutable job.

### Critical Integration: Compaction Job ↔ Storage
**Files:** `src/maintenance/database_maintenance_orchestrator.cpp` ↔ `include/storage/compaction_manager.h`
**Contract:** The compaction maintenance handler calls `CompactionManager::triggerCompaction(db_path, options)` within the scheduled execution window; compaction is async relative to the maintenance orchestrator (fires and monitors via callback).
**Thread Safety:** `CompactionManager` is thread-safe; concurrent maintenance and organic compaction triggers are serialised internally by RocksDB.
**Failure Mode:** Compaction errors are surfaced as `CompactionFailed` in the orchestrator job result log; the maintenance schedule advances to the next interval regardless.

### Critical Integration: MVCC Cleanup ↔ Transaction Module
**Files:** `src/maintenance/database_maintenance_orchestrator.cpp` ↔ `include/transaction/mvcc_cleanup.h`
**Contract:** The MVCC-cleanup handler calls `MvccCleanup::purgeBeforeHorizon(snapshot_horizon)` to reclaim obsolete version chains; the horizon is determined by the oldest active read snapshot at dispatch time.
**Thread Safety:** `MvccCleanup` operations hold an MVCC read lock for the duration of the purge window to prevent new snapshots from racing below the horizon.
**Failure Mode:** If the transaction module cannot supply a valid horizon the cleanup is skipped for the current window and a `MVCC_HORIZON_UNAVAILABLE` warning is emitted to the maintenance log.



- Verified files:
  - src/maintenance/database_maintenance_orchestrator.cpp
  - src/maintenance/maintenance_schedule_store.cpp
  - src/maintenance/maintenance_registry.cpp
- Verified architecture claims:
  - explicit orchestration, persistence, and registry planes
  - deterministic failure boundaries for schedule and handler paths
  - module-local ownership of maintenance orchestration behavior