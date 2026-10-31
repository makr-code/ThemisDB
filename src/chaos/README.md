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

# ThemisDB Chaos Module

## Module Purpose

The module provides an in-process fault registry and time-driven scheduler for
test consumers. A fault descriptor is state recorded by the module; consumer
code must implement and verify any associated simulated effect. The module does
not perform network or disk manipulation.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| [`include/chaos/chaos_framework.h`](../../include/chaos/chaos_framework.h) | `FaultInjector`, `ChaosScheduler`, and fault/schedule types |
| [`include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h) | v1.x contract and recommended interoperability bounds |
| `chaos_framework.cpp` | Registry and scheduler implementation |

## Operations

- [RUNBOOK.md](RUNBOOK.md) — simulated-fault diagnosis and cleanup
- [OPERATIONS.md](OPERATIONS.md) — supported signals and safety boundary

## Scope

In scope:
- in-process fault lifecycle registration and recovery controls
- configurable, tick-based scheduler-driven fault activation
- callback and event signaling for injected/recovered fault states

Out of scope:
- external cluster-wide orchestration beyond process-local chaos surfaces
- direct OS/network sabotage beyond modeled simulation semantics
- non-chaos business workflows outside resilience simulation boundaries

## Runtime Behavior and Limits

- active faults are process-local and keyed by configured fault identity dimensions.
- scheduler execution depends on configured wake strategy and tick behavior.
- `injectFault()` returns `false` for empty target IDs or probabilities below 0
  or above 1; it does not sample probabilities and does not reject NaN with its
  current range comparisons.
- The runtime does not enforce the contract's recommended queue, duration, tick,
  identifier-size, or callback-count bounds.
- Callbacks run synchronously; recovery callbacks run while the injector mutex is
  held, and callback exceptions are not caught.

## Sourcecode Verification (Module: chaos/readme)

- Verified files:
  - src/chaos/chaos_framework.cpp
- Verified behavior surfaces:
  - fault registration/recovery, expiry, and active-state inspection
  - scheduler-managed deferred fault injection on tick checks
  - synchronous callback dispatch and process-local state
- Note:
  - server source contains guarded admin HTTP code, but its compile-definition
    scope and `ChaosScheduler` construction are unresolved; see `ARCHITECTURE.md`
  - benchmark build status is documented in `../../benchmarks/chaos/README.md`
  - roadmap completion statements are not evidence of executed tests or benchmark results