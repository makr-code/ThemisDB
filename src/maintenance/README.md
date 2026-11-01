# ThemisDB Maintenance Module

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

<!-- Status: runtime coordinator; validated against current source 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PRODUCTION_REQUIREMENTS.md -->
<!-- Historical planning context: Wave A (Runtime Reliability Q3-Q4 2026); not a claim that the module implements crash recovery for task side effects. -->

## Module Purpose

The maintenance module coordinates schedule lifecycle, task dispatch, optional schedule persistence/reload, job state, and health reporting. It does not itself implement compaction, MVCC cleanup, or transactional recovery of task side effects; those operations are delegated to selected index-maintenance paths or registered task handlers.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| `database_maintenance_orchestrator.cpp` | Schedule/job orchestration, dispatch, in-memory job history and diagnostics |
| `maintenance_schedule_store.cpp` | Optional JSON schedule persistence and partial reload of valid records |
| `maintenance_registry.cpp` | Creates four disabled default schedules and optionally registers the index-maintenance health probe |

These sources are included in the modular build when `THEMIS_ENABLE_HTTP_SERVER` is enabled. The public schedule and task declarations are in `include/maintenance/`.

## Operations

- [RUNBOOK.md](RUNBOOK.md) — job and schedule incident response
- [OPERATIONS.md](OPERATIONS.md) — health, metrics, and safe controls

## Scope

In scope:
- maintenance schedule lifecycle and execution boundaries
- task orchestration and handler dispatch control
- persistence and reload behavior for maintenance schedule state

Out of scope:
- full distributed lock backend ownership beyond injected lock interfaces
- module-external task execution semantics outside maintenance dispatch boundaries
- scheduler internals beyond maintenance integration points

## Runtime Behavior and Limits

- `start()` loads schedules when storage is configured; corrupt JSON records are skipped while valid records are retained. A prefix-scan error is logged and startup continues with the already-held in-memory schedules.
- Schedule writes are persisted before the corresponding in-memory mutation is published. With no storage engine, schedule definitions are process-local.
- A missing delegated handler produces a task-level skip; selected index tasks fail if `IndexMaintenanceManager` is unavailable. A skipped task does not necessarily make the aggregate job fail.
- `triggerNow()` does not require the schedule's `enabled` flag and bypasses cron timing; `force=true` bypasses only the UTC window check.
- Cancellation and later task failures do not undo already-completed task effects.

`include/maintenance/maintenance_api_contract.h` contains compatibility helper
types used by focused tests and synthetic benchmarks; it is not the runtime
contract. The orchestrator uses `MaintenanceScheduleEntry`,
`MaintenanceHealthReport`, and repository `Result<T>`/`Error` values. In
particular, the helper error taxonomy and fail-closed predicate do not control
runtime dispatch.

`max_schedule_changes_per_interval` is enforced by create, update, patch, and
delete operations. Zero disables it; a non-zero value caps validated mutation
attempts per schedule in a fixed 60-second in-memory window. Existing-schedule
attempts consume a slot before persistence, including when a later storage write
fails. A failed create discards its uncommitted counter; successful deletion
clears the schedule's counter.
Updates, patches, and deletes use the existing schedule's limit; creates use the
submitted entry's limit. The counter resets after process restart.

## Sourcecode Verification (Module: maintenance/readme)

- Verified files:
  - src/maintenance/database_maintenance_orchestrator.cpp
  - src/maintenance/maintenance_schedule_store.cpp
  - src/maintenance/maintenance_registry.cpp
- Verified behavior surfaces:
  - schedule orchestration, persistence, and registry setup paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical module state remains in CHANGELOG.md