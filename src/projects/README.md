> **Build:** `cmake --preset release && cmake --build build/release --target themisdb`

# Projects Module

**Module Path:** `src/projects/`
**Namespace:** `themis::projects`

## Module Purpose

`src/projects/` contains the production implementations behind the public
`include/projects/*` contracts: lifecycle transitions, immutable snapshot
versioning, JSON diff/merge, template instantiation, collaboration change feed,
metrics emission, and in-memory audit logging.

## Main Components (`src/projects`)

| Source file | Implemented type(s) | Runtime responsibility |
|---|---|---|
| `project_lifecycle.cpp` | `ProjectLifecycle` | Persists state transitions and lifecycle audit entries |
| `project_versioning.cpp` | `ProjectVersioning` | Creates/lists/restores/verifies snapshots with checksum validation |
| `project_diff.cpp` | `ProjectDiff`, `ProjectMerge` | Computes field-level deltas and detects merge conflicts |
| `project_template.cpp` | `ProjectTemplate` | Validates template definitions and creates project object records |
| `collaboration_manager.cpp` | `CollaborationManager` | Project sharing, object locking, in-memory change feed, subscriber dispatch |
| `project_metrics.cpp` | `ProjectMetrics` | Exports Prometheus text counters for collaboration/diff activity |
| `in_memory_project_audit_log.cpp` | `InMemoryProjectAuditLog` | Thread-safe bounded append-only audit sink |

## Public Entry Points (`include/projects`)

- [`../../include/projects/project_lifecycle.h`](../../include/projects/project_lifecycle.h)
- [`../../include/projects/project_versioning.h`](../../include/projects/project_versioning.h)
- [`../../include/projects/project_diff.h`](../../include/projects/project_diff.h)
- [`../../include/projects/project_template.h`](../../include/projects/project_template.h)
- [`../../include/projects/collaboration_manager.h`](../../include/projects/collaboration_manager.h)
- [`../../include/projects/project_metrics.h`](../../include/projects/project_metrics.h)
- [`../../include/projects/project_audit_log.h`](../../include/projects/project_audit_log.h)
- [`../../include/projects/in_memory_project_audit_log.h`](../../include/projects/in_memory_project_audit_log.h)
- [`../../include/projects/project_bundle.h`](../../include/projects/project_bundle.h)

See header-focused documentation: [`../../include/projects/README.md`](../../include/projects/README.md).

## Runtime Behavior and Data Flow

### Lifecycle

- `initProject()` writes the initial `CREATED` state
- `activate()`, `archive()`, `deleteProject()` enforce guarded transitions
- State is stored under `lifecycle:<project_id>`
- Audit entries are appended under `lifecycle_log:<project_id>:<ts_ns>`

### Versioning

- `createSnapshot()` gathers document metadata keys `doc_proj:<project_id>:*`
- Snapshot metadata is written under `snap:<uuid>`
- Snapshot payload is written under `snap_data:<uuid>`
- `restoreSnapshot()` verifies SHA-256 checksum before writes

### Collaboration

- Sharing permissions are stored under `collab_share:<project_id>:<user_id>`
- Changes are persisted under `collab_change:<project_id>:<ts_ns>`
- `notifyChange()` updates optional audit and metrics sinks when configured
- In-memory change buffer is bounded to 10,000 entries per instance

## Configuration Surface (effective options)

- `TemplateOptions` (project name, description, sample data, extra JSON config)
- `BundleExportOptions` / `BundleImportResult` (bundle I/O contract)
- `AuditQueryOptions` (project/action/actor/time filtering + pagination)
- DI hooks:
  - `CollaborationManager::setAuditLog()`
  - `CollaborationManager::setMetrics()`
  - `ProjectDiff::setMetrics()`

## Error Cases and Limits

| Component | Important failure paths |
|---|---|
| `ProjectLifecycle` | empty project ID, lifecycle missing, invalid transition |
| `ProjectVersioning` | missing snapshot metadata/content, JSON parse failure, checksum mismatch |
| `ProjectTemplate` | invalid template schema, missing `options.project_name`, object creation persistence failure with rollback |
| `CollaborationManager` | empty sharing input, lock ownership mismatch, lock contention |

Known implementation limits:

- `CollaborationManager` change log keeps only latest 10,000 events
- `InMemoryProjectAuditLog` is memory-bounded and evicts oldest 10% on overflow
- `ProjectMerge` returns conflicts but does not auto-resolve them

## Usage Snippets

### Lifecycle + snapshot flow

```cpp
#include "projects/project_lifecycle.h"
#include "projects/project_versioning.h"

ProjectLifecycle lifecycle(storage);
ProjectVersioning versioning(storage);

lifecycle.initProject("proj-42", "alice");
lifecycle.activate("proj-42", "alice");
auto sid_or_err = versioning.createSnapshot("proj-42", "baseline");
```

### Template + collaboration hooks

```cpp
#include "projects/project_template.h"
#include "projects/collaboration_manager.h"
#include "projects/project_metrics.h"

ProjectTemplate templates(storage);
CollaborationManager collaboration(storage);
auto metrics = std::make_shared<ProjectMetrics>();

collaboration.setMetrics(metrics);
TemplateOptions opts{.project_name = "web-app", .include_sample_data = true};
auto result = templates.instantiate(BuiltinTemplate::WEB_APPLICATION, opts);
```

## Troubleshooting

| Problem | Check |
|---|---|
| Transition request fails | Verify current state and allowed transition path |
| Restore fails with checksum mismatch | Validate snapshot storage integrity and avoid manual payload edits |
| Collaboration callbacks appear delayed | Ensure callbacks are non-blocking; `notifyChange()` invokes subscribers synchronously |
| Missing old audit entries in in-memory sink | Increase capacity or switch to persistent audit backend |

## Installation

Built with the standard ThemisDB build flow:

```bash
cmake --preset release
cmake --build build/release --target themisdb
```

## Related Docs

- Architecture: [`./ARCHITECTURE.md`](./ARCHITECTURE.md)
- Security notes: [`./SECURITY.md`](./SECURITY.md)
- Performance targets: [`./PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
- Module roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Global roadmap: [`../../ROADMAP.md`](../../ROADMAP.md)
- Global future enhancements: [`../../FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md)
