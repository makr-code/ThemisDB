---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - CMakeLists.txt
  - bench_sharding_release_gates.cpp
  - bench_sharding_performance.cpp
  - ../../benchmarks/CMakeLists.txt
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# Sharding benchmarks

This directory contains Google Benchmark programs for selected sharding components. Benchmark
results are measurements of the named code paths and harness configuration; they are not by
themselves evidence of multi-node behavior, durable I/O, release readiness, or a passing CI gate.

## Sources and coverage

| Source | Current registered cases / focus |
|---|---|
| `bench_distributed_coordinator.cpp` | Coordinator election, task scheduling/state, and JSON serialization |
| `bench_gossip_config.cpp` | Vector-clock operations and config/resource-snapshot serialization |
| `bench_locality_aware_router.cpp` | Query routing, affinity, placement update, data presence, multi-shard query |
| `bench_shard_resource_manager.cpp` | Snapshot, query admission, peer lookup, health score, serialization |
| `bench_shard_routing.cpp` | Connection-pool hit rate and distribution quality |
| `bench_sharding_performance.cpp` | Percolator commit and concurrent shard access cases |
| `bench_sharding_release_gates.cpp` | SRG-01..SRG-06 microbenchmarks (hash lookup, 2PC state operations, in-memory WAL append, health lookup) |
| `bench_sharding_dedicated_gates.cpp` | Dedicated gate executable; inspect this source for its gate checks |

The precise measured work is defined in each benchmark source. In particular, the SRG 2PC/WAL
cases are microbenchmarks and do not measure network round trips or durable `fsync`.

## Build and configuration

`benchmarks/CMakeLists.txt` adds this directory when present. Its local `CMakeLists.txt` returns
unless `THEMIS_BUILD_BENCHMARKS` is enabled, then discovers `bench_*.cpp`; the dedicated-gates
source is added as an executable while the other sources use the repository's standard benchmark
helper. Use the repository CMake preset/build configuration with that option enabled, then build
the desired `bench_*` target. Exact target availability can depend on configuration and optional
dependencies.

## Test and evidence guidance

Tests are separately registered in `tests/sharding/CMakeLists.txt`; benchmarks do not replace those
tests. For reproducible comparisons, record the executable/source revision, compiler and build
type, benchmark arguments, hardware, and whether the operation uses mocks, in-memory data, RPC, or
persistent storage.

The files [`README_BASELINE.md`](README_BASELINE.md), [`WAVE_A_BASELINE_REPORT.md`](WAVE_A_BASELINE_REPORT.md),
and [`BASELINE_INDEX.md`](BASELINE_INDEX.md) retain a dated 2026-08-17 baseline snapshot. Treat
those figures as historical measurements, not as a current benchmark run or proof that current
release gates passed.
