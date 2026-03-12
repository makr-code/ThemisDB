### Context

This issue implements the roadmap item '`schedules_mutex_` Read-Path Upgrade' for the maintenance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: `schedules_mutex_` Read-Path Upgrade

### Goal

Deliver the scoped changes for `schedules_mutex_` Read-Path Upgrade in src/maintenance/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### `schedules_mutex_` Read-Path Upgrade
**Priority:** Medium
**Target Version:** v1.2.0

`database_maintenance_orchestrator.cpp` uses `std::lock_guard<std::mutex>` (exclusive) for all read operations (`listSchedules`, `getSchedule`, `listJobs`, `getJob`). Under concurrent admin API load, all readers serialize unnecessarily.

**Implementation Notes:**
- `[ ]` Replace `std::mutex schedules_mutex_` and `std::mutex jobs_mutex_` with `std::shared_mutex`; upgrade `listSchedules`, `getSchedule`, `listJobs`, `getJob` to `std::shared_lock`.
- `[ ]` Keep all write operations (`createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule`, `pruneOldJobs`) on `std::unique_lock`.
- `[ ]` Add a TSAN-enabled test with 8 concurrent `listSchedules` threads + 1 `createSchedule` thread.

---

### Acceptance Criteria

- [ ] Replace `std::mutex schedules_mutex_` and `std::mutex jobs_mutex_` with `std::shared_mutex`; upgrade `listSchedules`, `getSchedule`, `listJobs`, `getJob` to `std::shared_lock`.
- [ ] Keep all write operations (`createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule`, `pruneOldJobs`) on `std::unique_lock`.
- [ ] Add a TSAN-enabled test with 8 concurrent `listSchedules` threads + 1 `createSchedule` thread.

### Relationships

- Roadmap row: #185 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#schedules_mutex_-read-path-upgrade
- Source key: roadmap:185:maintenance:v1.2.0:schedules-mutex-read-path-upgrade

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:185:maintenance:v1.2.0:schedules-mutex-read-path-upgrade -->
<!-- roadmap-ref: row=185;module=maintenance;target=v1.2.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#schedules_mutex_-read-path-upgrade -->
