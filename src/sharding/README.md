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
  - ../../include/sharding/sharding_api_contract.h
  - ../../include/sharding/shard_router.h
  - ../../tests/sharding/CMakeLists.txt
  - ../../benchmarks/sharding/CMakeLists.txt
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Sharding Module

<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The sharding module provides distributed partitioning, routing, consensus coordination, cross-shard transaction/repair/rebalancing behavior, and operational observability for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| `ShardRouter` (`shard_router.h/.cpp`) | URN point operations, query strategy selection, scatter/gather, and selected-shard execution |
| `ConsistentHashRing` (`consistent_hash.h/.cpp`) | Hash-ring placement and lookup |
| `AdaptiveShardRouter` (`adaptive_shard_router.h/.cpp`) | Adaptive routing surface; policy is component-specific |
| `DistributedCoordinator` (`distributed_coordinator.h/.cpp`) | Distributed task/coordination surface |
| `CrossShardTransactionCoordinator` (`cross_shard_transaction.h/.cpp`) | Cross-shard transaction coordination and transaction-state APIs |
| `TwoPhaseCommitCoordinator` (`two_phase_commit_coordinator.h/.cpp`) | 2PC coordinator surface |
| `ShardRepairEngine`, `AutoRebalancer`, `DataMigrator` | Separate repair, rebalance, and migration components |
| `WALManager`, `WALApplier` (`wal_manager.h`, `wal_applier.h`) | Sharding WAL interfaces and application path |
| `HealthMonitor`, `OperationalMetrics` | Health and metrics surfaces |

These are source/header mappings, not claims that all adapters, transport backends, or optional
hardware paths are enabled in every build. The normative v1.x statements in
`include/sharding/sharding_api_contract.h` are contracts; implementation conformance must be
checked in the affected component and its tests.

## Scope

In scope:
- shard placement, routing, and distributed coordination
- cross-shard transaction and consensus integration behavior
- rebalancing, repair, migration, and shard operational metrics

Out of scope:
- storage engine internals owned by storage subsystem
- client-facing API contracts outside sharding module boundaries
- non-sharding business-domain orchestration logic

## Runtime Behavior and Limits

- routing/placement behavior depends on the selected topology and router configuration.
- transaction, repair, rebalance, and migration classes expose component-specific results and
  states; this summary does not assert one universal commit, rollback, or failover policy.
- the public contract describes intended outcomes; verify the implementation and relevant test for
  each changed path.

## Concurrency and failure behavior

Thread-safety and failure semantics are component-specific; there is no single module-wide
lock-ordering or automatic failover policy. The API contract header documents intended contract
semantics, while the implementation and focused test must be checked for the component being
changed. **TODO:** document a lock hierarchy only where it is backed by the corresponding current
class implementation and test.

Do not infer that a quorum loss automatically blocks every write path, a failed consensus adapter
automatically switches to another, or a migration automatically triggers repair. Those effects
depend on the specific coordinator/router/operation path.

## Build and test hints

The `linux-release` preset defaults both tests and benchmarks off. To opt in for a focused local
build, configure with:

```sh
cmake --preset linux-release -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-gcc-linux-release --target test_sharding_core_focused bench_sharding_release_gates
ctest --test-dir build-gcc-linux-release -R ShardingCoreFocusedTests
```

Tests and their CMake names/labels are registered conditionally in
`../../tests/sharding/CMakeLists.txt`. Other useful focused sources include
`test_sharding_contract_hardening_focused.cpp`, `test_sharding_multishard_exact.cpp`, and
`test_sharding_thread_safety_lock_order_focused.cpp`. Registration does not mean a test passed;
this audit did not execute the commands above.

Benchmarks are in `../../benchmarks/sharding/`. See `PERFORMANCE_EXPECTATIONS.md` for current
benchmark-to-source mapping and the boundary of the historical baseline.

---

## Source map checked for this document (2026-10-31)

- Verified files:
  - src/sharding/shard_router.cpp
  - src/sharding/adaptive_shard_router.cpp
  - src/sharding/consistent_hash.cpp
  - src/sharding/distributed_coordinator.cpp
  - src/sharding/cross_shard_transaction.cpp
  - src/sharding/two_phase_commit_coordinator.cpp
  - src/sharding/shard_repair_engine.cpp
  - src/sharding/auto_rebalancer.cpp
  - src/sharding/data_migrator.cpp
  - src/sharding/replication_coordinator.cpp
  - src/sharding/health_monitor.cpp
  - src/sharding/operational_metrics.cpp
  - src/sharding/quorum_manager.cpp
  - src/sharding/wal_manager.cpp
  - src/sharding/wal_applier.cpp
- The files above are implementation references for routing/consensus/transaction/repair/rebalance/
  observability topics; their presence is not an execution or conformance result.
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
  - the thread-safety/lock-order source is `tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp`;
    current CMake registration is in `tests/sharding/CMakeLists.txt`.