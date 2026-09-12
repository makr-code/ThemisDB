# Architecture - Failover Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The failover module composes automatic failover orchestration and disaster recovery execution into a bounded runtime contract for recovery-sensitive operations in ThemisDB.

## Main Execution Planes

1. Monitoring and queue plane
- health/failure observation and failover request queueing
- worker-driven failover task processing with bounded queue semantics

2. Recovery execution plane
- disaster recovery plan validation and step sequencing
- dry-run and non-dry-run execution boundaries

3. State and telemetry plane
- lifecycle events, retry telemetry, and queue-pressure counters
- result snapshots and state transition observability

## Core Contracts

| Contract | Behavior |
|---|---|
| failover manager contract | deterministic queue/worker lifecycle and failover semantics |
| recovery manager contract | explicit plan validation and ordered DR pipeline execution |
| telemetry contract | bounded retry/queue-pressure observability behavior |

## Failure Semantics

- invalid DR plans fail before mutating operational state.
- unavailable external managers produce explicit recovery failures where required.
- queue-limit and lifecycle precondition violations fail with explicit non-silent results.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| observability | `include/observability/metrics_collector.h`, `include/observability/tracer.h` | Failover event counters, retry telemetry, and distributed trace spans across recovery steps |
| storage | `include/storage/` (snapshot, replica-state interfaces) | Reads replica state and storage health signals to drive failover decisions |
| sharding | `include/sharding/` (health_check interfaces) | Shard-level health checks feed the failure-detection plane used by `AutoFailoverManager` |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/failover/auto_failover_manager.h`, `include/failover/disaster_recovery_manager.h` | Server control plane triggers and monitors failover/DR operations; exposes failover status via admin API |

## Integration Points

### Critical Integration: AutoFailoverManager ↔ Server Control Plane
**Files:** `include/failover/auto_failover_manager.h` ↔ `src/server/`
**Contract:** `AutoFailoverManager::enqueue(failover_request)` places a typed request into the bounded work queue; the server's admin API calls this surface and polls `getStatus()` for progress. The `canTransition()` state-machine gate (Wave A, completed 2026-07-29) must return `true` before any promotion is executed.
**Thread Safety:** Queue operations are mutex-protected; worker threads drain the queue independently of the enqueue callers.
**Failure Mode:** Queue-full returns `QUEUE_CAPACITY_EXCEEDED` immediately; the server must surface this to the operator without retrying silently. Worker errors emit a `FAILOVER_FAILED` result and leave the primary unchanged (fail-closed).

### Critical Integration: preventSplitBrain() ↔ Storage and Sharding Health
**Files:** `src/failover/auto_failover_manager.cpp` ↔ `include/storage/`, `include/sharding/` (health_check)
**Contract:** `AutoFailoverManager::preventSplitBrain()` (Wave A, 2026-07-29) calls into storage and sharding health-check interfaces to confirm quorum before executing a promotion; if quorum cannot be confirmed the method returns `SPLIT_BRAIN_RISK` and promotion is aborted.
**Thread Safety:** Health-check calls are read-only and non-locking; the decision to abort is recorded atomically in the failover state machine.
**Failure Mode:** Fail-closed — promotion is never executed when quorum is ambiguous. The conservative bias is intentional to avoid data-loss split-brain scenarios.

### Critical Integration: DisasterRecoveryManager ↔ DR Plan Validation
**Files:** `include/failover/disaster_recovery_manager.h` ↔ `src/failover/disaster_recovery_manager.cpp`
**Contract:** `DisasterRecoveryManager::execute(plan, dry_run)` validates the full DR plan (step ordering, dependency checks, resource availability) before any mutation; validation errors abort the run and return a typed `PlanValidationError`. Dry-run mode executes all validation and emits telemetry without applying state changes.
**Thread Safety:** Each DR execution holds an exclusive lock; concurrent DR executions are rejected with `DR_ALREADY_RUNNING`.
**Failure Mode:** Step failure halts the pipeline at the failing step; completed steps are not automatically rolled back. The operator is responsible for remediation before re-running.



- Verified files:
  - src/failover/auto_failover_manager.cpp
  - src/failover/disaster_recovery_manager.cpp
  - include/failover/auto_failover_manager.h
  - include/failover/disaster_recovery_manager.h
- Verified architecture claims:
  - explicit monitoring/queue, recovery, and telemetry planes
  - bounded failure behavior for invalid plans and manager dependencies
  - module-local ownership of failover/recovery orchestration contracts