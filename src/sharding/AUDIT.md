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
  - ../../include/sharding/sharding_api_contract.h
  - ../../include/sharding/shard_router.h
  - ../../tests/sharding/CMakeLists.txt
  - ../../benchmarks/sharding/CMakeLists.txt
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# Sharding documentation/source audit

## Scope and method

Static documentation-to-source inventory review performed 2026-10-31 for the top-level Markdown
files in `src/sharding/`, `include/sharding/`, and `benchmarks/sharding/`. `tests/sharding/` has
no top-level Markdown files. Checked current source/header paths, selected public API declarations,
test and benchmark registrations, and build configuration. No build, test, benchmark, or runtime
validation was performed.

## Source and registration evidence

- Public contract: `include/sharding/sharding_api_contract.h`.
- Router API: `include/sharding/shard_router.h` (`RoutingStrategy`, `ShardResult`,
  `ShardRouter::Config`, `get`, `put`, `del`, `executeQuery`, `analyzeQuery`, `scatterGather`,
  `executeOnShards`, and related methods).
- Relevant implementations include `src/sharding/shard_router.cpp`,
  `cross_shard_transaction.cpp`, `wal_manager.cpp`, `auto_rebalancer.cpp`, and
  `locality_aware_router.cpp`.
- `tests/sharding/CMakeLists.txt` conditionally declares focused targets; registration confirms
  target configuration only, not a passing test result. It includes core, contract-hardening,
  multi-shard exact, and thread-safety/lock-order suites.
- `benchmarks/sharding/CMakeLists.txt` returns unless `THEMIS_BUILD_BENCHMARKS` is enabled and
  discovers `bench_*.cpp`. The SRG benchmark source uses local in-memory/stub implementations for
  several measured paths.

## Audit findings and limits

- The previous report's 2026-07-18 “pass” and “no blockers” entries were execution-era assertions;
  they are not current validation evidence and have been removed from the current status summary.
- The prior module README's universal lock-free, lock-order, failover, quorum, and self-healing
  claims were broader than the inspected component evidence; the README now states those behaviors
  are component-specific.
- Older benchmark baseline documents describe a 2026-08-17 run. They are explicitly marked
  historical; current benchmark execution and release-gate results remain **TODO**.
- Contract declarations describe intended behavior. This audit did not establish runtime
  conformance for every backend or public method. **TODO:** perform targeted implementation/test
  verification when behavior changes.

## Validation status

### Follow-up validation (2026-11-01)

- Ran `bench_sharding_dedicated_gates.cpp` directly with
  `c++ -O3 -DNDEBUG -std=c++20 -pthread`; on this 4-vCPU virtualized AMD EPYC 9V74 host it
  reported SH-BM-01 route p95 0.03 µs, SH-BM-02 stub commit p95 0.03 µs,
  SH-BM-03 stub migration 1,845,699,520 ops/s, and SH-BM-04 stub anti-entropy
  1,844,337,883 rounds/s. These are execution results for the benchmark's local stub code only,
  not sharding runtime, network, durable-WAL, or deployment evidence. The executable was compiled
  directly because the configured CMake build stopped at missing system RocksDB.
- CMake configuration was attempted with Release, benchmarks enabled, tests disabled, and Vulkan
  disabled; configuration failed at `cmake/Dependencies.cmake:660` because RocksDB was unavailable.
  The Google Benchmark release-gate target was therefore not built or run.
- No sharding deployment endpoint, deployment configuration, or runtime telemetry was provided.
  Deployment behavior and security controls remain unverified; no security audit or penetration
  test was performed.
- Current test/benchmark CMake registrations were inspected, but no sharding tests or CI runs were
  executed or checked. The 2026-08-17 SRG-01..06 measurements remain historical and are not
  refreshed by the standalone SH-BM-01..04 stub run.
