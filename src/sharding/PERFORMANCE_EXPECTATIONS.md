---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../benchmarks/sharding/CMakeLists.txt
  - ../../benchmarks/sharding/bench_sharding_release_gates.cpp
  - ../../benchmarks/CMakeLists.txt
  - ../../tests/sharding/CMakeLists.txt
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# Sharding performance expectations

## Scope

This page maps sharding benchmark sources and the gate expectations declared by the current
benchmark code. A benchmark registration or threshold comment is not evidence that the benchmark
was run, passed in CI, or represents end-to-end distributed performance.

## Current benchmark mapping

| Source / case | Measured path | Scope limitation |
|---|---|---|
| `bench_sharding_release_gates.cpp`: `BM_SRG01_ConsistentHashRouting1K`, `BM_SRG06_RouteLookup64ShardWarm` | Stub hash-ring lookup; SRG-01 and SRG-06 label p99 expectations of 50 µs and 20 µs respectively | In-process stub, warm lookup |
| `BM_SRG02_TwoPhaseCommitPrepare`, `BM_SRG03_TwoPhaseCommitCommit` | Single in-memory mock participant; labels 1 ms and 500 µs | Not networked 2PC |
| `BM_SRG04_WalAppend` | In-memory log append; label 100 µs | No persistent storage or `fsync` |
| `BM_SRG05_ShardHealthCheck` | Local in-memory health lookup; label 50 µs | No network probe |
| `bench_shard_routing.cpp` | `BM_ConnectionPoolHitRate`, `BM_DistributionQuality` | See source for fixture and measured operation |
| `bench_sharding_performance.cpp` | `BM_PercolatorCommitLatency`, `BM_ConcurrentShardAccess` | See source for implementation and configuration |
| Other sharding benchmark files | Coordinator, gossip/config, locality-router, and resource-manager cases | These are component microbenchmarks; no universal deployment SLO is asserted here |
| `bench_sharding_dedicated_gates.cpp`: SH-BM-01..04 | Local route hash, atomic-counter commit stub, no-op migration stub, and CPU-loop anti-entropy stub | No production sharding components, RPC, storage, or durable WAL are exercised |

The local benchmark build is gated by `THEMIS_BUILD_BENCHMARKS` (default `OFF` in the root
`CMakeLists.txt`). `benchmarks/sharding/CMakeLists.txt` discovers `bench_*.cpp`; CMake may also
declare targets in the parent benchmark file. Confirm target availability in the chosen build.

## Test evidence boundary

Relevant correctness suites are registered in `tests/sharding/CMakeLists.txt`, including the
sharding core, contract-hardening, multi-shard exact, and thread-safety/lock-order tests. Test
registration is not a test result. No sharding test or CI run was executed or checked for this
follow-up.

## Baseline status

The SRG-01..06 documents in `../../benchmarks/sharding/` contain a dated 2026-08-17 baseline
snapshot. Those measurements and p95/p99 estimates remain historical. On 2026-11-01 the standalone
SH-BM-01..04 harness was compiled directly using
`c++ -O3 -DNDEBUG -std=c++20 -pthread` and run on a 4-vCPU virtualized AMD EPYC 9V74 host:

| Case | Observed result | Interpretation |
|---|---:|---|
| SH-BM-01 route p95 | 0.03 µs | Hash stub only |
| SH-BM-02 commit p95 | 0.03 µs | Atomic-counter stub only |
| SH-BM-03 migration throughput | 1,845,699,520 ops/s | No-op migration stub only |
| SH-BM-04 anti-entropy throughput | 1,844,337,883 rounds/s | Small CPU loop only |

This is a fresh harness execution, not a production or representative-hardware baseline; it does not
refresh SRG-01..06 or establish runtime performance. The normal CMake Release benchmark configure
was attempted with `THEMIS_BUILD_BENCHMARKS=ON`, but stopped because system RocksDB was unavailable,
so the Google Benchmark release-gate executable could not be built. **TODO:** rerun the registered
SRG targets and capture the build configuration, raw output, and host manifest before using numeric
results as release expectations. Do not extrapolate in-memory SRG cases to network latency,
persistent WAL durability, or multi-node availability.
