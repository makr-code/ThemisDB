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

# Chaos public-header architecture

`include/chaos/chaos_framework.h` declares the process-local `FaultInjector`
registry and `ChaosScheduler`; `include/chaos/chaos_contract.h` states the
v1.x contract and recommended interoperability limits. Runtime implementation
details are in [`../../src/chaos/ARCHITECTURE.md`](../../src/chaos/ARCHITECTURE.md).

## Public surface and implementation boundary

The public types are in `themis::chaos`. The framework stores/query fault
descriptors and schedules them; it does not itself manipulate network or disk
I/O. Consumer code defines any simulated effect. The header declares a fixed
tick or condition-variable wake strategy, but the queue has no enforced capacity.

## Runtime enforcement visible in source

- `injectFault()` rejects an empty target ID and probabilities below 0 or above
  1; it does not sample the probability or reject NaN with its current
  comparisons.
- The runtime does not enforce the contract's recommended duration, tick,
  queue-size, identifier-size, or callback-count bounds.
- `stop()` joins the worker and retains pending entries; `clearPending()` clears
  them explicitly.
- Callback registration is unsynchronized. Injection callbacks run after
  registry mutation; recovery callbacks run synchronously while the registry
  mutex is held.

## Build and verification references

Focused tests are discovered from `tests/chaos/test_*.cpp` by
[`../../tests/chaos/CMakeLists.txt`](../../tests/chaos/CMakeLists.txt); that
directory currently contains eight test sources. Benchmark source/build status
is tracked in [`../../benchmarks/chaos/README.md`](../../benchmarks/chaos/README.md).

The HTTP admin integration is separate from this public header API. Its source
and build guard are described in the implementation architecture page; enabling
the feature is a deployment decision, not a property of these headers.
