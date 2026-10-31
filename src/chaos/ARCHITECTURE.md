---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos module architecture

## Runtime components

`FaultInjector` in [`chaos_framework.cpp`](chaos_framework.cpp) stores one
in-memory entry per target ID and fault type. It supports injection, recovery,
active-state queries, expiry pruning, and synchronous event callbacks.
`ChaosScheduler` owns a pending vector and a worker thread that checks due
entries on each configured tick and submits them to a `FaultInjector`.

The public declarations are in
[`../../include/chaos/chaos_framework.h`](../../include/chaos/chaos_framework.h);
the v1.x recommended contract is in
[`../../include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h).
The module records simulated fault labels; it does not itself manipulate
network or disk I/O.

## Implementation limits and failure behavior

- Injection rejects an empty target ID and probability values below 0 or above
  1; the current comparisons do not reject NaN. Probability is not sampled.
- Recommended duration, tick, pending-entry, identifier, and callback limits in
  `chaos_contract.h` are not enforced by this runtime.
- The condition-variable strategy still processes entries at the tick cadence;
  its wait predicate wakes for stop, not for enqueue notifications.
- Callbacks are synchronous and exceptions are not caught. Recovery callbacks
  run while the injector mutex is held; callback registration is unsynchronized.
- `stop()` joins the worker but retains pending entries; `clearPending()` removes
  them. Registry and queue state are process-local.

## HTTP admin integration (build status TODO)

Server source contains a guarded `ChaosAdminApiHandler` and route dispatch for
`/admin/chaos/inject`, `/reset`, `/status`, and `/history`; the guarded route
calls `requireAccess(req, "admin", "admin.chaos", path)`. CMake defaults
`THEMIS_CHAOS_ADMIN` to `OFF` and conditionally adds the handler source when both
HTTP server support and the option are enabled.

The build wiring does not establish a working endpoint: CMake defines
`THEMIS_CHAOS_ADMIN` on `themis_server`, while the guarded HTTP server and
handler sources are compiled as module sources; the definition is not visibly
propagated to those source-owning targets. Additionally, `http_server.cpp`
constructs `ChaosScheduler` with no argument although its public constructor
requires an injector. **TODO:** reconcile target compile definitions and
constructor setup, then build/test the enabled configuration. Do not describe
the route as live or production-ready until verified. CMake warns against
shipping the option in production.

## Test and benchmark wiring

`tests/chaos/CMakeLists.txt` globs `test_*.cpp`; eight chaos test sources are
present in this checkout. See
[`../../benchmarks/chaos/README.md`](../../benchmarks/chaos/README.md) for the
benchmark source/target discrepancy and the two separately registered benchmark
targets.
