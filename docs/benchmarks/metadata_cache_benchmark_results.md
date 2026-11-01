---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Metadata Cache Benchmark Evidence

## Evidence status

There are currently **no validated benchmark measurements** published in this
document. The previous numeric tables had no retained Google Benchmark output,
run ID, commit, or verified machine configuration. They have been removed rather
than presented as measured results.

The scheduled [thematic benchmark workflow](../../.github/workflows/benchmark-thematic-suites.yml)
is configured to run the metadata cache suite by building only `bench_metadata_cache` with
LLM targets disabled. The reusable job installs yaml-cpp, records run/commit and
runner CPU details in `run_metadata.json`, and stores non-empty Google Benchmark
JSON as `results/bench_metadata_cache.json`. A missing or empty result fails the
metadata benchmark gate.

To review actual measurements, download the `benchmark-pipeline-<scenario>-metadata-<run-id>`
artifact from a successful run. Do not copy values into this document without
keeping the matching raw JSON and run metadata. Comparisons are meaningful only
when the commit, runner, compiler/build configuration, benchmark arguments, and
benchmark implementation are identified.

## Benchmark coverage

The `bench_metadata_cache` executable is implemented in
[`bench_metadata_cache.cpp`](../../benchmarks/metadata/bench_metadata_cache.cpp).
It exercises warm cache reads, forced cache refreshes, table lookups, database
metadata aggregation, TTL variants, adaptive TTL, concurrent reads, and a
RocksDB-backed forced scan. These are benchmark scenarios, not measurements or
claims about production performance.

The separate `bench_metadata_release_gates` target is based on the frozen
`metadata_api_contract.h` contract artifact; its output must not be described as
a measurement of the current metadata runtime.

## Local reproduction

Configure a Release build with benchmarks enabled, build the metadata cache
target, locate the resulting executable, and retain its JSON output alongside
the exact commit and machine details:

```bash
cmake -S . -B build/metadata-bench -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_ENABLE_LLM=OFF
cmake --build build/metadata-bench --target bench_metadata_cache
find build/metadata-bench -type f -name bench_metadata_cache -executable -print
```

Run the located executable with
`--benchmark_format=json --benchmark_out=bench_metadata_cache.json`.
No latency or throughput threshold is asserted here until measured evidence is
reviewed and retained.

## References

- [Metadata benchmark registration](../../benchmarks/metadata/CMakeLists.txt)
- [Benchmark suite map](../../benchmarks/benchmark_suite_map.json)
- [Metadata module roadmap](../../src/metadata/ROADMAP.md)
