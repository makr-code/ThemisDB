---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Performance Expectations — Vector Search

## Scope

This document records the repository's vector-search benchmark thresholds and
the limits of their current evidence. It does not publish measured production
baselines or certify a release.

## Benchmark references

- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` —
  synthetic flat-index gate fixture.
- `benchmarks/ann/bench_vector_search.cpp` — benchmark using
  `VectorIndexManager`.
- `benchmarks/search/bench_vector_search_gates.cpp` — search benchmark suite.
- `tests/integration/test_vector_search_soak.cpp` and
  `tests/vector_search/test_vector_search_highcardinality_stress.cpp` —
  in-process flat-index fixtures, not production runtime benchmarks.

## Documented gate thresholds

| Gate | Documented threshold | Current evidence limitation |
|---|---|---|
| VS-BM-01 | Insert throughput ≥ 1,000 ops/sec | Dedicated benchmark uses `GateFlatIndex`; its counter records the threshold, not a production measurement. |
| VS-BM-02 | k=10 query latency ≤ 10 ms for 128 dimensions | Dedicated benchmark uses `GateFlatIndex`; it does not establish production-runtime P95/P99. |
| VS-BM-03 | Build time for 1,000 vectors | Dedicated benchmark builds a flat index, not HNSW; no portable numeric threshold is defined. |
| VS-BM-04 | Four-thread search throughput ≥ 500 ops/sec | Dedicated benchmark uses `GateFlatIndex`; the threshold counter is not a measured runtime result. |

These are targets to validate, not statements that the production runtime has
met them. Historical values such as 8.2 ms P99, 1,800 ops/sec, and 31% memory
overhead are not retained because the current checked-in evidence does not
substantiate them.

## Validation requirements

A release-quality measurement must:

1. exercise the production `VectorIndexManager` runtime and identify its
   backend;
2. record hardware, vector count, dimension, workload, and sample count;
3. calculate and report the required percentile from measured samples;
4. compare measured values with the approved threshold and retain the output;
5. distinguish setup, index-build, and query time.

Until that evidence exists, no current measured baseline or performance pass is
asserted here. See [module gaps](MODULE_GAPS.md).
