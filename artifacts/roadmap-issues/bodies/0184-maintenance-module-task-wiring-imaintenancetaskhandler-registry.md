### Context

This issue implements the roadmap item 'Module Task Wiring: `IMaintenanceTaskHandler` Registry' for the maintenance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: Module Task Wiring: `IMaintenanceTaskHandler` Registry

### Goal

Deliver the scoped changes for Module Task Wiring: `IMaintenanceTaskHandler` Registry in src/maintenance/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Module Task Wiring: `IMaintenanceTaskHandler` Registry
**Priority:** Medium
**Target Version:** v1.2.0

`executeTask()` in `database_maintenance_orchestrator.cpp` succeeds immediately for all delegated task types (`STORAGE_COMPACTION`, `REPLICA_VALIDATION`, `MVCC_CLEANUP`, etc.) without calling any real module code. This is documented in `ROADMAP.md` as a known limitation.

**Implementation Notes:**
- `[ ]` Add `registerTaskHandler(MaintenanceTaskType, std::shared_ptr<IMaintenanceTaskHandler>)` to the orchestrator public API.
- `[ ]` `StorageModule` registers a handler for `STORAGE_COMPACTION` that calls `CompactionManager::triggerCompaction()`.
- `[ ]` `ShardingModule` registers a handler for `REPLICA_VALIDATION` that calls the consistency checker.
- `[ ]` `StorageEngine` registers a handler for `MVCC_CLEANUP` that triggers MVCC tombstone GC.
- `[ ]` For unregistered task types, `executeTask()` returns a `SKIPPED` result with a structured log message indicating no handler is registered.
- `[ ]` Add a `GET /api/v1/maintenance/task-handlers` endpoint listing registered handlers per task type (useful for diagnosing unregistered tasks).

---

### Acceptance Criteria

- [ ] Add `registerTaskHandler(MaintenanceTaskType, std::shared_ptr<IMaintenanceTaskHandler>)` to the orchestrator public API.
- [ ] `StorageModule` registers a handler for `STORAGE_COMPACTION` that calls `CompactionManager::triggerCompaction()`.
- [ ] `ShardingModule` registers a handler for `REPLICA_VALIDATION` that calls the consistency checker.
- [ ] `StorageEngine` registers a handler for `MVCC_CLEANUP` that triggers MVCC tombstone GC.
- [ ] For unregistered task types, `executeTask()` returns a `SKIPPED` result with a structured log message indicating no handler is registered.
- [ ] Add a `GET /api/v1/maintenance/task-handlers` endpoint listing registered handlers per task type (useful for diagnosing unregistered tasks).

### Relationships

- Roadmap row: #184 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#module-task-wiring-imaintenancetaskhandler-registry
- Source key: roadmap:184:maintenance:v1.2.0:module-task-wiring-imaintenancetaskhandler-registry

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:184:maintenance:v1.2.0:module-task-wiring-imaintenancetaskhandler-registry -->
<!-- roadmap-ref: row=184;module=maintenance;target=v1.2.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#module-task-wiring-imaintenancetaskhandler-registry -->
