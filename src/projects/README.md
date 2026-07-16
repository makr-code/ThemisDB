# ThemisDB Projects Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The projects module provides project lifecycle control, snapshot/version management, structural diff/merge, template-driven initialization, collaboration state handling, metrics emission, and audit logging support surfaces for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| project_lifecycle.cpp | lifecycle transitions and state persistence |
| project_versioning.cpp | immutable snapshot creation/listing/restore behavior |
| project_diff.cpp | structural diff and merge conflict detection |
| project_template.cpp | built-in/custom template instantiation |
| collaboration_manager.cpp | sharing, locking, change feed, subscriber dispatch |
| project_metrics.cpp | project metrics counters and export text generation |
| in_memory_project_audit_log.cpp | bounded thread-safe audit event storage |

## Scope

In scope:
- project state lifecycle, snapshots, and version metadata behavior
- project diff/merge and template-based bootstrap paths
- project collaboration state/permissions, metrics, and audit surfaces

Out of scope:
- transport-specific realtime collaboration protocols outside this module
- non-project domain workflows owned by other modules
- external orchestration layers beyond project module contracts

## Runtime Behavior and Limits

- lifecycle transitions are validation-gated and explicit.
- snapshot restore paths are integrity-checked before writes.
- collaboration change feeds are bounded in-memory structures.
- merge paths report conflicts explicitly and do not silently auto-resolve.

## Sourcecode Verification (Module: projects/readme)

- Verified files:
  - src/projects/project_lifecycle.cpp
  - src/projects/project_versioning.cpp
  - src/projects/project_diff.cpp
  - src/projects/project_template.cpp
  - src/projects/collaboration_manager.cpp
  - src/projects/project_metrics.cpp
  - src/projects/in_memory_project_audit_log.cpp
- Verified behavior surfaces:
  - lifecycle, snapshot/versioning, diff/merge, template, collaboration, metrics, audit
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md