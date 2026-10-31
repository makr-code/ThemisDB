---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/exporters/CMakeLists.txt; benchmarks/CMakeLists.txt; benchmarks/exporters/bench_exporters.cpp; benchmarks/exporters/bench_exporters_release_gates.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter benchmarks

Exact model backend not verifiable from final document text alone.

## Registered targets

`benchmarks/exporters/CMakeLists.txt` returns without adding targets when
`THEMIS_BUILD_BENCHMARKS` is off. When enabled, it registers:

- `bench_exporters` from `bench_exporters.cpp`: JSONL batch/template/compression,
  streaming, incremental, and Parquet/CSV cases.
- `bench_exporters_release_gates` from `bench_exporters_release_gates.cpp`:
  six ERRG serialization/schema/null/quota cases.
- `bench_exporters_dedicated_gates` from `bench_exporters_dedicated_gates.cpp`
  when the source exists and the target has not already been defined; these
  cases cover metric/trace export throughput and queue latency.

The directory-local CMake file does not register `bench_csv_export.cpp` or
`bench_parquet_export.cpp`. The root `benchmarks/CMakeLists.txt` registers
those files separately as `bench_csv_export` and `bench_parquet_export` when
their sources exist and benchmarks are enabled.

## Build and run

Configure with a benchmark-enabled preset, for example:

```sh
cmake --preset community-debug -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset community-debug --target bench_exporters
```

Run the produced benchmark executable with the desired Google Benchmark
arguments and record the build configuration, hardware, and baseline. Target
registration alone is not evidence that a benchmark was built or executed.
