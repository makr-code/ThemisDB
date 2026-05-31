# ThemisDB Maintenance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The maintenance module orchestrates recurring and on-demand database maintenance operations, including schedule management, execution coordination, persistence of schedules, health aggregation, and task-handler dispatch.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| database_maintenance_orchestrator.cpp | central schedule and job orchestration |
| maintenance_schedule_store.cpp | schedule persistence and recovery |
| maintenance_registry.cpp | default maintenance setup and schedule bundles |

## Scope

In scope:
- maintenance schedule lifecycle and execution boundaries
- task orchestration and handler dispatch control
- persistence and reload behavior for maintenance schedule state

Out of scope:
- full distributed lock backend ownership beyond injected lock interfaces
- module-external task execution semantics outside maintenance dispatch boundaries
- scheduler internals beyond maintenance integration points

## Runtime Behavior and Limits

- behavior depends on configured schedules, task handlers, and execution policies.
- unsupported or unavailable task handlers degrade deterministically with explicit outcomes.
- schedule execution behavior is bounded by orchestrator constraints and persistence state.

## Sourcecode Verification (Module: maintenance/readme)

- Verified files:
  - src/maintenance/database_maintenance_orchestrator.cpp
  - src/maintenance/maintenance_schedule_store.cpp
  - src/maintenance/maintenance_registry.cpp
- Verified behavior surfaces:
  - schedule orchestration, persistence, and registry setup paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical module state remains in CHANGELOG.md