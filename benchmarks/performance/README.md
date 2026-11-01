---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-11-01"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "benchmarks/performance/CMakeLists.txt"
  - "benchmarks/CMakeLists.txt"
  - "benchmarks/performance/bench_performance_release_gates.cpp"
  - "benchmarks/performance/bench_random_access_prefetch.cpp"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Benchmarks

`benchmarks/performance/CMakeLists.txt` is active only when
`THEMIS_BUILD_BENCHMARKS` is enabled. It registers two executable targets:
`bench_performance_release_gates`, from
[`bench_performance_release_gates.cpp`](bench_performance_release_gates.cpp).
That source defines six `GATE-PFM-01`–`GATE-PFM-06` microbenchmark cases for
types in `performance_api_contract.h`; the names do not imply configured
thresholds or a passing release gate.

`bench_random_access_prefetch` is built from
[`bench_random_access_prefetch.cpp`](bench_random_access_prefetch.cpp). It
contains paired prefetch-disabled/enabled point-read, batch-read, and prefix-scan
cases, plus prefetch-overhead, prefetch-range, and batch-scaling cases. The
paired cases produce measurements, but the benchmark does not calculate an
on/off ratio or enforce a regression threshold. The source's target figures are
comments, not executable acceptance criteria.

Configure and build the registered target with the repository's normal
dependencies:

```bash
cmake -S . -B <build-dir> -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build <build-dir> --target bench_performance_release_gates bench_random_access_prefetch
```

The standard benchmark helper applies release compile flags only for
`CMAKE_BUILD_TYPE=Release`. Record host, revision, build flags, command, output,
and comparison baseline before making performance claims. No performance-module
baseline or numeric threshold is committed: do not infer one from source
comments or from a different host. A defensible gate requires repeated
measurements on declared representative hardware. See
[`../../src/performance/PERFORMANCE_EXPECTATIONS.md`](../../src/performance/PERFORMANCE_EXPECTATIONS.md).

Exact model backend not verifiable from final document text alone.
