> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/projects/ARCHITECTURE.md -->

# Projects Module — Public Header Architecture

**Module Path:** `include/projects/`  
**Implementation:** `../../src/projects/`  
**Canonical architecture doc:** [`../../src/projects/ARCHITECTURE.md`](../../src/projects/ARCHITECTURE.md)

---

## 1. Overview

`include/projects/` defines the **public project lifecycle, versioning, bundling, collaboration, diff tracking, templates, and audit logging API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/projects/ARCHITECTURE.md`](../../src/projects/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Project Lifecycle

| Header | Public Type | Purpose |
|--------|------------|---------|
| `project_lifecycle.h` | `ProjectLifecycle` | Project create/activate/archive lifecycle |
| `project_versioning.h` | `ProjectVersioning` | Snapshot-based project versioning |
| `project_bundle.h` | `ProjectBundle` | Project asset bundling for export/import |
| `project_diff.h` | `ProjectDiff` | Structural diff between project versions |
| `project_template.h` | `ProjectTemplate` | Reusable project scaffold templates |
### 2.2 Collaboration and Metrics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `collaboration_manager.h` | `CollaborationManager` | Real-time multi-user collaboration |
| `project_metrics.h` | `ProjectMetrics` | Project health and activity metrics |
### 2.3 Auditing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `project_audit_log.h` | `ProjectAuditLog` | Immutable project change audit log |
| `in_memory_project_audit_log.h` | `InMemoryProjectAuditLog` | In-memory audit log for testing |

---

## 3. Namespace Layout

All public types reside in the `themis::projects` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/projects/` expose the **stable public API**; internal types live in `src/projects/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
