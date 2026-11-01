---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-11-01"
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
`THEMIS_BUILD_BENCHMARKS` is off. When enabled, it registers `bench_performance_release_gates` and
`bench_random_access_prefetch` using the repository's standard benchmark
helper.
The helper applies release compile flags only when `CMAKE_BUILD_TYPE` is
`Release`.

| Source ID | Benchmark case | What the source measures |
|---|---|---|
| GATE-PFM-01 | `BM_CacheStats_Compute` | Local computation of a `CacheStats` hit-rate value |
| GATE-PFM-02 | `BM_PoolAcquireResult_Construct` | Default result construction and boolean assignment |
| GATE-PFM-03 | `BM_PerfError_Cast` | Cast throughput across the seven `PerfError` values |
| GATE-PFM-04 | `BM_CostEstimate_Arithmetic` | Arithmetic on `CostEstimate` values |
| GATE-PFM-05 | `BM_CacheStats_BatchAlloc` | Vector allocation and initialization for 1,000 `CacheStats` entries |
| GATE-PFM-06 | `BM_PoolAcquireResult_Batch` | Vector allocation and initialization for 1,000 result entries |

Each GATE-PFM case uses five repetitions. Deterministic seeds are used for the
cases that generate random values. These microbenchmarks do not exercise
end-to-end query, storage, distributed, or vector-search workloads and the C++
source contains no numeric pass/fail thresholds or cross-run baseline
comparison.

## Random-access and prefetch cases

`bench_random_access_prefetch.cpp` is registered as
`bench_random_access_prefetch`. It includes paired prefetch-disabled/enabled
point-read, batch-read, and prefix-scan cases, as well as standalone prefetch
overhead/range and batch-scaling cases. The paired results are not reduced to
an on/off ratio by this executable, and its source comments' target values do
not cause a pass/fail result.

## Tests and reproducibility

The test module CMake file registers root-level tests and recursively includes
`tests/performance/phase3/` and `phase4/`; nested target and test names include a
stable hash of the relative source path to prevent collisions. Inspect tests
discovered in the configured build before reporting coverage. To configure
and build the registered benchmark targets:

```bash
cmake -S . -B <build-dir> -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build <build-dir> --target bench_performance_release_gates bench_random_access_prefetch
```

No measured performance-module baseline or numeric threshold is currently
available. Establish one only from repeated benchmark runs on declared
representative hardware; retain the exact revision, build configuration,
command/output, host, workload, and raw results. Until that evidence exists,
the repository expectation comparator has no defensible numeric limit for
this module, and the benchmarks are measurements rather than enforced release
gates.

Exact model backend not verifiable from final document text alone.
