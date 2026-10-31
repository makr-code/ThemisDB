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

Exact model backend not verifiable from final document text alone.

# Analytics benchmarks

The analytics benchmark sources are registered through
[`CMakeLists.txt`](./CMakeLists.txt), the parent
[`benchmarks/CMakeLists.txt`](../CMakeLists.txt), and repository CMake. The
registered target list below reflects those files as checked on 2026-10-31.

## Registered targets

| Target | Source | Scope |
|---|---|---|
| `bench_diff_engine` | `bench_diff_engine.cpp` | Diff-engine benchmark fixture |
| `bench_streaming_window` | `bench_streaming_window.cpp` | Streaming window operations |
| `bench_analytics_release_gates` | `bench_analytics_release_gates.cpp` | Contract-focused benchmark cases |
| `bench_analytics_distributed_coordinator` | `bench_analytics_distributed_coordinator.cpp` | Distributed coordinator operations |
| `bench_analytics_gap_closure` | `bench_analytics_gap_closure.cpp` | Self-contained gap-closure microbenchmarks |
| `bench_analytics_dedicated_gates` | `bench_analytics_dedicated_gates.cpp` | Self-contained stub/microbenchmark workloads |
| `bench_analytics_operability_paths` | `bench_analytics_operability_paths.cpp` | Export, serving, streaming, and distributed API paths |
| `bench_analytics_critical_paths_focused` | `bench_analytics_critical_paths_focused.cpp` | Self-contained critical-path microbenchmarks |
| `bench_olap_analytics` | `bench_olap_analytics.cpp` | OLAP-shaped benchmark workloads |
| `bench_olap_performance` | `bench_olap_performance.cpp` | OLAP API benchmark cases |

The presence of a target is not evidence that it has been run, passed a
performance threshold, or been included in a release gate. Several targets
measure local stand-ins rather than production implementations; consult the
source before using a result as an application-performance claim.

## Build and run

Configure with the repository option that enables benchmark targets, then build
one or more targets:

```bash
cmake -S . -B build-analytics-bench \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-analytics-bench --target \
  bench_streaming_window bench_analytics_operability_paths
```

The repository places runtime binaries under `<build-dir>/bin/`. For example:

```bash
./build-analytics-bench/bin/bench_streaming_window --benchmark_list_tests
./build-analytics-bench/bin/bench_streaming_window \
  --benchmark_filter='BM_TumblingWindow_IngestThroughput.*' \
  --benchmark_format=json
```

Use the CMake preset/build configuration selected for the local dependency and
platform requirements. For the current case list, run the target with
`--benchmark_list_tests`; do not rely on the historical Phase 5 counts in older
reports.
