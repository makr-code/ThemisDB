---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Runbook

## Purpose and safety boundary

This is a runbook for the in-process chaos simulator used by tests and controlled
integration exercises. `FaultInjector` records simulated fault state; it does
not suppress real network packets or block real disk I/O. Server source contains
guarded `/admin/chaos/*` route code, but the CMake compile definition is scoped
to `themis_server` while `http_server.cpp` is compiled into `themis_core`, and
the route setup calls an unavailable no-argument `ChaosScheduler` constructor.
**TODO:** verify/fix and build the enabled integration. Do not treat those routes
as usable operational controls. CMake defaults `THEMIS_CHAOS_ADMIN` to `OFF`
and warns against shipping it in production.

Run fault injection only in an explicitly approved test/staging process with an owner, target, duration, and recovery observer identified. Do not treat this module as a production fault-control mechanism.

## Diagnose an active or unexpected fault

1. Capture the target node ID, fault type, description, configured duration, and injector/scheduler identity from the test harness.
2. Inspect `FaultInjector::getActiveFaults()`, `activeFaultCount()`, or `isFaultActive(target, type)`. The first two prune expired entries.
3. Inspect `ChaosScheduler::isRunning()` and `pendingCount()`. A stopped scheduler will not fire queued entries until it is started.
4. Check the registered event callback's injected/recovered events and the consumer/test logs. The framework itself does not provide a persistent audit log, Prometheus metrics, or alert delivery.
5. If application behavior looks like a real network or disk failure, investigate that subsystem independently; the chaos registry alone cannot prove an actual I/O fault occurred.

## Recover a fault safely

1. Stop scheduling new injections and record `pendingCount()` and the active-fault list.
2. Recover each affected node/type with `recoverFault(node_id, type)` and confirm it returns true; verify with `isFaultActive()` and `activeFaultCount()`.
3. Use `clearPending()` for scheduled entries that must not execute. `stop()` stops the worker; it is not a substitute for explicitly clearing pending work.
4. Verify the consumer has observed recovery and the test returns to baseline before continuing.
5. If a fault used duration zero, it is permanent until explicitly recovered. Restarting the process clears this process-local state, but it does not undo external state changed by a test consumer.

`recoverFault()` invokes callbacks synchronously while holding the fault mutex. Do not let a recovery callback re-enter the same injector; callback exceptions are not caught by the framework. `clearAllFaults()` clears registry state without emitting per-fault recovery callbacks, so prefer individual recovery when callback observers must reconcile state.

## Invalid injection or scheduler failure

- `injectFault()` returns false for an empty target ID or probability outside `[0, 1]`; correct the descriptor instead of retrying unchanged input.
- A repeated injection for the same target/type updates that active entry, including its expiry. Check the new duration before resubmitting.
- Verify a scheduler's non-null injector and running state. Queue entries have no enforced capacity limit in the current runtime; keep test schedules bounded by the caller.

## Rollback and escalation

Cancel the test run and clear pending/active simulated faults when the target or blast radius is uncertain. Do not repair real storage, network, or cluster state based solely on a chaos event. Escalate to the test owner if callbacks throw, a scheduler fails to stop, state cannot be reconciled, or the test affected a shared environment.

## References

- [OPERATIONS.md](OPERATIONS.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/chaos/chaos_framework.h`
- `include/chaos/chaos_contract.h`
- `src/chaos/chaos_framework.cpp`
