# PERFORMANCE_EXPECTATIONS - src/distributed_knowledge

## Scope

- Module: src/distributed_knowledge
- This file defines measurable distributed_knowledge module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_distributed_knowledge.cpp
  - benchmarks/bench_distributed_knowledge_or.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| DKP-1 | aggregation trigger path remains within release baseline budget across configured participant scales | BM_TriggerAggregation_N64, BM_TriggerAggregation_NoTimeout |
| DKP-2 | federated merge path remains bounded under configured shard/doc fan-out | BM_FederatedRAGMerge_N16x50, BM_MergeWithTimedOutShards |
| DKP-3 | feedback dedup and publish paths remain bounded and stable | BM_FeedbackDedup_Throughput, BM_PublishFeedback_Latency, BM_FeedbackPublish_BackpressureSkip |
| DKP-4 | coordinator erase/cleanup and low-risk trust path remain bounded | BM_Erase_FederationCoordinator, BM_ZeroTrust_LowRiskPath |

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
  - benchmarks/bench_distributed_knowledge.cpp
  - benchmarks/bench_distributed_knowledge_or.cpp
- Verified mapping surfaces:
  - aggregation and timeout-resilient trigger paths
  - federated merge including timed-out shard scenarios
  - feedback dedup/publish and trust/cleanup benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.