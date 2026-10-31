---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/timeseries/CMakeLists.txt; benchmarks/timeseries/bench_*.cpp; benchmarks/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries benchmarks

This directory contains four Google Benchmark sources:

- `bench_timeseries_adaptive_flush.cpp`
- `bench_timeseries_dedicated_gates.cpp`
- `bench_timeseries_ingestion.cpp`
- `bench_timeseries_release_gates.cpp`

`CMakeLists.txt` returns without registering targets unless
`THEMIS_BUILD_BENCHMARKS` is enabled. When enabled, it discovers `bench_*.cpp`
and registers the target named from each source stem. The repository-level
`benchmarks/CMakeLists.txt` also contains explicit registrations for some
timeseries targets.

Build an available target after configuring a benchmark-enabled build:

```bash
cmake --build <build-dir> --target bench_timeseries_ingestion
```

Read each benchmark source before treating a result as module performance
evidence. In particular, `bench_timeseries_release_gates.cpp` measures local
in-memory helper operations (including a bit-copy stand-in for Gorilla), and
`bench_timeseries_dedicated_gates.cpp` uses in-process stubs. Their labels and
threshold comments are not measurements of end-to-end TSStore behavior or proof
that a release gate passed.

`src/timeseries/PERFORMANCE_BASELINE.md` records the limits of the current
benchmark evidence. No benchmark was built or run for this documentation task.
