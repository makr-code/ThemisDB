# ThemisDB Failover Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The failover module provides automatic failover orchestration and disaster recovery execution surfaces for ThemisDB, including failover queueing, recovery workflow execution, and operational state/telemetry handling.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| auto_failover_manager.cpp | failover monitoring, queueing, and worker orchestration |
| disaster_recovery_manager.cpp | disaster recovery plan validation and step execution |
| include/failover/auto_failover_manager.h | failover manager API/contracts |
| include/failover/disaster_recovery_manager.h | disaster recovery API/contracts |

## Scope

In scope:
- automatic failover state machine and task queue behavior
- disaster recovery plan validation and ordered execution
- failover/recovery statistics and lifecycle event surfaces

Out of scope:
- external replication/fencing manager implementations
- traffic management ownership outside failover contracts
- unrelated cluster management orchestration

## Runtime Behavior and Limits

- failover behavior depends on configured thresholds, queue limits, and external manager availability.
- disaster recovery enforces plan validation before non-dry-run state mutation.
- monitoring and worker loops are concurrency-sensitive and bounded by configured queue/retry settings.

## Sourcecode Verification (Module: failover/readme)

- Verified files:
  - src/failover/auto_failover_manager.cpp
  - src/failover/disaster_recovery_manager.cpp
  - include/failover/auto_failover_manager.h
  - include/failover/disaster_recovery_manager.h
- Verified behavior surfaces:
  - failover queue/worker and lifecycle state transitions
  - DR plan validation and step-ordered execution flow
  - runtime telemetry/event handling for retry and queue pressure
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md