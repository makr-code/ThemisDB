### Context

This issue implements the roadmap item 'Explicit Per-Task DAG with `depends_on`' for the maintenance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: Explicit Per-Task DAG with `depends_on`

### Goal

Deliver the scoped changes for Explicit Per-Task DAG with `depends_on` in src/maintenance/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### Explicit Per-Task DAG with `depends_on`
**Priority:** Medium
**Target Version:** v1.2.0

Task execution order is currently determined by list order in `MaintenanceScheduleEntry::tasks`. There are no explicit dependency declarations, making it impossible to express "run WAL rotation before compaction" without relying on position.

**Implementation Notes:**
- `[ ]` Add `MaintenanceTaskDependency` struct: `{ task_type: MaintenanceTaskType, depends_on: vector<MaintenanceTaskType> }`.
- `[ ]` Add `MaintenanceScheduleEntry::task_dependencies` field (optional; defaults to sequential list order).
- `[ ]` Implement topological sort of the dependency graph using `ModuleDependencyResolver` (already implemented in `src/base/module_loader.cpp`).
- `[ ]` Cycle detection: reject schedule creation / update with a cycle; return `ERR_UTIL_INVALID_ARGUMENT`.
- `[ ]` Tests: DAG ordering correctness, cycle rejection, cascading failure with `halt_on_task_failure`.

**Performance Targets:**
- Topological sort: O(V+E); V=19 max task types — negligible overhead.

---

### Acceptance Criteria

- [ ] Add `MaintenanceTaskDependency` struct: `{ task_type: MaintenanceTaskType, depends_on: vector<MaintenanceTaskType> }`.
- [ ] Add `MaintenanceScheduleEntry::task_dependencies` field (optional; defaults to sequential list order).
- [ ] Implement topological sort of the dependency graph using `ModuleDependencyResolver` (already implemented in `src/base/module_loader.cpp`).
- [ ] Cycle detection: reject schedule creation / update with a cycle; return `ERR_UTIL_INVALID_ARGUMENT`.
- [ ] Tests: DAG ordering correctness, cycle rejection, cascading failure with `halt_on_task_failure`.
- [ ] Topological sort: O(V+E); V=19 max task types — negligible overhead.

### Relationships

- Roadmap row: #183 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/maintenance/FUTURE_ENHANCEMENTS.md#explicit-per-task-dag-with-depends_on
- Source key: roadmap:183:maintenance:v1.2.0:explicit-per-task-dag-with-depends-on

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:183:maintenance:v1.2.0:explicit-per-task-dag-with-depends-on -->
<!-- roadmap-ref: row=183;module=maintenance;target=v1.2.0 -->
<!-- roadmap-detail: src/maintenance/FUTURE_ENHANCEMENTS.md#explicit-per-task-dag-with-depends_on -->
