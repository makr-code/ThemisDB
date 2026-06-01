# PERFORMANCE_EXPECTATIONS - src/search

## Scope

- Module: src/search
- This file defines measurable search module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/benchmark_hybrid_search.cpp
  - benchmarks/benchmark_distributed_hybrid_search.cpp
  - benchmarks/bench_vector_search.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SRCP-1 | hybrid fusion and score normalization paths remain bounded | BM_RRF_BM25Only, BM_RRF_VectorOnly, BM_RRF_Hybrid_NoOverlap, BM_RRF_Hybrid_50PctOverlap, BM_RRF_Hybrid_FullOverlap, BM_Linear_Hybrid, BM_NormalizeScores_BM25, BM_NormalizeScores_Vector, BM_RRF_VaryingRrfK, BM_ConfigConstruction |
| SRCP-2 | distributed shard merge paths remain bounded under overlap/failure/k-limit variation | BM_MergeShardResults_ShardCount, BM_MergeShardResults_ResultsPerShard, BM_MergeShardResults_Overlap, BM_MergeShardResults_KLimit, BM_MergeShardResults_WithFailures |
| SRCP-3 | vector-search-sensitive retrieval building blocks remain bounded | BM_VectorSearch_efSearch, BM_VectorInsert_Batch100, BM_L2Distance_1000_512, BM_CosineDistance_1000_512, BM_TopK_5000_50 |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SRCG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SRCG-2 | search hot-path p99 <= release threshold | p99 from mapped search benchmark cases |
| SRCG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional search benchmark scenarios are introduced.

## Sourcecode Verification (Module: search/performance)

- Verified benchmark sources:
  - benchmarks/benchmark_hybrid_search.cpp
  - benchmarks/benchmark_distributed_hybrid_search.cpp
  - benchmarks/bench_vector_search.cpp
- Verified mapping surfaces:
  - hybrid fusion, distributed merge, and vector-search building block behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
