[docs](../../README.md) > [en](../INDEX.md) > [failover](./README.md) > [overview](./README.md)
**Date:** 2026-05-12
**Status:** review
**Primary Source:**
- `src/failover/README.md`
- `src/failover/ARCHITECTURE.md`
- `src/failover/ROADMAP.md`
- `src/failover/FUTURE_ENHANCEMENTS.md`
- `include/failover/README.md`

**Reference:**
- Issue: `[MODULE] failover`
- Context: Reality check and module-level Secondary documentation sync for the failover module.

---

# Failover Module

## TL;DR

Core failover/DR functionality is implemented and test-covered.
Remaining gaps are roadmap/enhancement items (cross-region integration, ABI/soak validation, extended metrics export), not missing baseline runtime logic.

## Reality Check Summary

- Implemented components:
  - `AutoFailoverManager` (`include/failover/auto_failover_manager.h`, `src/failover/auto_failover_manager.cpp`)
  - `DisasterRecoveryManager` (`include/failover/disaster_recovery_manager.h`, `src/failover/disaster_recovery_manager.cpp`)
- Main source components:
  - `AutoFailoverManager` monitoring loop + failover worker queue
  - `DisasterRecoveryManager` 7-step recovery pipeline with per-step results
- Existing tests:
  - `tests/test_auto_failover_manager.cpp` (39 tests)
  - `tests/test_disaster_recovery_manager.cpp` (8 tests)
  - `tests/test_failover_chaos_scenarios.cpp` (17 tests)

## Roadmap / Future Enhancements Verification

- `src/failover/ROADMAP.md` matches the current implementation status for completed and open items.
- `src/failover/FUTURE_ENHANCEMENTS.md` is actionable and follows the required structure.
- `include/failover/` currently ships public headers plus `README.md`; include-side roadmap/future docs are not present in this module snapshot, so header/test inspection is the authoritative source for include-boundary behavior.

## Public API and Configuration

### `AutoFailoverManager`

- Lifecycle: `start()`, `stop()`, `isRunning()`
- Manual control: `triggerManualFailover(failed_node_id, target_promote_id)`
- Observability: `getStatistics()`, `getLastFailoverResult()`, `registerEventCallback()`
- Important config knobs:
  - `consecutive_failures_before_action`
  - `max_concurrent_failovers`
  - `queue_pressure_threshold`
  - `enable_network_partition_detection`
  - `enable_split_brain_prevention`
  - `enable_automatic_recovery`, `recovery_retry_interval`, `max_recovery_attempts`

### `DisasterRecoveryManager`

- Control path: `validatePlan()`, `executePlan()`
- Override/test hooks: `setStepHook()`, `clearStepHooks()`
- Important config knobs:
  - `require_quorum`
  - `enforce_epoch_fencing`
  - `catchup_timeout`
  - `verification_timeout`
  - `max_verification_retries`
  - `allow_dry_run_without_managers` (declared in the public config; current implementation already permits dry-run execution without managers)

## Runtime Behavior, Error Cases, and Limits

- `AutoFailoverManager` rejects manual failovers while stopped and when the queue is full.
- Queue-pressure telemetry is emitted once queue fill ratio reaches `queue_pressure_threshold`.
- Network partition handling can increment statistics and enter graceful split-brain prevention behavior when enabled.
- `DisasterRecoveryManager` rejects invalid plans early; non-dry-run plans need a `snapshot_id`.
- Missing managers become hard failures when quorum checks or epoch fencing are required.
- Verification is retry-bounded; catchup and verification both stop on timeout.

## Installation

The module is part of the standard ThemisDB build; no separate installation step is required.

## Usage

- Use `AutoFailoverManager::start/stop/triggerManualFailover` for failover orchestration control.
- Use `DisasterRecoveryManager::validatePlan/executePlan` for DR execution.
- See the public header overview in [`../../../include/failover/README.md`](../../../include/failover/README.md).

```cpp
#include "failover/auto_failover_manager.h"

themis::failover::AutoFailoverConfig cfg;
cfg.max_concurrent_failovers = 4;
cfg.queue_pressure_threshold = 0.5f;

themis::failover::AutoFailoverManager mgr(cfg, replication_mgr, health_monitor, spare_mgr, fencing_mgr);
mgr.start();
mgr.triggerManualFailover("node-a");
```

```cpp
#include "failover/disaster_recovery_manager.h"

themis::failover::DisasterRecoveryPlan plan;
plan.plan_id = "dr-plan-1";
plan.primary_site = "dc-a";
plan.recovery_site = "dc-b";
plan.snapshot_id = "snapshot-42";

auto result = dr_mgr.executePlan(plan);
```

## Troubleshooting

- `triggerManualFailover()` returns `false`: ensure the manager is running and the queue is not saturated.
- Repeated `QUEUE_PRESSURE` events: inspect `max_concurrent_failovers`, enqueue rate, and worker drain behavior.
- DR error `snapshot_id must not be empty for non-dry-run recovery`: provide a snapshot or use `dry_run=true`.
- DR fails on quorum/fencing requirements: wire the replication/fencing managers or relax the config intentionally for a dry lab scenario.
- See [Missing implementations report (DE)](../../de/failover/MISSING_IMPLEMENTATIONS.md) for open delivery gaps.

## Related Documents

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [Public API (`include/failover/README.md`)](../../../include/failover/README.md)
- [Implementation overview (`src/failover/README.md`)](../../../src/failover/README.md)
- [Architecture](../../../src/failover/ARCHITECTURE.md)
- [Roadmap](../../../src/failover/ROADMAP.md)
- [Future enhancements](../../../src/failover/FUTURE_ENHANCEMENTS.md)
- [Missing implementations report (DE)](../../de/failover/MISSING_IMPLEMENTATIONS.md)
