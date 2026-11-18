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

# Chaos performance expectations

## Source and build mapping

| Source | Example cases | Current CMake status |
|---|---|---|
| [`../../benchmarks/chaos/bench_chaos_stress.cpp`](../../benchmarks/chaos/bench_chaos_stress.cpp) | `BM_CallbackDispatch`, `BM_ConcurrentStress`, `BM_ChaosScheduler_Schedule` | CMake condition checks `chaos/bench_chaos_stress.cpp` and adds the target when present; build/run evidence is still required. |
| [`../../benchmarks/chaos/bench_chaos_release_gates.cpp`](../../benchmarks/chaos/bench_chaos_release_gates.cpp) | `BM_GATE_CHS01_InjectThroughput` through `BM_GATE_CHS06_CallbackOverhead` | CMake conditionally adds and registers `bench_chaos_release_gates` when the source exists. |
| [`../../benchmarks/chaos/bench_chaos_microbenchmarks.cpp`](../../benchmarks/chaos/bench_chaos_microbenchmarks.cpp) | inject/recover latency and concurrent throughput scenarios | CMake conditionally adds and registers `bench_chaos_microbenchmarks` when the source exists. |

## Declared thresholds versus verified performance

The release-gate source declares counter thresholds including inject/recover
throughput, query latency, concurrent throughput, scheduler enqueue throughput,
and callback overhead. The microbenchmark source declares p99 and throughput
threshold counters. These values are source declarations, not proof of measured
results, a baseline comparison, or automatic pass/fail enforcement.

**Validation gate:** build all three targets, run them on named representative
hardware, and retain raw results with p50/p95/p99, throughput, queue depth, and
resource use. Establish an approved baseline and variance policy before
enforcing thresholds; verify that an injected threshold violation fails the
gate. No performance result or approved numeric threshold is asserted here.
