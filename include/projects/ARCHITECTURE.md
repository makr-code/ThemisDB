<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Projects Module — Architecture Guide

## Overview

The projects module provides workspace and project management for ThemisDB: organizing tables, indexes, queries, process models, and ML models into named projects. It handles project-level permissions, versioning, snapshots, and templates. The sole current public header is `DocumentManager/document_manager.h`.

## Design Principles

- **Project as namespace** — every database object (table, index, query, model) belongs to exactly one project.
- **Snapshot-based versioning** — projects support point-in-time snapshots for reproducible environments.
- **Template instantiation** — `DocumentManager` supports project templates for rapid environment bootstrapping.
- **RBAC-gated access** — all project operations are checked against project-level role assignments.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `DocumentManager/document_manager.h` | `DocumentManager` | CRUD for project documents; object organization, versioning, snapshots, and template management |
| `project_versioning.h` | `ProjectVersioning`, `SnapshotMeta` | Immutable, content-addressed project snapshots with SHA-256 integrity verification |
| `project_diff.h` | `ProjectDiff`, `ProjectMerge`, `DeltaSet` | Structured field-level diff and three-way merge between project snapshots |
| `project_lifecycle.h` | `ProjectLifecycle`, `ProjectStateTransition` | Atomic project state machine (CREATED→ACTIVE→ARCHIVED/DELETED) with append-only audit log |
| `project_template.h` | `ProjectTemplate`, `BuiltinTemplate` | Factory for instantiating projects from 7 built-in or custom JSON templates |
| `collaboration_manager.h` | `CollaborationManager`, `Change` | RBAC sharing, optimistic object locking, real-time event callbacks, and change feed |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `storage` | All managers | Persistent project metadata storage via `RocksDBWrapper` |
| `auth` | `DocumentManager`, `CollaborationManager` | Project-level RBAC permission checks and sharing |
| `query` | `DocumentManager` | Query objects organized by project |
| `process` | `DocumentManager` | Process definitions scoped to projects |
| `plugins` | `PluginManager` | Per-project plugin activation |

## Key Design Decisions

- **Snapshots are immutable and content-addressed** — SHA-256 digest computed at creation, verified on restore.
- **DeltaSet over raw text diffs** — all diff/merge results are structured `DeltaSet` with typed field-level changes.
- **Atomic lifecycle transitions** — state change and audit log entry written in a single RocksDB write batch.
- **Thread-safe collaboration** — `CollaborationManager` uses `std::shared_mutex` with fine-grained per-operation locks.
- **Template schema validation before write** — `ProjectTemplate::instantiateFromDefinition` always validates before creating objects; failures are fully rolled back.

## Implementation

Implementation in `../../src/projects/`.
