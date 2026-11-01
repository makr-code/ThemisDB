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

# Architecture - Projects Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The projects module composes lifecycle state management, immutable project snapshots, structural diff/merge behavior, template-driven project initialization, and bounded collaboration/audit support into a single project-domain subsystem.

## Main Execution Planes

1. Lifecycle and versioning plane
- `ProjectLifecycle` validates transitions and writes current state and its transition record together through RocksDB.
- `ProjectVersioning` captures project document metadata, stores snapshot metadata/content, and checks its SHA-256 checksum on restore.

2. Diff/template plane
- structural delta computation and merge conflict reporting
- template validation and project bootstrap behavior

3. Collaboration/observability plane
- `CollaborationManager` stores sharing and change records, maintains in-process object locks, and synchronously invokes subscribers.
- Its change feed is bounded to 10,000 entries per manager. Metrics and an optional audit sink are injected separately; the provided audit implementation is in-memory and bounded.

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic, validation-gated state transitions |
| versioning contract | immutable snapshot semantics with explicit restore behavior |
| diff contract | explicit delta and merge conflict surfaces |
| collaboration contract | sharing/permission-record operations, object-lock ownership checks, and bounded change feed behavior |
| project API error codes | `ProjError` declares four numeric values (7700–7703); comments in that header refer to membership/audit methods not declared by the current interfaces |
| bundle API | `IProjectBundleManager` is an interface declaration; this module has no concrete bundle implementation |

## Failure Semantics

- invalid transitions fail explicitly.
- missing or invalid snapshot payloads fail deterministically.
- invalid sharing inputs and lock violations are surfaced as explicit errors.
- merge conflicts are returned explicitly for caller handling.
- the module has no general authentication/authorization middleware or module-level configuration object; callers must enforce deployment access control.

## Sourcecode Verification (Module: projects/architecture)

- Verified files:
  - src/projects/project_lifecycle.cpp
  - src/projects/project_versioning.cpp
  - src/projects/project_diff.cpp
  - src/projects/project_template.cpp
  - src/projects/collaboration_manager.cpp
  - src/projects/in_memory_project_audit_log.cpp
  - include/projects/projects_api_contract.h
  - include/projects/project_bundle.h
- Verified architecture claims:
  - lifecycle/versioning + diff/template + collaboration/observability plane split
  - explicit failure boundaries for invalid transitions, snapshots, locks, and conflicts
  - module-local ownership of project-domain behavior surfaces
  - the bundle and membership-like surfaces above are declarations/contracts, not implemented module features
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `document` | `include/projects/DocumentManager/document_manager.h` | Deprecated document manager adapter bridges the legacy projects-scoped `DocumentManager` API (`include/document/document_manager_deprecated.h`) |
