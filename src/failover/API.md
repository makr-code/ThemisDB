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

# Failover API

## Scope

The public C++ API is declared in `include/failover/`. This page summarizes
inputs, outcomes, and operational caveats for callers; the headers remain the
contract source.

## Automatic failover

`themis::failover::AutoFailoverManager` is constructed with an
`AutoFailoverConfig` and shared pointers to the replication, health-monitor,
hot-spare, and epoch-fencing managers.

| Method | Contract and caller action |
|---|---|
| `start()` / `stop()` | Start or stop the monitor/worker lifecycle. Both return `false` when the requested transition cannot be performed (including already-running/already-stopped cases). |
| `triggerManualFailover(failed_node_id, target_promote_id)` | Queue a request only while running and while capacity remains; returns `false` if stopped or full. The current worker does not use the queued `target_promote_id` when selecting a candidate, so callers must not rely on it to force a target. |
| `getState()`, `isFailoverInProgress()`, `getFailingNodes()`, `getLastFailoverResult()` | Read current in-process state and the latest result. |
| `updateConfig()` / `getConfig()` | Replace/read runtime configuration. Review timing and concurrency changes against quorum and fencing behavior. |
| `getStatistics()`, `resetStatistics()`, `registerEventCallback()` | Read/reset in-process counters and subscribe to lifecycle events. These are not a promise of externally exported metrics. |

Defaults include three consecutive failures before action, a queue capacity of
two pending requests, a 75% queue-pressure threshold, a 5-second health-check
call timeout, and a 30-second quorum wait. Promotion depends on the injected
replication and fencing managers. Queue rejection is surfaced to the caller and
counted; there is no automatic retry of a rejected manual enqueue. A queued
`target_promote_id` is not currently used to select the promotion candidate.

## Disaster recovery

`DisasterRecoveryManager::executePlan(const DisasterRecoveryPlan&)` returns a
`DisasterRecoveryResult`, including final state, error text, fenced epoch,
duration, and per-step outcomes. `validatePlan()` requires a non-empty
`plan_id`, `primary_site`, and `recovery_site`; non-dry-run plans also require a
non-empty `snapshot_id`.

`executePlan()` runs prechecks, snapshot validation, epoch fencing, restore,
replica catch-up, traffic shift, and verification in that order. Only one
uncached execution may run at a time. Results for non-empty plan IDs are cached
for this manager instance, including failures; retrying with the same ID returns
the cached result. A new ID is not a safe retry until the caller has reconciled
partial state.

`setStepHook()` replaces a built-in handler for a step; `clearStepHooks()`
removes those replacements. Hooks are callable code, not automatically
side-effect-free test mocks. Protect hook registration from concurrent execution
and ensure any hook used in a dry run explicitly preserves dry-run safety.

Important current behavior: the built-in snapshot check tests only ID presence;
restore and traffic shift report success without calling storage or traffic
adapters; catch-up waits for quorum; verification checks health/quorum but not
restored data or recovery-site identity. A successful result does not by itself
prove that a real restore or cutover occurred.

## Supporting types

- `FailoverErrorCode`, epoch helpers, and quorum-size calculation are declared
  in `include/failover/failover_api_contract.h`.
- `QuorumLog` exposes append and recovery operations for an optional local log.
  Recovery can skip invalid records; CRC32 is not an authentication mechanism.
- `TopologySnapshot` captures node IDs and failure counts for deterministic
  topology comparisons.

The public API documents declarations, not proof of external side effects.
`AutoFailoverManager::stop()` uses direct thread joins; there is no enforced
timed-join bound.

## References

- [IMPLEMENTATION.md](IMPLEMENTATION.md)
- [TRUST.md](TRUST.md)
- `include/failover/auto_failover_manager.h`
- `include/failover/disaster_recovery_manager.h`
- `include/failover/failover_api_contract.h`
- `include/failover/quorum_log.h`
- `include/failover/topology_snapshot.h`
