> **Build:** `cmake --preset release && cmake --build build/release --target themisdb`

# Projects Module

**Module Path:** `src/projects/`  
**Namespace:** `themis::projects`

## Module Purpose

The Projects module implements the project management layer for ThemisDB: lifecycle state machines,
versioning and snapshot management, structural diffing and merging, template instantiation,
and real-time collaboration session management for concurrent document editing.

## Source Files

| File | Role |
|------|------|
| `collaboration_manager.cpp` | Implements `CollaborationManager` — real-time session management for concurrent project editing with permission enforcement |
| `project_diff.cpp` | Implements `ProjectDiff` / `ProjectMerge` — structural delta computation and three-way merge for project versions |
| `project_lifecycle.cpp` | Implements `ProjectLifecycle` — state-machine transitions (`DRAFT → ACTIVE → ARCHIVED → DELETED`) |
| `project_template.cpp` | Implements `ProjectTemplate` — built-in and custom template instantiation with schema initialization |
| `project_versioning.cpp` | Implements `ProjectVersioning` — snapshot creation, retrieval, and point-in-time restoration |

## Key Classes (in `namespace themis::projects`)

### `ProjectLifecycle`
**Header:** `include/projects/project_lifecycle.h`

Manages project state transitions with guard validation.

```cpp
#include "projects/project_lifecycle.h"

using namespace themis::projects;
ProjectLifecycle lifecycle(storage);
lifecycle.transition(project_id, ProjectState::ACTIVE);
```

States: `DRAFT`, `ACTIVE`, `SUSPENDED`, `ARCHIVED`, `DELETED`

### `ProjectVersioning`
**Header:** `include/projects/project_versioning.h`

Snapshot-based version control for project documents.

```cpp
#include "projects/project_versioning.h"

ProjectVersioning versioning(storage);
auto snap = versioning.createSnapshot(project_id, "v1.2", author);
versioning.restore(project_id, snap.snapshot_id);
```

### `ProjectDiff` / `ProjectMerge`
**Header:** `include/projects/project_diff.h`

Computes structural deltas (`DeltaSet`) between two project versions and performs three-way merge.

Delta types: `ADDED`, `REMOVED`, `MODIFIED`, `MOVED`

### `ProjectTemplate`
**Header:** `include/projects/project_template.h`

Instantiates projects from built-in templates (`BLANK`, `ANALYTICS`, `ML_PIPELINE`, `REPORT`) or custom YAML/JSON schemas.

### `CollaborationManager`
**Header:** `include/projects/collaboration_manager.h`

Manages concurrent editing sessions with permission control (`READ`, `WRITE`, `ADMIN`, `OWNER`).

## Additional Interfaces

| Header | Role |
|--------|------|
| `include/projects/project_audit_log.h` | `IProjectAuditLog` — append-only audit trail for project operations |
| `include/projects/project_bundle.h` | `IProjectBundleManager` — import/export project bundles |

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/projects/README.md`](../../include/projects/README.md) for the public API.
