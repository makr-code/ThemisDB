---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Projects Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The projects module provides project lifecycle transitions, document-metadata snapshots, structural diff/merge, template instantiation, collaboration sharing/locking/change feeds, metrics, and an in-memory audit-log implementation.

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
| `include/projects/projects_api_contract.h` | error-code enum (`ProjError`, values 7700–7703); its comments refer to membership/audit methods not declared by the current public API |
| `include/projects/project_bundle.h` | `IProjectBundleManager` interface only; no bundle implementation is provided by this module |

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
- snapshots capture project document metadata, not document blobs.
- collaboration callbacks run synchronously from `notifyChange()`; callbacks can delay later notifications.
- collaboration change history is bounded to 10,000 entries per manager; `InMemoryProjectAuditLog` defaults to 100,000 entries and evicts the oldest 10% when full.
- the module exposes no general authentication, deployment configuration, or authorization middleware; sharing/locking APIs are not a substitute for application-level access control.

## Usage

The public API examples and input/failure details are in
[`include/projects/README.md`](../../include/projects/README.md). Callers supply
the storage handle to the module classes; the project module has no separate
deployment configuration object.

## Installation, Build, and Tests

The root `tests/CMakeLists.txt` registers `ProjectsModuleTests` for
`tests/projects/test_projects.cpp`. The focused tests in `tests/projects/` are
registered from `tests/projects/CMakeLists.txt`; enable `THEMIS_BUILD_TESTS`
when configuring, then run `ctest --test-dir <build-dir> -L projects
--output-on-failure`.

The release preset `linux-release` sets `THEMIS_BUILD_TESTS=OFF` and
`THEMIS_BUILD_BENCHMARKS=OFF`; it builds the product but does not configure
these module tests or benchmarks.

## Sourcecode Verification (Module: projects/readme)

- Verified files:
  - src/projects/project_lifecycle.cpp
  - src/projects/project_versioning.cpp
  - src/projects/project_diff.cpp
  - src/projects/project_template.cpp
  - src/projects/collaboration_manager.cpp
  - src/projects/project_metrics.cpp
  - src/projects/in_memory_project_audit_log.cpp
  - include/projects/projects_api_contract.h
  - include/projects/project_bundle.h
- Verified behavior surfaces:
  - lifecycle, snapshot/versioning, diff/merge, template, collaboration, metrics, audit
- Boundary:
  - bundle operations are declarations only; `projects_api_contract.h` declares error values but its membership/audit method descriptions do not match the current interfaces.
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md