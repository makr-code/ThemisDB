# Distributed Tensor Module Performance Expectations

**Last Updated:** 2026-10-31
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `benchmarks/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `benchmarks/distributed_tensor/bench_distributed_tensor_dedicated_gates.cpp`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `src/distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

## Current evidence

`benchmarks/distributed_tensor/CMakeLists.txt` conditionally registers sources
matching `bench_*.cpp`. The current source,
`bench_distributed_tensor_dedicated_gates.cpp`, defines four local stub gates:

| Gate | Source comment threshold | Limitation |
|---|---:|---|
| DT-BM-01 sharded operation dispatch | `≤ 1 µs` per call | In-process stub |
| DT-BM-02 tensor sync round-trip | `≤ 10 µs` per cycle | In-process stub |
| DT-BM-03 partition routing decision | `≤ 1 µs` per call | In-process stub |
| DT-BM-04 concurrent sharded operations | `≥ 500 ops/s` with 8 workers | In-process stub |

The benchmark source explicitly says it does not require external network shards
or GPU backends and must not be used in production code paths. These thresholds
are source-defined targets, not measured results.

`tests/distributed_tensor/test_tensor_performance.cpp` is registered as a
focused test executable; it is not the benchmark registration. No performance
measurements, representative-hardware baselines, or SLO results are established
by the checked-in source inventory or this documentation review.

## Other configured benchmark sources

The parent `benchmarks/CMakeLists.txt` also adds
`benchmarks/epic3_distributed_tensor/` when its CMake file exists. That file
configures four standard benchmark sources (placement, integrity, recovery,
distributed retrieval) and two additional targets (partial refit and
summary-first routing) when enabled. `tests/epic3_distributed_tensor/` also
defines an integrity-verification benchmark test target. These configured
sources and targets establish benchmark availability, not pass status or
measured results.

The numeric EPIC 3 workload thresholds documented in older module files are
planning criteria unless paired with recorded benchmark output. The dedicated
local benchmark's stub thresholds likewise do not establish production
performance.

## Follow-up evidence needed

- Record benchmark output, hardware and build configuration for any reported
  result.
- Use representative distributed and accelerator infrastructure before
  extrapolating beyond the in-process stubs.
- Keep performance claims explicitly separate from source-level gate comments.
