---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "benchmarks/performance/CMakeLists.txt"
  - "benchmarks/performance/bench_performance_release_gates.cpp"
  - "benchmarks/performance/bench_random_access_prefetch.cpp"
  - "tests/performance/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Expectations

## Scope

This file maps the benchmark sources currently present under
`benchmarks/performance/` and states what evidence they provide. It does not
assert module-wide performance guarantees. A benchmark source, CMake target, or
test name is not evidence that a benchmark was run or passed.

## Registered benchmark target

`benchmarks/performance/CMakeLists.txt` returns without defining a target when
`THEMIS_BUILD_BENCHMARKS` is off. When enabled, it registers target
`bench_performance_release_gates` from
`bench_performance_release_gates.cpp` using the repository's standard benchmark
helper. The helper applies release compile flags only when
`CMAKE_BUILD_TYPE` is `Release`.

| Source ID | Benchmark case | What the source measures |
|---|---|---|
| GATE-PFM-01 | `BM_CacheStats_Compute` | Local computation of a `CacheStats` hit-rate value |
| GATE-PFM-02 | `BM_PoolAcquireResult_Construct` | Default result construction and boolean assignment |
| GATE-PFM-03 | `BM_PerfError_Cast` | Cast throughput across the seven `PerfError` values |
| GATE-PFM-04 | `BM_CostEstimate_Arithmetic` | Arithmetic on `CostEstimate` values |
| GATE-PFM-05 | `BM_CacheStats_BatchAlloc` | Vector allocation and initialization for 1,000 `CacheStats` entries |
| GATE-PFM-06 | `BM_PoolAcquireResult_Batch` | Vector allocation and initialization for 1,000 result entries |

Each registered case uses five repetitions. Deterministic seeds are used for
the cases that generate random values. These microbenchmarks do not exercise
end-to-end query, storage, distributed, or vector-search workloads and the C++
source contains no numeric pass/fail thresholds or baseline comparison.

## Present but not registered

`bench_random_access_prefetch.cpp` contains random-access, batch-read, prefix
scan, and prefetch cases, but `benchmarks/performance/CMakeLists.txt` does not
add this source to a target. Do not treat those cases as part of the registered
performance benchmark run.

## Tests and reproducibility

The test module CMake file registers only root-level `test_*.cpp` files.
Nested `tests/performance/phase3/` and `phase4/` sources are not included by
that glob. Inspect tests discovered in the configured build before reporting
coverage. To configure and build the registered benchmark target:

```bash
cmake -S . -B <build-dir> -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build <build-dir> --target bench_performance_release_gates
```

For any release comparison, record the exact revision, build configuration,
benchmark command/output, host, workload, and baseline. No release baseline or
threshold value is specified by the current module benchmark source; if a
release policy requires thresholds, link its canonical policy and its evidence
instead of inferring a limit here.

Exact model backend not verifiable from final document text alone.
