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

The local benchmark build is gated by `THEMIS_BUILD_BENCHMARKS` (default `OFF` in the root
`CMakeLists.txt`). `benchmarks/sharding/CMakeLists.txt` discovers `bench_*.cpp`; CMake may also
declare targets in the parent benchmark file. Confirm target availability in the chosen build.

## Test evidence boundary

Relevant correctness suites are registered in `tests/sharding/CMakeLists.txt`, including the
sharding core, contract-hardening, multi-shard exact, and thread-safety/lock-order tests. Test
registration is not a test result. No benchmark or test was run as part of this documentation
audit.

## Baseline status

The documents in `../../benchmarks/sharding/` contain a dated 2026-08-17 baseline snapshot. Their
measurements and p95/p99 estimates are historical and have not been revalidated against the current
build here. **TODO:** recapture the benchmark targets from a documented build configuration before
using any numeric results as current release expectations. Do not extrapolate the in-memory SRG
cases to network latency, persistent WAL durability, or multi-node availability.
