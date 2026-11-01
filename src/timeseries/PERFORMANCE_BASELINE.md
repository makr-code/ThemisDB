---
Author: "ThemisDB Team"
Created: "2026-08-07"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/timeseries/CMakeLists.txt; benchmarks/timeseries/bench_timeseries_release_gates.cpp; benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp; benchmarks/timeseries/bench_timeseries_ingestion.cpp; benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries benchmark inventory and evidence limits

This page documents benchmark sources present in the repository. It does not
record a current hardware baseline or establish that any performance target has
passed. The prior version contained measurements, machine assumptions,
configuration variables, and release sign-off claims that were not supported
by the inspected benchmark implementations; those claims are withdrawn.

## Registered sources

| Source | What the source measures |
|---|---|
| `bench_timeseries_ingestion.cpp` | Benchmark fixture cases for raw/batch writes, multiple metrics, Gorilla helpers, range query, downsampling, and out-of-order writes. Inspect the fixture and operation for the exact path under measurement. |
| `bench_timeseries_adaptive_flush.cpp` | Adaptive-buffer fixture cases, stats exposure, batch-watermark behavior, and a standalone flush-controller case. |
| `bench_timeseries_release_gates.cpp` | Six `TSRG-01`–`TSRG-06` microbenchmarks. The source uses in-memory helper data; its Gorilla encode/decode case is a `memcpy` bit-copy helper, not the module's Gorilla codec. |
| `bench_timeseries_dedicated_gates.cpp` | Four `TS-BM-01`–`TS-BM-04` cases over in-process ingest/query/flush/remote-write stubs. It does not execute those production components. |

`benchmarks/timeseries/CMakeLists.txt` discovers `bench_*.cpp` only when
`THEMIS_BUILD_BENCHMARKS` is enabled. The repository-level benchmark CMake file
also has explicit registrations for some timeseries targets. For example, in
an already configured benchmark build:

```bash
cmake --build <build-dir> --target bench_timeseries_release_gates
```

## Interpreting targets and results

The `TSRG-*` and `TS-BM-*` labels and thresholds are comments/labels in the
benchmark sources. They are not, by themselves, evidence of a CI gate, a
measured baseline, or a release pass. A valid comparison requires an executed
benchmark result with the build configuration, compiler, host, and run
parameters recorded; no such result is established here.

For module behavior, use the relevant implementation and tests. Do not equate
the in-memory/stub benchmark cases with persisted TSStore throughput, remote
write transport, or end-to-end production latency. No benchmark was built or
run for this documentation task.
