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
  - "benchmarks/CMakeLists.txt"
  - "benchmarks/search/bench_search_release_gates.cpp"
  - "benchmarks/search/bench_layered_retrieval_phase5.cpp"
  - "tests/search/CMakeLists.txt"
Related_Issue: "#7378"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Search Benchmark Sources and Build Registration

This page inventories current search benchmark definitions and their CMake registration. It is not a report of benchmark runs or measured release performance.

## Current CMake registration

When `THEMIS_BUILD_BENCHMARKS` is enabled, `benchmarks/search/CMakeLists.txt` declares these standard benchmark targets:

- `bench_search_release_gates`
- `bench_advanced_scenarios`
- `bench_distributed_merge_gates`
- `bench_hybrid_fusion_gates`
- `bench_layered_retrieval_phase5`
- `bench_vector_search_gates`
- `bench_search_dedicated_gates`

It conditionally registers CTest smoke entries for `bench_layered_retrieval_phase5` and `bench_search_dedicated_gates` only when those targets exist. The root `benchmarks/CMakeLists.txt` separately declares `benchmark_hybrid_search` and `benchmark_distributed_hybrid_search` from the sources in this directory.

## Source cases

- `bench_search_release_gates.cpp`: SRCP-1 through SRCP-6 and SRCP-ADV-1 through SRCP-ADV-3.
- `bench_search_dedicated_gates.cpp`: FTS query, ranking, facet-filter, and concurrent-throughput cases.
- `bench_layered_retrieval_phase5.cpp`: layered retrieval and fallback cases.
- `benchmark_hybrid_search.cpp`: RRF, linear fusion, score normalization, and config construction.
- `benchmark_distributed_hybrid_search.cpp`: shard-count, result-count, overlap, k-limit, failure, and config-construction cases.
- The remaining registered source files define the advanced-scenario, distributed-merge, hybrid-fusion, and vector-gate functions. Their existence does not establish runtime results.

## Build and validation

Configure a build with `THEMIS_BUILD_BENCHMARKS=ON`, then build a named target using that build directory's CMake build command. Repository release presets inspected for issue #7378 disable benchmarks by default, so do not assume those presets enable these targets.

No measured baseline, gate pass/fail, hardware profile, or timing value is established here. Older SRCP thresholds and hardware figures previously listed in this README are retained only as historical planning context and are **not verified results**. **TODO:** attach the reproducible run command, build configuration, hardware details, and result artifact before publishing measured claims.

The local benchmark CMake file's CTest entries are smoke registrations, not evidence that a benchmark was executed in this checkout.
