---
Author: "ThemisDB Team"
Created: "2026-05-13"
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
>
> This preset disables tests and benchmarks. Configure with `THEMIS_BUILD_TESTS=ON`
> to build and run the test targets described below.

# Projects Module - Public Header Documentation

## Module Purpose

`include/projects/` exposes the public API for project lifecycle management,
immutable snapshots, structural diff/merge, template-based project bootstrap,
collaboration change feeds, metrics, and audit/bundle interfaces.

## Public Header Surface (Entry Points)

| Header | Main API surface | Notes |
|---|---|---|
| `collaboration_manager.h` | `CollaborationManager`, `Permission`, `Change` | Sharing, object locks, change feed, subscriber callbacks |
| `project_lifecycle.h` | `ProjectLifecycle`, `ProjectState` | Guarded state transitions + append-only lifecycle trail |
| `project_versioning.h` | `ProjectVersioning`, `SnapshotMeta` | Immutable snapshots with SHA-256 integrity checks |
| `project_diff.h` | `ProjectDiff`, `ProjectMerge`, `DeltaSet` | Structured JSON diff and three-way merge conflict reporting |
| `project_template.h` | `ProjectTemplate`, `TemplateOptions` | Built-in/custom template instantiation with validation + rollback |
| `project_metrics.h` | `ProjectMetrics` | Thread-safe Prometheus counters for change + diff activity |
| `project_audit_log.h` | `IProjectAuditLog`, `ProjectAuditEntry`, `AuditQueryOptions` | Append-only audit interface for project actions |
| `in_memory_project_audit_log.h` | `InMemoryProjectAuditLog` | Bounded in-memory `IProjectAuditLog` implementation |
| `project_bundle.h` | `IProjectBundleManager`, `BundleExportOptions` | Interface declaration only; no concrete bundle implementation is included in this module |
| `projects_api_contract.h` | `ProjError` | Error enum values 7700–7703; comments mention membership and audit methods not declared by the current public API |

Related dependency contract:

- [`DocumentManager/document_manager.h`](./DocumentManager/document_manager.h)

## Configuration Options

### Template instantiation (`project_template.h`)

- `TemplateOptions::project_name` (required)
- `TemplateOptions::description` (optional)
- `TemplateOptions::include_sample_data` (default: `false`)
- `TemplateOptions::extra_config` (default: empty JSON object)

### Bundle export/import (`project_bundle.h`)

- `BundleExportOptions` declares `include_data`, `include_schema`, and
  `include_indexes` (default: `true`), `include_permissions` (default:
  `false`), `compression_level` (default: `"fast"`), and `encryption_key`.
- These fields are interface declarations; this module does not implement
  ZIP import/export, encryption, or compression.

### Audit queries (`project_audit_log.h`)

- `AuditQueryOptions::project_id` (required selector)
- Optional filters: `action_filter`, `actor_id_filter`, time window
- Pagination/sort: `limit`, `offset`, `sort_direction`

## Runtime Behavior, Failure Modes, and Limits

### Lifecycle (`ProjectLifecycle`)

- Allowed transitions: `CREATED -> ACTIVE`, `ACTIVE -> ARCHIVED`, `ACTIVE -> DELETED`, `ARCHIVED -> ACTIVE`, `ARCHIVED -> DELETED`
- `DELETED` is terminal
- Typical failures include prefixed status messages for an empty project ID,
  an unknown lifecycle, and a disallowed state transition (for example
  `applyTransition: invalid transition from ...`).

### Versioning (`ProjectVersioning`)

- Snapshot IDs are prefixed as `snap:<uuid>`
- snapshots contain document metadata, not document blobs; `restoreSnapshot()`
  verifies SHA-256 before writing
- Typical failures include `restoreSnapshot: snapshot not found: <id>`,
  missing snapshot content, and checksum mismatch errors.

### Diff/Merge (`ProjectDiff`, `ProjectMerge`)

- Diffs are structured field-level deltas (`ADDED`, `REMOVED`, `MODIFIED`)
- Merge reports unresolved conflicts in `MergeResult::conflicts`
- No automatic conflict policy is applied

### Collaboration (`CollaborationManager`)

- `shareProject()` rejects invalid project/user identifiers
- `lockObject()` rejects empty locker IDs and concurrent lock collisions
- `unlockObject()` requires lock ownership
- callbacks registered by `subscribe()` run synchronously during
  `notifyChange()`; slow callbacks delay later notifications
- in-memory change feed is bounded to the latest **10,000** events per instance
- sharing/locking do not provide general authentication or enforce application
  authorization outside these operations

### Audit log implementation (`InMemoryProjectAuditLog`)

- Thread-safe append-only sink
- Default capacity: **100,000** entries (`kDefaultMaxEntries`)
- On overflow, evicts the oldest 10%

## Usage Snippets

### 1) Create and restore a snapshot

```cpp
#include "projects/project_versioning.h"

using namespace themis::projects;

ProjectVersioning pv(storage);
auto snap_or_err = pv.createSnapshot("project-1", "before-migration");
if (std::holds_alternative<SnapshotId>(snap_or_err)) {
    const auto sid = std::get<SnapshotId>(snap_or_err);
    const auto restore = pv.restoreSnapshot(sid, "project-1");
}
```

### 2) Instantiate a built-in template

```cpp
#include "projects/project_template.h"

TemplateOptions options;
options.project_name = "analytics-demo";
options.include_sample_data = true;

ProjectTemplate pt(storage);
auto created = pt.instantiate(BuiltinTemplate::ANALYTICS, options);
```

### 3) Subscribe to collaboration changes

```cpp
#include "projects/collaboration_manager.h"

CollaborationManager cm(storage);
cm.subscribe([](const Change& c) {
    // react to project change
});
```

## Installation

This module is built as part of ThemisDB. The `linux-release` preset disables
tests and benchmarks. With a build directory configured using
`THEMIS_BUILD_TESTS=ON`, run the module tests with:

```bash
ctest --test-dir <build-dir> -L projects --output-on-failure
```

The root CMake configuration registers `ProjectsModuleTests` and focused tests
from `tests/projects/`; see those CMake files for target details.

## Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| `shareProject: user id must not be empty` | One or more `User` entries have empty `id` | Validate request payload before `shareProject()` |
| `Invalid transition ...` | Illegal lifecycle edge (for example `CREATED -> ARCHIVED`) | Call `activate()` before `archive()` |
| `Snapshot checksum mismatch` | Snapshot content changed/corrupt | Recreate snapshot and verify storage integrity |
| Missing older collaboration events | In-memory feed exceeded 10,000 entries | Consume feed continuously or persist externally |

## Related Documentation and Roadmaps

- Implementation overview: [`../../src/projects/README.md`](../../src/projects/README.md)
- Architecture: [`../../src/projects/ARCHITECTURE.md`](../../src/projects/ARCHITECTURE.md)
- Security: [`../../src/projects/SECURITY.md`](../../src/projects/SECURITY.md)
- Performance targets: [`../../src/projects/PERFORMANCE_EXPECTATIONS.md`](../../src/projects/PERFORMANCE_EXPECTATIONS.md)
- Module roadmap: [`../../src/projects/ROADMAP.md`](../../src/projects/ROADMAP.md)
- Future enhancements: [`../../src/projects/FUTURE_ENHANCEMENTS.md`](../../src/projects/FUTURE_ENHANCEMENTS.md)
- Primary source maps: [`../../docs/en/projects/PRIMARY_SOURCES.md`](../../docs/en/projects/PRIMARY_SOURCES.md), [`../../docs/de/projects/PRIMARY_SOURCES.md`](../../docs/de/projects/PRIMARY_SOURCES.md)

---
