---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-18
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos module future enhancements

This is a proposal list, not an approved delivery plan. No numeric performance
target or release date is approved without retained baseline evidence.

## Scope

- Harden the process-local fault registry and scheduler contract for v2.5.0 →
  v3.0.0. Do not claim real network, disk, process, or cluster fault injection.

## Design Constraints

- Invalid input must be rejected without registry mutation or callback
  dispatch.
- Keep scheduler work, recovery, and callbacks bounded; document existing
  limitations until their runtime behavior is changed and tested.
- Do not describe registered tests or benchmark sources as execution evidence.

## Required Interfaces

- Preserve the `FaultInjector` and `ChaosScheduler` public surface unless an API
  compatibility review approves a change.
- Treat [`../../include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h)
  as the source for descriptor, timing, and capacity recommendations.

## Implementation Notes

- Current `injectFault()` rejects non-finite/out-of-range probability and empty
  targets, but the scheduler still accepts unvalidated descriptors and has no
  queue cap. Recommended duration, tick, identifier, and callback limits are
  not enforced.
- Define callback exception and scheduler-worker failure handling before
  changing the synchronous callback contract.
- `bench_chaos_stress` source discovery now uses the actual `benchmarks/chaos/`
  path. Build/run evidence is still required.

## Test Strategy

- Extend live-runtime tests for accepted probability boundaries, non-finite and
  out-of-range rejection without state/callback side effects, scheduler
  validation/capacity, callback failures, and recovery.
- Run registered lifecycle, concurrency, callback, scheduler-timing, and stress
  suites; retain CI outcomes. Stub-only soak tests are not live-runtime evidence.

## Performance Targets

- Run stress, release-gate, and microbenchmark targets on named hardware and
  retain p50/p95/p99, throughput, queue depth, and resource use.
- Establish numeric release thresholds only after representative baselines,
  variance policy, and a test proving threshold violations fail the gate.
  Until then, no numeric runtime performance guarantee is claimed.

## Security / Reliability

- Keep scenarios in isolated, approved test processes; the module has no
  authorization, persistent audit, or infrastructure-control boundary.
- Verify invalid input leaves state unchanged and recovery/cleanup returns the
  harness to an empty pending/active state. Do not rely on the unresolved
  optional server admin integration.

Evidence and current gaps: [`ROADMAP.md`](ROADMAP.md),
[`API.md`](API.md), and
[`../../benchmarks/chaos/README.md`](../../benchmarks/chaos/README.md).
