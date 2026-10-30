---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Module API

## Purpose and public surface

This page describes the in-process simulation API declared in
[`include/chaos/chaos_framework.h`](../../include/chaos/chaos_framework.h).
The normative v1.x interoperability constraints are in
[`include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h).
No method in this API performs real network or disk manipulation.

## Types

| Type | Contract |
|---|---|
| `FaultType` | Descriptor label: `NODE_FAILURE`, `NETWORK_PARTITION`, `LEADER_CRASH`, `DELAYED_RESPONSE`, `DISK_FAILURE`, `RANDOM_FAILURE`, or `DISASTER_RECOVERY_DRILL`. A consumer assigns the effect. |
| `FaultSpec` | `type`, `target_node_id`, `duration`, `probability`, and `description`. Empty target IDs are rejected by `injectFault()`. Duration zero (and, in the current implementation, negative durations) means no expiry. |
| `ActiveFault` | A spec with `injected_at` and `expires_at`; `isExpired()` uses `steady_clock`. |
| `ChaosScheduleEntry` | A `steady_clock` trigger time and the `FaultSpec` to submit when due. |
| `WakeStrategy` | `FIXED_TICK` or `CONDVAR`; both process entries on the configured tick. `CONDVAR` wakes promptly for stop; enqueue notifications do not bypass the tick wait because the wait predicate only checks the running state. |
| `ChaosSchedulerConfig` | Tick interval defaults to 10 ms and wake strategy defaults to `FIXED_TICK`. Recommended bounds are in `chaos_contract.h`; the runtime does not enforce them. |

## `FaultInjector`

| Method | Behavior and edge cases |
|---|---|
| `FaultInjector(std::string injector_id = "default")` | Creates an empty process-local registry. `id()` returns the supplied identifier. |
| `bool injectFault(const FaultSpec&)` | Returns false for empty target ID or probability `< 0` / `> 1`; otherwise inserts or replaces the target/type entry and returns true unless a callback throws. Probability is not sampled here. Callback exceptions propagate; NaN is not rejected by the current comparisons. |
| `bool recoverFault(const std::string& target_node_id)` | Removes every matching target entry; false if none matched. Recovery callbacks run synchronously while the registry mutex is held and before each removal. |
| `bool recoverFault(const std::string& target_node_id, FaultType)` | Removes one target/type entry; false if absent. Callback exception propagates before erase. |
| `bool isFaultActive(const std::string&) const` and typed overload | Reports whether a matching non-expired entry exists. Expired entries may remain stored until a pruning operation. |
| `std::vector<ActiveFault> getActiveFaults()` / `size_t activeFaultCount()` | Prune expired entries, then return a snapshot or count. |
| `void clearAllFaults()` | Clears all entries without per-entry recovery callbacks. |
| `void registerEventCallback(EventCallback)` | Appends a synchronous lifecycle callback `(const FaultSpec&, bool injected)`. Register before concurrent use; callback registration is not synchronized. |

Callbacks are invoked in registration order. Injection callbacks run after
registry mutation; recovery callbacks run under the injector mutex. Do not
re-enter the same injector from a recovery callback. Callback exceptions are
not caught by the framework.

## `ChaosScheduler`

Construction requires a non-null `shared_ptr<FaultInjector>` and throws
`std::invalid_argument` otherwise.

| Method | Behavior and edge cases |
|---|---|
| `schedule(ChaosScheduleEntry)` | Appends an entry whether running or stopped; it does not validate or cap the queue. |
| `scheduleIn(milliseconds, const FaultSpec&)` | Converts a relative delay to a `steady_clock` trigger. Negative delays are already due when processed. |
| `start()` | Starts the worker unless already running. A stopped scheduler may process retained pending entries. |
| `stop()` | Stops and joins the worker. It does not clear pending entries. |
| `isRunning()` / `pendingCount()` | Report worker state and queued entry count. |
| `clearPending()` | Discards all unfired entries; it does not affect active faults. |

The worker scans due entries at each configured tick and invokes
`FaultInjector::injectFault()`. A callback exception escaping this worker is not
caught by the scheduler. Serialize lifecycle calls and keep callbacks
non-throwing. See [OPERATIONS.md](OPERATIONS.md) for safe cleanup and
[MODULE_GUIDE.md](MODULE_GUIDE.md) for an integration sequence.
