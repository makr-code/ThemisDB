# PERFORMANCE_EXPECTATIONS - src/llm

## Scope
- Module: src/llm
- This file defines measurable LLM module performance expectations for release gating.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/bench_llm_inference_performance.cpp
  - benchmarks/bench_llm_infrastructure.cpp
  - benchmarks/bench_llm_response_cache.cpp
  - benchmarks/bench_llm_raid_pipeline.cpp
  - benchmarks/bench_rag_hybrid_retriever.cpp

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| LLM-1 | Token throughput stays within release baseline budget | BM_LLM_TokenThroughput |
| LLM-2 | Prompt latency p95/p99 stays within release baseline budget | BM_LLM_PromptLatency |
| LLM-3 | LoRA load/apply/remove path remains within baseline budget | BM_LoRA_Load, BM_LoRA_Apply, BM_LoRA_Remove |
| LLM-4 | End-to-end inference path remains within baseline budget | BM_LLM_EndToEnd |
| LLM-5 | Cache hit/miss/mixed workload regressions remain bounded | BM_CacheGetExactHit, BM_CacheGetMiss, BM_CacheMixedWorkload |
| LLM-6 | RAID routing/fan-out overhead remains bounded | BM_DomainRouting_OverheadPerRequest, BM_BatchFanOut_LatencyScaling |
| LLM-7 | Hybrid retriever path remains within baseline budget | BM_HybridRetriever_BM25Baseline, BM_HybridRetriever_VectorizerPath |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| LG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| LG-2 | Prompt latency p99 <= release threshold | p99 from BM_LLM_PromptLatency |
| LG-3 | Routing overhead p99 <= release threshold | p99 from BM_DomainRouting_OverheadPerRequest |
| LG-4 | No benchmark case missing in mapped release run | benchmark run manifest completeness |

## Validation
- Expectations are considered met when mapped benchmarks run reproducibly in release profile and stay within configured thresholds.
- For proxy-only targets, follow-up benchmark hardening tasks must remain tracked.

## Sourcecode Verification (Module: llm/performance)

- Verified benchmark sources:
  - benchmarks/bench_llm_inference_performance.cpp
  - benchmarks/bench_llm_infrastructure.cpp
  - benchmarks/bench_llm_response_cache.cpp
  - benchmarks/bench_llm_raid_pipeline.cpp
  - benchmarks/bench_rag_hybrid_retriever.cpp
- Verified mapping surfaces:
  - inference throughput and prompt latency (BM_LLM_*)
  - cache behavior (BM_Cache*)
  - routing and fan-out overhead (BM_DomainRouting_*, BM_BatchFanOut_*)
  - retrieval path overhead (BM_HybridRetriever_*)
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparison.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
