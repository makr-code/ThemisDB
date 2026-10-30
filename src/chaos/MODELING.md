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

# Chaos Module State Model

## Purpose and boundary

This document describes the runtime state model for developers writing tests
against `src/chaos`. The module is a rule-based, in-process simulator; it does
not contain a statistical failure model, an infrastructure model, or a learned
model. `FaultType` identifies a descriptor consumed by test code; it does not
execute the named failure.

## Fault registry

The registry is a map keyed by target node ID and fault type. Each entry holds
the original `FaultSpec`, an injection time, and an expiry time. For a positive
duration, expiry is `injected_at + duration`; zero or negative duration maps to
`steady_clock::time_point::max()` and remains active until explicitly removed.
Injecting an already-active target/type pair replaces its entry (last writer
wins).

`isFaultActive()` returns false for an expired entry but does not itself prune
that entry. `getActiveFaults()` and `activeFaultCount()` prune expired entries
before reporting. `recoverFault()` removes matching entries; `clearAllFaults()`
clears the registry without per-entry recovery callbacks.

## Scheduler state and transitions

The scheduler has two externally visible states:

```text
STOPPED --start()--> RUNNING --stop()--> STOPPED
```

Entries can be queued in either state. While running, one worker periodically
scans the queue, removes every entry whose `trigger_at` has elapsed, and calls
`injectFault()` for each. `FIXED_TICK` waits with `sleep_for`; `CONDVAR` waits
with a condition variable and is promptly notified for stop. Although enqueue
calls notify the condition variable, its wait predicate only checks whether
the scheduler is stopping, so a newly queued entry does not bypass the tick
wait. The trigger time is therefore subject to the configured tick window.

`stop()` ends and joins the worker; it does not clear the pending queue.
Stopped entries remain in memory and may be considered after a later `start()`.
Use `clearPending()` when they must not fire. A scheduler with a null injector
throws `std::invalid_argument` at construction.

## Event and concurrency semantics

Callbacks run synchronously in registration order. Injection dispatch happens
after the registry mutation; recovery dispatch currently occurs while the
registry mutex is held. A recovery callback must not re-enter the same
`FaultInjector`. Callback exceptions are not caught: they propagate from direct
calls, and an exception escaping a scheduler worker can terminate the process.
`clearAllFaults()` does not dispatch recovery callbacks.

The registry protects its active map, and the scheduler protects its pending
queue. Callback registration is not synchronized with callback dispatch;
register callbacks before starting concurrent use. Serialize scheduler
`start()`/`stop()` lifecycle operations at the call site.

## Limits of the model

The normative recommended limits in
[chaos_contract.h](../../include/chaos/chaos_contract.h)—including tick,
duration, pending-entry, and callback limits—are not all enforced by
[chaos_framework.cpp](chaos_framework.cpp). The runtime rejects an empty target
ID and probabilities less than zero or greater than one at injection time, but
callers should provide a finite probability in range; NaN is not rejected by
those comparisons. Scheduler enqueue operations do not validate descriptors or
impose a queue capacity.

For test design and evidence, see [RESEARCH.md](RESEARCH.md) and
[IMPLEMENTATION.md](IMPLEMENTATION.md).
