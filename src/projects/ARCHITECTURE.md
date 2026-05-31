# Architecture - Projects Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The projects module composes lifecycle state management, immutable project snapshots, structural diff/merge behavior, template-driven project initialization, and bounded collaboration/audit support into a single project-domain subsystem.

## Main Execution Planes

1. Lifecycle and versioning plane
- state transition validation and persistence
- snapshot creation/listing/restore and checksum/integrity behavior

2. Diff/template plane
- structural delta computation and merge conflict reporting
- template validation and project bootstrap behavior

3. Collaboration/observability plane
- permission-checked sharing and locking behavior
- bounded change feed, metrics, and in-memory audit trails

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic, validation-gated state transitions |
| versioning contract | immutable snapshot semantics with explicit restore behavior |
| diff contract | explicit delta and merge conflict surfaces |
| collaboration contract | permission-checked operations and bounded change feed behavior |

## Failure Semantics

- invalid transitions fail explicitly.
- missing or invalid snapshot payloads fail deterministically.
- permission and lock violations are surfaced as explicit errors.
- merge conflicts are returned explicitly for caller handling.

## Sourcecode Verification (Module: projects/architecture)

- Verified files:
  - src/projects/project_lifecycle.cpp
  - src/projects/project_versioning.cpp
  - src/projects/project_diff.cpp
  - src/projects/project_template.cpp
  - src/projects/collaboration_manager.cpp
  - src/projects/in_memory_project_audit_log.cpp
- Verified architecture claims:
  - lifecycle/versioning + diff/template + collaboration/observability plane split
  - explicit failure boundaries for invalid transitions, snapshots, locks, and conflicts
  - module-local ownership of project-domain behavior surfaces