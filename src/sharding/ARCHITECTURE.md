# Architecture - Sharding Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The sharding module composes shard routing, consensus-aware coordination, cross-shard transaction control, repair/rebalance/migration behavior, and operational health/metrics surfaces into a bounded distributed data-partitioning subsystem.

## Main Execution Planes

1. Routing and placement plane
- key/tenant/request to shard decision behavior
- consistent hash and adaptive routing behavior

2. Coordination and transaction plane
- distributed coordinator and consensus-integrated operation behavior
- cross-shard transaction and 2PC flow behavior

3. Durability and operations plane
- WAL, repair, rebalancing, migration, and health/metrics behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| routing contract | deterministic shard selection under topology constraints |
| coordination contract | explicit distributed decision and consensus outcomes |
| transaction contract | explicit cross-shard commit/abort semantics |
| operations contract | bounded repair/rebalance/migration with observable states |

## Failure Semantics

- routing/topology mismatches return explicit failures.
- cross-shard transaction errors surface deterministic abort/rollback outcomes.
- repair/rebalance job failures remain explicit and diagnosable.
- WAL/quorum/health degradation is surfaced through operational signals.

## Sourcecode Verification (Module: sharding/architecture)

- Verified files:
  - src/sharding/shard_router.cpp
  - src/sharding/adaptive_shard_router.cpp
  - src/sharding/distributed_coordinator.cpp
  - src/sharding/cross_shard_transaction.cpp
  - src/sharding/shard_repair_engine.cpp
  - src/sharding/auto_rebalancer.cpp
  - src/sharding/wal_manager.cpp
  - src/sharding/health_monitor.cpp
- Verified architecture claims:
  - routing/placement + coordination/transaction + durability/operations plane split
  - explicit failure boundaries for routing, transaction, and job-execution faults
  - module-local ownership of sharding-domain behavior