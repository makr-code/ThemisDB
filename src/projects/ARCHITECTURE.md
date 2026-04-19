> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Projects Module — Architecture Guide

**Version:** 1.0
**Module Path:** `src/projects/`
**Namespace:** `themis::projects`

---

## 1. Overview

The Projects module provides ThemisDB's project management infrastructure: lifecycle state
machines, snapshot-based versioning, structural diff/merge, template instantiation, and
real-time collaboration session management for multi-user concurrent editing.

---

## 2. Design Principles

- **State-Machine Lifecycle** — `ProjectLifecycle` guards all state transitions; invalid
  transitions return an error rather than silently mutating state.
- **Immutable Snapshots** — `ProjectVersioning` stores snapshots as immutable records;
  restoration creates a new snapshot rather than overwriting history.
- **Delta-Based Diffing** — `ProjectDiff` computes structural deltas against two versions;
  `ProjectMerge` performs three-way merge for collaborative editing.
- **Template-Driven Initialization** — `ProjectTemplate` provides built-in and custom schemas
  so new projects start in a valid, schema-consistent state.
- **Permission-Gated Collaboration** — `CollaborationManager` enforces `READ / WRITE / ADMIN / OWNER`
  permissions on all session operations.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Class | Role |
|------|-------|------|
| `project_lifecycle.cpp` | `ProjectLifecycle` | State-machine for project lifecycle transitions |
| `project_versioning.cpp` | `ProjectVersioning` | Snapshot creation, retrieval, and restoration |
| `project_diff.cpp` | `ProjectDiff`, `ProjectMerge` | Structural diff and three-way merge |
| `project_template.cpp` | `ProjectTemplate` | Template instantiation and schema initialization |
| `collaboration_manager.cpp` | `CollaborationManager` | Concurrent editing sessions with permission enforcement |

### 3.2 Class Hierarchy

```
namespace themis::projects

ProjectLifecycle
  └─ ProjectState enum { DRAFT, ACTIVE, SUSPENDED, ARCHIVED, DELETED }
  └─ ProjectStateTransition struct { from, to, actor, timestamp_ms }

ProjectVersioning
  └─ SnapshotMeta struct { snapshot_id, project_id, label, author, timestamp_ms, ... }

ProjectDiff
  └─ DeltaType enum { ADDED, REMOVED, MODIFIED, MOVED }
  └─ DeltaEntry struct { type, path, old_value, new_value }
  └─ DeltaSet struct { entries, base_version, target_version }
ProjectMerge
  └─ MergeResult struct { merged_document, conflicts, success }

ProjectTemplate
  └─ BuiltinTemplate enum { BLANK, ANALYTICS, ML_PIPELINE, REPORT, ... }
  └─ TemplateOptions struct { name, description, tags, initial_schema }
  └─ TemplateInstantiationResult struct { project_id, snapshot_id, warnings }

CollaborationManager
  └─ Permission enum { READ, WRITE, ADMIN, OWNER }
  └─ User struct { user_id, display_name, permissions }
  └─ Change struct { change_id, user_id, path, delta, timestamp_ms, ... }

IProjectAuditLog (interface, project_audit_log.h)
  └─ ProjectAuditAction enum { CREATED, UPDATED, DELETED, STATE_CHANGED, ... }
  └─ ProjectAuditEntry struct { action, project_id, actor, timestamp_ms, details }

IProjectBundleManager (interface, project_bundle.h)
  └─ ProjectBundleManifest struct { bundle_id, version, projects, created_at }
  └─ BundleExportOptions / BundleImportResult
```

---

## 4. Data Flow

### 4.1 Project Lifecycle Transition

```
Caller: lifecycle.transition(project_id, ProjectState::ACTIVE)
    │
    ├─ Validate: current state allows transition to ACTIVE?
    ├─ Apply transition: persist new state with actor + timestamp
    ├─ Emit audit entry: ProjectAuditAction::STATE_CHANGED
    └─ Return ProjectStateTransition record
```

### 4.2 Collaborative Edit

```
User A: collaboration_manager.submitChange(session_id, change_a)
    │
    ├─ Permission check: user has WRITE access?
    ├─ OT/CRDT transform against concurrent changes from User B
    ├─ Append Change to session log
    └─ Broadcast delta to other session participants
```

---

## 5. Integration Points

| Direction | Module | Interface |
|-----------|--------|-----------|
| **Uses** | `storage` | Document persistence and key-value storage |
| **Uses** | `utils` | `AuditLogger` for project audit trail |
| **Provides to** | `api` | REST endpoints for project CRUD and collaboration |

---

## 6. Known Limitations

- Real-time collaboration transport is not implemented in this module; session state management is provided, but WebSocket/SSE transport is handled by the server module.
- Merge conflict resolution in `ProjectMerge` returns conflicts for the caller to resolve; no automatic resolution is applied.

