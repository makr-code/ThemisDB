> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# Projects Module — Future Enhancements

## Scope

The Projects module (`src/projects/`) implements project lifecycle management
(`ProjectLifecycle`), snapshot versioning (`ProjectVersioning`), structural diff/merge
(`ProjectDiff`, `ProjectMerge`), template instantiation (`ProjectTemplate`), and
real-time collaboration session management (`CollaborationManager`). Enhancements focus
on test coverage, observability, REST API exposure, and collaboration transport.

---

## Design Constraints

- `ProjectLifecycle` state-machine guard must be preserved; no state transitions may bypass
  the validation logic, including from new REST endpoints.
- `IProjectAuditLog` is append-only; implementations must never mutate or delete existing entries.
- `ProjectVersioning` snapshots are immutable; restoration must create a new snapshot and not
  overwrite existing snapshot records.
- `CollaborationManager` permission checks must be enforced on every session write; no
  bypass path should be introduced for "internal" callers.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ProjectLifecycle::transition(project_id, target_state)` | REST API, admin CLI | Returns `ProjectStateTransition`; rejects invalid transitions |
| `ProjectVersioning::createSnapshot(project_id, label, author)` | REST API, CI/CD hooks | Returns `SnapshotMeta`; snapshot is immutable after creation |
| `ProjectDiff::compute(version_a, version_b)` | Collaboration UI, review API | Returns `DeltaSet` with typed `DeltaEntry` items |
| `CollaborationManager::submitChange(session_id, change)` | Real-time collaboration | Validates `Permission::WRITE`; applies OT/CRDT transform |
| `IProjectBundleManager::exportBundle(project_ids, options)` | Admin API, backup tooling | Returns bundle archive with `ProjectBundleManifest` |

---

## Planned Features

### Test Coverage for Core Paths
**Priority:** High
**Target:** 2026-Q3

Current test coverage for the projects module is not confirmed.

**Implementation Notes:**
- Unit tests for `ProjectLifecycle`: all valid transitions, all invalid-transition rejections, idempotent double-transition guard.
- Unit tests for `ProjectVersioning`: snapshot creation, list, restore round-trip; verify snapshot immutability.
- Unit tests for `ProjectDiff`: delta type coverage (`ADDED`, `REMOVED`, `MODIFIED`, `MOVED`); empty-diff case; identical versions.
- Unit tests for `ProjectMerge`: three-way merge success, conflict detection, conflict record structure.
- Unit tests for `CollaborationManager`: permission enforcement (READ cannot submit change, WRITE can, ADMIN can revoke session).

**Test Strategy:**
- In-memory storage mock for lifecycle and versioning tests.
- Concurrent goroutine/thread test for `CollaborationManager` to verify lock correctness.

---

### REST API for Project Bundle Import/Export
**Priority:** Medium
**Target:** 2026-Q3

The `IProjectBundleManager` interface is defined in `include/projects/project_bundle.h` but
no REST endpoint exposes it.

**Implementation Notes:**
- `POST /api/v1/projects/export` — accepts list of project IDs + `BundleExportOptions`; returns bundle archive
- `POST /api/v1/projects/import` — accepts bundle archive; returns `BundleImportResult` with project IDs and warnings
- Require `projects:admin` RBAC scope for both endpoints.

---

### Observability: Metrics and Audit Hooks
**Priority:** Medium
**Target:** 2026-Q4
**Status:** ✅ Implemented (2026-04-27, v1.9.0)

`ProjectMetrics` class added to `include/projects/project_metrics.h` /
`src/projects/project_metrics.cpp`:
- `projects_changes_total` counter — incremented per `CollaborationManager::notifyChange()`.
- `project_diff_calls_total` counter — incremented per `ProjectDiff::diff()` call.
- `project_diff_duration_ms_total` counter — cumulative wall-clock diff latency.
- `ProjectMetrics::getMetricsText()` emits Prometheus text v0.0.4.
- DI setters: `CollaborationManager::setMetrics()`, `ProjectDiff::setMetrics()`.
- Tests: PM-01..PM-06 in `tests/test_projects.cpp`.

**Remaining (not in scope for v1.9.0):**
- Prometheus gauges `projects_active_total` / `projects_archived_total` / `projects_deleted_total` (require lifecycle state-change hooks — Target: v2.0.0)
- Wire `IProjectAuditLog` into `ProjectLifecycle` (Target: v2.0.0)

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | > 80% new code | All lifecycle transitions, diff types, permission checks |
| Integration | Collaboration round-trip | Submit change → conflict detect → merge |
| Security | Permission enforcement | READ / WRITE / ADMIN / OWNER boundary checks |

## Performance Targets

- `ProjectDiff::compute()` for documents up to 10 MB: ≤ 50 ms
- `ProjectVersioning::createSnapshot()`: ≤ 20 ms (excluding storage I/O)

## Security / Reliability

- All state-changing operations (`transition`, `submitChange`) must be audit-logged.
- `CollaborationManager` must hold a session lock for the duration of OT transform and change append to prevent data races.
- `ProjectVersioning` snapshot IDs must be globally unique (UUID v4).

