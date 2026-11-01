---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Utils Performance Expectations

## Current registered benchmark

`benchmarks/utils/CMakeLists.txt` registers only
[`bench_utils_release_gates.cpp`](../../benchmarks/utils/bench_utils_release_gates.cpp)
as `bench_utils_release_gates`. Its cases measure `BloomFilterConfig`
construction/batch construction, `RetryPolicy` copy/batch construction, and
small `UtilsError` cast/switch operations. These are narrow microbenchmarks;
they do not establish end-to-end or production-path latency for utility
subsystems.

Other sources in `benchmarks/utils/` are not registered by that CMake file.
In particular, `phase_a3_benchmark_runner.cpp` uses simulated work rather than
the production privacy, compression, rate-limiting, thread-pool, HKDF, or key
rotation implementations.

## Interpretation and gap

- The historical numbers in
  [`benchmarks/utils/BENCHMARK_BASELINE.md`](../../benchmarks/utils/BENCHMARK_BASELINE.md)
  are not a current release baseline and must not be used to claim production
  performance or regression compliance.
- No current registered utils benchmark in this directory measures p95/p99
  for privacy scanning, compression, cryptographic operations, or concurrent
  runtime services.
- TODO: add representative production-path benchmarks, define the workload
  and hardware/compiler environment, and establish reproducible thresholds
  before describing these subsystems as release-gated.

## Validation

Configure a preset with `THEMIS_BUILD_BENCHMARKS=ON`, build
`bench_utils_release_gates`, and record the compiler, build type, host, and
benchmark command with any reported result. A microbenchmark result does not
replace subsystem-specific tests or measurements.
