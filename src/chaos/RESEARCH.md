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

# Chaos Module Research Notes

## Purpose and scope

This page records the evidence and evaluation approach for the in-process chaos
simulator. It is for maintainers deciding whether a test scenario can use the
module; it is not evidence that the simulator reproduces real infrastructure
failures.

The implementation provides two mechanisms: `FaultInjector` stores
process-local fault descriptors, and `ChaosScheduler` submits descriptors to
that registry after a time trigger. Neither mechanism performs network, disk,
process, or cluster operations. The system under test must explicitly query
`isFaultActive()` and decide how to respond
([public header](../../include/chaos/chaos_framework.h),
[runtime](chaos_framework.cpp)).

## Assumptions and questions

- Tests control the process using the injector and scheduler APIs; the fault
  descriptors themselves do not cause application behavior.
- A `FaultType` is a label for test logic. For example, `DISK_FAILURE` does not
  intercept disk I/O. Probability comparisons reject values below 0 or above 1,
  but NaN passes those comparisons; probability is not sampled by this runtime.
- All timing is process-local and uses `std::chrono::steady_clock`; restart
  discards registry and queue state.
- The relevant research question is whether consumers observe the intended
  state transitions and recover to baseline, not whether a real network/storage
  subsystem has failed.

## Evidence and evaluation

The unit and stress test sources provide candidate reproducible evidence; this
document does not assert that they were built or passed:

- [framework tests](../../tests/chaos/test_chaos_framework.cpp) cover registry
  lifecycle, expiry, validation, callbacks, and scheduled firing.
- [scheduler stress tests](../../tests/chaos/test_chaos_stress.cpp) exercise
  both wake strategies, bursts, concurrent scheduling, and stop behavior.
- [callback determinism tests](../../tests/chaos/test_chaos_callback_determinism.cpp)
  cover update, callback, and clearing semantics.
- [microbenchmarks](../../benchmarks/chaos/bench_chaos_microbenchmarks.cpp)
  and [release gates](../../benchmarks/chaos/bench_chaos_release_gates.cpp)
  provide performance measurement code; their existence is not a recorded
  successful run or a hardware-independent performance guarantee.

For a new scenario, define the expected observable state, trigger window,
recovery action, and failure criterion before testing. Verify that pending work
and active entries return to zero. Do not infer correctness of real I/O or
distributed failure behavior from these tests.

## Limitations and trade-offs

The model leaves effect semantics to test consumers. Scheduler processing is
tick-based rather than an exact-time guarantee. The runtime does not enforce the recommended duration,
tick, queue, identifier-length, or callback-count limits in
[chaos_contract.h](../../include/chaos/chaos_contract.h). Callers must bound
their own workloads. See [model assumptions](MODELING.md),
[operations](OPERATIONS.md), and the [roadmap](ROADMAP.md) for scope and
planned work.
