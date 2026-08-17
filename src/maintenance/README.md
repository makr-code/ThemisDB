# ThemisDB Maintenance Module

<!-- Status: Production Ready (Wave A Reliability) | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PRODUCTION_REQUIREMENTS.md -->
<!-- Wave Context: Wave A (Runtime Reliability Q3-Q4 2026) — Maintenance Determinism + Crash Consistency + Concurrent Operation Safety -->

## Module Purpose

Production-capable maintenance workflows, compaction, cleanup, and resource management for ThemisDB with emphasis on safety under concurrent operations and crash consistency. **Batch 5 enhancement focus: Maintenance operation safety, compaction correctness, crash consistency recovery, resource cleanup reliability**.

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