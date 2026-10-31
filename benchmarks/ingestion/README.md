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
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/ingestion/CMakeLists.txt; benchmarks/ingestion/bench_ingestion_release_gates.cpp; benchmarks/ingestion/bench_ingestion_dedicated_gates.cpp; src/ingestion/PERFORMANCE_EXPECTATIONS.md"
---

> Exact model backend not verifiable from final document text alone.

# Ingestion benchmarks

This directory contains Google Benchmark sources for ingestion key-value
operations, extraction/adapter helpers, quality-judge paths, synthetic release
gate cases, and dedicated IN-BM cases. It is not a mirror of
`src/ingestion/`.

## Build registration

`benchmarks/ingestion/CMakeLists.txt` returns without registering targets unless
`THEMIS_BUILD_BENCHMARKS` is enabled. It registers
`bench_ingestion_extraction`, `bench_ingestion_kv`,
`bench_ingestion_quality_judge`, and `bench_ingestion_release_gates`. The
`bench_ingestion_dedicated_gates` target is added only when its source exists.
Configure/build commands depend on the repository's selected CMake preset and
dependency setup; no successful build is claimed here.

## Interpreting results

The INRG source uses in-memory/mock write, serialization, validation, buffer,
back-pressure, and quota operations. Its labels/thresholds do not establish
database durability, connector throughput, end-to-end capacity, or that a
release CI job enforced/passed the threshold. IN-BM results likewise require
inspection of each benchmark body and a recorded run before being used as a
production baseline. See [`../../src/ingestion/PERFORMANCE_EXPECTATIONS.md`](../../src/ingestion/PERFORMANCE_EXPECTATIONS.md).
