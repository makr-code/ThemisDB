---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "benchmarks/search/CMakeLists.txt"
  - "benchmarks/search/benchmark_hybrid_search.cpp"
  - "benchmarks/search/benchmark_distributed_hybrid_search.cpp"
  - "benchmarks/search/bench_search_release_gates.cpp"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/search

## Scope

- Module: src/search
- This file defines measurable search module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/search/benchmark_hybrid_search.cpp`
  - `benchmarks/search/benchmark_distributed_hybrid_search.cpp`
  - `benchmarks/search/bench_vector_search_gates.cpp`
  - `benchmarks/search/bench_search_release_gates.cpp`

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SRCP-1 | hybrid fusion and score normalization paths remain bounded | BM_RRF_BM25Only, BM_RRF_VectorOnly, BM_RRF_Hybrid_NoOverlap, BM_RRF_Hybrid_50PctOverlap, BM_RRF_Hybrid_FullOverlap, BM_Linear_Hybrid, BM_NormalizeScores_BM25, BM_NormalizeScores_Vector, BM_RRF_VaryingRrfK, BM_ConfigConstruction |
| SRCP-2 | distributed shard merge paths remain bounded under overlap/failure/k-limit variation | BM_MergeShardResults_ShardCount, BM_MergeShardResults_ResultsPerShard, BM_MergeShardResults_Overlap, BM_MergeShardResults_KLimit, BM_MergeShardResults_WithFailures |
| SRCP-3 | vector-search-sensitive retrieval building blocks remain bounded | Current vector-gate functions: `BM_VectorSearchL2`, `BM_VectorSearchCosineBatch`; the former list of `efSearch`, insert, and TopK cases is not present in the current scoped benchmark source. |

## Module Hard Gates (proposed thresholds; execution evidence required)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SRCG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SRCG-2 | search hot-path p99 <= release threshold | p99 from mapped search benchmark cases |
| SRCG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Build, registration, and validation status

- `benchmarks/search/CMakeLists.txt` currently declares seven standard benchmark targets, including `bench_search_release_gates`, `bench_layered_retrieval_phase5`, and `bench_search_dedicated_gates`. It registers two benchmark smoke tests conditionally when the corresponding targets exist.
- The two `benchmark_hybrid_search` and `benchmark_distributed_hybrid_search` targets are separately declared in `benchmarks/CMakeLists.txt`; the source cases are not evidence that a benchmark was executed.
- The SRCP tables above are expectations only. No baseline run, measured p95/p99, or passing release gate is established by this document or by inspecting benchmark definitions.
- **TODO:** document the current release-run manifest and measured baseline source before claiming any threshold passed; keep expectations distinct from results.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
