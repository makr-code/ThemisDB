---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-19"
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
| DKG-1 | p95 and p99 regression <= 10 percent vs a pinned, same-hardware release baseline for every production-path case | (current - baseline) / baseline |
| DKG-2 | Every mapped benchmark case is present in the release run manifest, with build/profile, hardware, commit, and raw result metadata | benchmark run manifest completeness |
| DKG-3 | Five independent release-profile runs per mapped case; report p50/p95/p99 and retain the baseline comparison artifact | run artifacts and comparison data |
| DKG-4 | No performance sign-off from synthetic/in-memory cases alone; include the service's real shard fan-out path before release | benchmark source and topology evidence |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- A baseline is valid only when hardware, compiler, build profile, commit, and workload are recorded and the comparison uses the same workload configuration.
- Missing case data, a missing baseline, or a regression above 10 percent is a failed gate, not a pending performance claim.
- Mapping should be expanded as additional distributed_knowledge benchmark scenarios are introduced.

## v2.5.0 -> v3.0.0 correctness and safety gates

| Gate | Pass condition | Current evidence boundary |
|---|---|---|
| DK-COR-1 | All registered `module_distributed_knowledge_*_focused` tests pass from a build that links `themis_distributed_knowledge`; record the exact build and CTest output | Focused source inventory is not a run result; the production library now includes the distillation implementation |
| DK-SAF-1 | Unknown capability domains, absent trust policy, and rejected trust policy cause zero inbound callback dispatches | Covered by focused regressions; trust policy is not peer authentication |
| DK-DEG-1 | With three supplied shard results, one failed/timed-out shard yields an explicitly partial merge; all three timed out fails; invalid configuration throws | In-process component tests only; no real shard requests are issued by this module |
| DK-OPS-1 | Operators can distinguish callback attempt, skipped publish, policy rejection, partial RAG merge, and process-local state loss | Local counters/exceptions only; external delivery acknowledgement and durable audit remain host responsibilities |
| DK-REC-1 | Any hard-deadline, retry, or restart-recovery claim has cancellation-aware implementation and a real failure-path test | Not met: no automatic retry/recovery loop exists, and the LoRA timeout overload cannot cancel work |

Until DK-COR-1 through DK-OPS-1 have reproducible run artifacts and DK-REC-1
is either implemented or explicitly excluded from the release contract, the
module is not eligible for v3.0.0 performance/safety sign-off. The current
`bench_dk_release_gates` target measures local in-memory/mock cases and cannot
satisfy DKG-4.

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