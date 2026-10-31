---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Failover Module
<!-- Wave Context: Wave A (Runtime Reliability Q3-Q4 2026) — Detection Accuracy + Promotion Timing + Split-Brain Prevention -->

## Module Purpose

Automatic failover orchestration and disaster-recovery plan coordination for ThemisDB. The module provides bounded queueing, health/failure tracking, quorum and configured fencing gates, replica-promotion requests, ordered recovery steps, and in-process results/events.

**Implementation boundary:** orchestration results do not prove every implied external action. Spare activation emits a failure event when a spare manager is configured (its return value is ignored by the main path); metadata update only logs; built-in DR restore and traffic-shift handlers do not call storage or traffic adapters. See [IMPLEMENTATION.md](IMPLEMENTATION.md).

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| auto_failover_manager.cpp | failover monitoring, queueing, and worker orchestration |
| disaster_recovery_manager.cpp | disaster recovery plan validation and step execution |
| quorum_log.cpp | optional local quorum decision log |
| topology_snapshot.cpp | deterministic topology snapshots and change comparison |
| include/failover/auto_failover_manager.h | failover manager API/contracts |
| include/failover/disaster_recovery_manager.h | disaster recovery API/contracts |
| include/failover/failover_api_contract.h | shared error, epoch, role, and retry contract helpers |
| include/failover/quorum_log.h | optional local quorum log declarations |
| include/failover/topology_snapshot.h | topology snapshot declarations |

## Operations

- [RUNBOOK.md](RUNBOOK.md) — incident diagnosis and safe recovery
- [OPERATIONS.md](OPERATIONS.md) — runtime signals and operating safeguards
- [API.md](API.md) — public interfaces and caller behavior
- [IMPLEMENTATION.md](IMPLEMENTATION.md) — execution paths and current limits
- [MODULE_GUIDE.md](MODULE_GUIDE.md) — code map and focused tests
- [RESEARCH.md](RESEARCH.md) · [MODELING.md](MODELING.md) · [EMBEDDING.md](EMBEDDING.md) — method, decision, and embedding scope
- [TRUST.md](TRUST.md) — trust boundaries and controls

## Scope

In scope:
- automatic failover state machine and task queue behavior
- disaster recovery plan validation and ordered step coordination
- failover/recovery statistics and lifecycle event surfaces

Out of scope:
- external replication/fencing manager implementations
- traffic management ownership outside failover contracts
- unrelated cluster management orchestration

## Runtime Behavior and Limits

- failover behavior depends on configured thresholds, queue limits, and external manager availability.
- disaster recovery validates required plan strings before executing its ordered steps; current built-in snapshot validation checks only that an ID is present.
- the request queue is bounded, but a queue limit does not bound dependency call duration or `stop()` join time. `stop()` calls `join()` directly; it has no enforced timed join.
- `DisasterRecoveryManager` does not roll back completed steps; built-in restore and traffic-shift handlers do not perform those external operations.

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