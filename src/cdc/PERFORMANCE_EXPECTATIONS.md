---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "benchmarks/cdc/bench_changefeed_throughput.cpp; benchmarks/cdc/bench_cdc_pipeline.cpp; benchmarks/cdc/bench_cdc_delivery_gates.cpp; benchmarks/replication/bench_replication_throughput.cpp; benchmarks/CMakeLists.txt"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC performance expectations

This page maps CDC performance expectations to benchmark sources. It is not
evidence that a benchmark was run or that a release baseline passed.

## Benchmark sources and cases

| Source | Cases / coverage |
|---|---|
| [`benchmarks/cdc/bench_changefeed_throughput.cpp`](../../benchmarks/cdc/bench_changefeed_throughput.cpp) | `BM_EventTypeMix`, `BM_BurstWrites`, `BM_ReplicationLag`, `BM_ReplicationLagWAN`, `BM_RecordEventLatency`, `BM_ListEventsLatency` |
| [`benchmarks/cdc/bench_cdc_pipeline.cpp`](../../benchmarks/cdc/bench_cdc_pipeline.cpp) | Consumer-group create/delete, fetch, acknowledgement, and concurrent-fetch paths |
| [`benchmarks/cdc/bench_cdc_delivery_gates.cpp`](../../benchmarks/cdc/bench_cdc_delivery_gates.cpp) | `DeliveryTracker`, `ConsumerGroupManager`, and replay gates CDG-01..06 |
| [`benchmarks/replication/bench_replication_throughput.cpp`](../../benchmarks/replication/bench_replication_throughput.cpp) | Related WAL serialization, replication-manager, conflict-detection, and merge cases; these are replication benchmarks, not CDC benchmarks |

`benchmarks/CMakeLists.txt` registers `bench_cdc_pipeline` and
`bench_cdc_delivery_gates` when their source files are present. It does not
register `bench_changefeed_throughput.cpp` as a CMake target in this checkout;
do not infer a runnable target from the source file alone.

## Expectations

| Target | Expectation | Evidence to evaluate |
|---|---|---|
| CDC-1 | Mixed event-type and burst writes remain within the approved release baseline | Repeated benchmark runs and baseline comparison |
| CDC-2 | Local and WAN replication-lag cases remain within their approved envelopes | Reported latency distribution and run profile |
| CDC-3 | Event record/list latency remains within the approved release budget | `BM_RecordEventLatency` and `BM_ListEventsLatency` |
| CDC-4 | Delivery-gate cases remain within documented thresholds | CDG-01..06 definitions in `bench_cdc_delivery_gates.cpp` |
| CDC-5 | No required release benchmark case is missing | Benchmark run manifest |

Thresholds and release-gate results are maintained by the benchmark source,
baseline, and release workflow. The module roadmap records outstanding
re-baselining work; do not treat these expectations as a current production
SLO or a verified pass without run evidence.

## Validation

- Use the `bench_cdc_pipeline` and `bench_cdc_delivery_gates` targets in a build
  configured with benchmarks enabled.
- Record the build profile, hardware, sample count, run variance, and baseline
  used for comparisons.
- Track proxy mappings and missing CDC-specific cases as follow-up work rather
  than interpreting related replication benchmarks as direct CDC measurements.

**Exact model backend not verifiable from final document text alone.**
