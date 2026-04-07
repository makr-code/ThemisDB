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

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `storage` | `DocumentManager` | Persistent project metadata storage |
| `auth` | `DocumentManager` | Project-level RBAC permission checks |
| `query` | `DocumentManager` | Query objects organized by project |
| `process` | `DocumentManager` | Process definitions scoped to projects |
| `plugins` | `PluginManager` | Per-project plugin activation |

## Implementation

Implementation in `../../src/projects/`.
