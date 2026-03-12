### Context

This issue implements the roadmap item 'Schedule Persistence (RocksDB)' for the maintenance domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Schedule Persistence (RocksDB)

### Goal

Deliver the scoped changes for Schedule Persistence (RocksDB) in src/maintenance/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Schedule Persistence (RocksDB)
**Priority:** High
**Target Version:** v1.1.0

Schedules are currently in-memory (`std::unordered_map<std::string, MaintenanceScheduleEntry> schedules_`). They are lost on every server restart. Operators must re-create all schedules after each deployment.

**Implementation Notes:**
- `[ ]` Add a `MaintenanceScheduleStore` class wrapping the existing `StorageEngine` API; key format: `maint_sched::{id}` (UTF-8 JSON value).
- `[ ]` In `DatabaseMaintenanceOrchestrator::start()`, call `MaintenanceScheduleStore::loadAll()` and populate `schedules_` before registering cron jobs.
- `[ ]` In `createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule` — persist the change to RocksDB inside the `schedules_mutex_` critical section (write-through).
- `[ ]` Corrupt schedule JSON on load: log `WARN` and skip that entry; all valid entries must be loaded.
- `[ ]` Add a restart-persistence integration test: create 3 schedules, restart the orchestrator, verify all 3 are present.

**Performance Targets:**
- `loadAll()` at startup: ≤ 100 ms for 10 000 stored schedules.

---

### Acceptance Criteria

- [ ] Add a `MaintenanceScheduleStore` class wrapping the existing `StorageEngine` API; key format: `maint_sched::{id}` (UTF-8 JSON value).
- [ ] In `DatabaseMaintenanceOrchestrator::start()`, call `MaintenanceScheduleStore::loadAll()` and populate `schedules_` before registering cron jobs.
- [ ] In `createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule` — persist the change to RocksDB inside the `schedules_mutex_` critical section (write-through).
- [ ] Corrupt schedule JSON on load: log `WARN` and skip that entry; all valid entries must be loaded.
- [ ] Add a restart-persistence integration test: create 3 schedules, restart the orchestrator, verify all 3 are present.
- [ ] `loadAll()` at startup: ≤ 100 ms for 10 000 stored schedules.

### Relationships

- Roadmap row: #19 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#schedule-persistence-rocksdb
- Source key: roadmap:19:maintenance:v1.1.0:schedule-persistence-rocksdb

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:19:maintenance:v1.1.0:schedule-persistence-rocksdb -->
<!-- roadmap-ref: row=19;module=maintenance;target=v1.1.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#schedule-persistence-rocksdb -->
