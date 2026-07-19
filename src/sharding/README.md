# ThemisDB Sharding Module

<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The sharding module provides distributed partitioning, routing, consensus coordination, cross-shard transaction/repair/rebalancing behavior, and operational observability for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| shard_router.cpp | shard routing and key-to-shard decision behavior |
| adaptive_shard_router.cpp | adaptive route selection under load/profile constraints |
| consistent_hash.cpp | consistent hash placement behavior |
| distributed_coordinator.cpp | distributed coordination behavior |
| cross_shard_transaction.cpp | cross-shard transaction orchestration behavior |
| two_phase_commit_coordinator.cpp | 2PC coordinator behavior |
| shard_repair_engine.cpp | repair scheduling and execution behavior |
| auto_rebalancer.cpp | automatic rebalance decision behavior |
| data_migrator.cpp | data movement and migration path behavior |
| replication_coordinator.cpp | shard-level replication coordination behavior |
| health_monitor.cpp | shard health monitoring behavior |
| operational_metrics.cpp | operational metrics surface behavior |
| quorum_manager.cpp | quorum decision and validation behavior |
| wal_manager.cpp | shard WAL handling behavior |
| wal_applier.cpp | WAL apply behavior |

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

- routing and placement behavior is bounded by topology/config constraints.
- cross-shard transaction paths expose explicit commit/abort outcomes.
- repair/rebalance/migration behavior remains observable and deterministic.
- degraded shard states and quorum/health outcomes are explicit.

## Sourcecode Verification (Module: sharding/readme)

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
- Verified behavior surfaces:
  - routing/consensus/transaction/repair/rebalance/observability paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md