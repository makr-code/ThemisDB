---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
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
`THEMIS_BUILD_BENCHMARKS` is enabled. It registers one executable target,
`bench_performance_release_gates`, from
[`bench_performance_release_gates.cpp`](bench_performance_release_gates.cpp).
That source defines six `GATE-PFM-01`–`GATE-PFM-06` microbenchmark cases for
types in `performance_api_contract.h`; the names do not imply configured
thresholds or a passing release gate.

[`bench_random_access_prefetch.cpp`](bench_random_access_prefetch.cpp) is
present in this directory but is not included by this CMake file. It is not
part of the registered module benchmark target.

Configure and build the registered target with the repository's normal
dependencies:

```bash
cmake -S . -B <build-dir> -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build <build-dir> --target bench_performance_release_gates
```

The standard benchmark helper applies release compile flags only for
`CMAKE_BUILD_TYPE=Release`. Record host, revision, build flags, command, output,
and comparison baseline before making performance claims. See
[`../../src/performance/PERFORMANCE_EXPECTATIONS.md`](../../src/performance/PERFORMANCE_EXPECTATIONS.md).

Exact model backend not verifiable from final document text alone.
