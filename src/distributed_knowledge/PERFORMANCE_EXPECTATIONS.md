---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/distributed_knowledge

## Scope

- Module: src/distributed_knowledge
- This file defines measurable distributed_knowledge module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp`
  - `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`
  - `benchmarks/distributed_knowledge/bench_dk_release_gates.cpp`

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| DKP-1 | aggregation trigger path remains within release baseline budget across configured participant scales | BM_TriggerAggregation_N64, BM_TriggerAggregation_NoTimeout |
| DKP-2 | federated merge path remains bounded under configured shard/doc fan-out | BM_FederatedRAGMerge_N16x50, BM_MergeWithTimedOutShards |
| DKP-3 | feedback dedup and publish paths remain bounded and stable | BM_FeedbackDedup_Throughput, BM_PublishFeedback_Latency, BM_FeedbackPublish_BackpressureSkip |
| DKP-4 | coordinator erase/cleanup and low-risk trust path remain bounded | BM_Erase_FederationCoordinator, BM_ZeroTrust_LowRiskPath |

The separate `bench_dk_release_gates.cpp` target declares DKRG-01..DKRG-06
cases for in-memory entity insert, mock graph neighbour/path lookups, the
`resolveLww` helper, an in-memory federation union, and mock serialization.
These cases exercise local helpers and the API-contract function; they are not
measurements of a live shard topology or the full federation runtime.

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| DKG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| DKG-2 | federation hot-path p99 <= release threshold | p99 from mapped distributed_knowledge benchmark cases |
| DKG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional distributed_knowledge benchmark scenarios are introduced.

## Sourcecode Verification (Module: distributed_knowledge/performance)

- Verified benchmark sources:
  - benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp
  - benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp
  - benchmarks/distributed_knowledge/bench_dk_release_gates.cpp
- Verified mapping surfaces:
  - aggregation and timeout-resilient trigger paths
  - federated merge including timed-out shard scenarios
  - feedback dedup/publish and trust/cleanup benchmark paths
  - DKRG-01..DKRG-06 local contract/microbenchmark cases
- Result:
  - The mapped benchmark symbols exist in the current benchmark sources.
  - Source presence does not establish a successful run or threshold/baseline result; release conclusions require reproducible run artifacts and comparison data.