# ThemisDB Scheduler Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The scheduler module provides task registration/execution orchestration, retention workflows, distributed coordination adapters, audit/result tracking, anomaly detection, and event-trigger integration for scheduled automation in ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| task_scheduler.cpp | core task registration, execution, and scheduler runtime behavior |
| hybrid_retention_manager.cpp | retention lifecycle orchestration behavior |
| distributed_task_coordinator.cpp | distributed task coordination and leadership behavior |
| external_scheduler_adapter.cpp | integration behavior for external scheduler backends |
| task_audit_manager.cpp | audit trail querying and lifecycle behavior |
| task_result_store.cpp | persisted task result tracking behavior |
| task_anomaly_detector.cpp | anomaly detection surfaces for task runs |
| event_trigger.cpp | event-driven scheduling trigger behavior |
| task_audit_event.cpp | audit event model behavior |

## Scope

In scope:
- task registration/listing/execution and stats behavior
- retention and distributed coordination scheduler surfaces
- audit/result/trigger/anomaly observability paths

Out of scope:
- business-domain workflow ownership outside scheduler contracts
- storage/query internals owned by other modules
- global authn/authz ownership outside scheduler boundaries

## Runtime Behavior and Limits

- scheduler operations are config-bounded and explicit.
- registration/execution/listing paths are deterministic and observable.
- distributed/external adapters expose explicit outcomes.
- anomaly/audit/result paths remain diagnosable and non-silent.

## Sourcecode Verification (Module: scheduler/readme)

- Verified files:
  - src/scheduler/task_scheduler.cpp
  - src/scheduler/hybrid_retention_manager.cpp
  - src/scheduler/distributed_task_coordinator.cpp
  - src/scheduler/external_scheduler_adapter.cpp
  - src/scheduler/task_audit_manager.cpp
  - src/scheduler/task_result_store.cpp
  - src/scheduler/task_anomaly_detector.cpp
  - src/scheduler/event_trigger.cpp
  - src/scheduler/task_audit_event.cpp
- Verified behavior surfaces:
  - registration/execution/listing/stats, coordination/adapters, audit/result/anomaly
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md