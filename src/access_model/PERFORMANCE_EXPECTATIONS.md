---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Performance Expectations

## Scope and benchmark source

This page records the numeric targets declared by
`benchmarks/access_model/bench_access_coordinator_gates.cpp` and distinguishes
those targets from measured evidence. The benchmark currently runs event
callbacks without setting an `AgeBasedPolicy`; the coordinator takes its
no-policy observation path. The current cases therefore do not establish
end-to-end transfer or policy-decision performance.

## Declared targets and current coverage

| Gate | Intended target | Benchmark case | Current source behavior |
|---|---|---|---|
| GATE-ACM-01 | L1→L2 promotion ≤ 50 µs p99 | `GATE_ACM_01_L1ToL2Promotion` | Calls `onStorageAccess(STORAGE_COLD, ...)` without a policy; no promotion is queued. |
| GATE-ACM-02 | Cache-eviction feedback ≤ 100 µs p99 | `GATE_ACM_02_CacheEvictionToStorageFeedback` | Calls `onCacheEvicted(...)` without a policy; no demotion is queued. |
| GATE-ACM-03 | Cold→warm transition ≤ 100 ms p99 | `GATE_ACM_03_ColdToWarmPromotion` | Sends three cold-storage access events per key without a policy; no adapter I/O occurs. |
| GATE-ACM-04 | Event throughput ≥ 10,000 events/sec | `GATE_ACM_04_EventProcessingThroughput` | Measures alternating callback calls on the no-policy path. |
| GATE-ACM-05 | Coordinator memory ≤ 50 MB for 1 M events | `GATE_ACM_05_MemoryOverhead` | Sends event batches and sleeps; it does not measure RSS or assert a memory bound. |
| GATE-ACM-06 | Policy decision ≤ 10 µs/event | `GATE_ACM_06_PolicyDecisionOverhead` | Calls `onCacheEvicted(...)` without a policy; the policy-decision path is not run. |

The latency cases calculate elapsed wall-clock time divided by the number of
iterations; this is an average, not a p99 measurement. The benchmark's
`reportViolation()` prints a diagnostic when the measured value exceeds its
constant target. It does not compare against a stored baseline or fail the
process, so neither the declared ±10% tolerance nor a release-gate pass is
enforced by this source.

## Available runtime metrics

`AccessModelMetrics::counters` exposes `pending_queue_depth` and
`max_queue_depth`. The queue-depth gauge covers waiting tasks, not in-flight
workers. Worker-thread utilization is not exposed by the access-model API.

## Validation required before claiming a gate passes

- Configure a valid policy in the benchmark fixture and verify that each case
  reaches the intended decision or adapter operation.
- Measure the intended percentile for latency gates and include the actual
  adapter I/O path when the target claims a tier transfer.
- Measure resident memory externally for the memory target.
- Compare recorded results with an explicit baseline and make threshold
  violations fail the validation job.
- Capture reproducible release-profile results on representative hardware.

Until that evidence exists, the constants are target definitions only. See
[`WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md`](./WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md)
for the current evidence status.

**AI provenance:** Exact model backend not verifiable from final document text alone.
