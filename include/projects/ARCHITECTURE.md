---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
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
| `project_bundle.h` | `IProjectBundleManager` | Abstract import/export contract; no concrete implementation is included here |
| `project_diff.h` | `ProjectDiff` | Structural diff between project versions |
| `project_template.h` | `ProjectTemplate` | Reusable project scaffold templates |
### 2.2 Collaboration and Metrics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `collaboration_manager.h` | `CollaborationManager` | Project sharing, object locks, and in-process change notifications |
| `project_metrics.h` | `ProjectMetrics` | Project health and activity metrics |
### 2.3 Auditing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `project_audit_log.h` | `IProjectAuditLog` | Audit record/query/count/purge interface |
| `in_memory_project_audit_log.h` | `InMemoryProjectAuditLog` | Bounded in-memory audit-log implementation |
| `projects_api_contract.h` | `ProjError` | Four error-code constants (7700–7703); comments about membership/audit methods do not match the declared API |

---

## 3. Namespace Layout

Public types reside in `themis::projects`.

---

## 4. Contract Notes

- Headers in `include/projects/` declare the public API; implementation behavior is defined by the matching source files.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- `linux-release` sets `THEMIS_BUILD_TESTS=OFF` and `THEMIS_BUILD_BENCHMARKS=OFF`; enable tests explicitly to run module tests.
