# Architecture - Sharding Module

<!-- Status: current | validated: 2026-09-09 -->
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

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| `storage` | `include/storage/rocksdb_wrapper.h`, `include/storage/storage_engine.h` | Persistent KV store (data reads/writes backing shard data) |
| `transaction` | `include/transaction/recoverable_two_phase_coordinator.h` | Cross-shard 2PC coordination (managed circular dep) |
| `distributed_knowledge` | `include/distributed_knowledge/` | Federated knowledge routing per shard topology |
| `utils` | `include/utils/` | Utility helpers |
| `themis/edition` | `include/themis/edition.h` | Edition capability gating for advanced sharding features |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/sharding/shard_router.h`, `include/sharding/adaptive_shard_router.h` | HTTP request shard routing |
| `query` | `include/sharding/shard_router.h` | Federated query scatter-gather |
| `transaction` | `include/sharding/truetime.h`, `include/sharding/wal_manager.h` | TrueTime and WAL for distributed transactions |
| `storage` | `include/sharding/redundancy_strategy.h` | Storage consults sharding for redundancy placement |
| `distributed_knowledge` | `include/sharding/` | Cross-shard knowledge graph coordination and partition routing |
| `distributed_tensor` | `include/sharding/` | Cross-shard tensor collective operations and partition management |
| `analytics` | `include/sharding/` | Shard-aware query planning for distributed aggregations |

---

## Integration Points

### Critical Integration: Sharding → Storage (Data Access)
**Files:** `src/sharding/shard_router.cpp` ↔ `include/storage/rocksdb_wrapper.h`
**Contract:** After routing decisions, the shard layer delegates actual KV reads/writes to storage. Shard router does not buffer data; it only resolves shard ownership.
**Thread Safety:** Storage wrapper is concurrent-safe; shard router state uses shared_mutex for topology reads.
**Failure Mode:** Storage unavailability on target shard → explicit routing error returned; shard repair engine is triggered asynchronously.

### Critical Integration: Sharding ↔ Transaction (2PC Circular)
**Files:** `src/sharding/cross_shard_transaction.cpp` ↔ `include/transaction/recoverable_two_phase_coordinator.h`; `src/transaction/distributed_transaction_manager.cpp` ↔ `include/sharding/truetime.h`
**Contract:** Cross-shard transaction calls into `RecoverableTwoPhaseCoordinator`; TrueTime from sharding timestamps distributed commits. Circular dependency is managed: sharding imports transaction coordinator header; transaction imports sharding TrueTime header only.
**Thread Safety:** 2PC coordinator uses internal locking; TrueTime is lock-free read.
**Failure Mode:** Coordinator crash mid-2PC → WAL-based recovery on restart; participant timeout → abort broadcast.

### Critical Integration: Sharding → WAL (Durability)
**Files:** `src/sharding/wal_manager.cpp` ↔ `include/storage/rocksdb_wrapper.h` (WAL backend)
**Contract:** All topology-change and shard-migration events are WAL-logged before acknowledgment. WAL replay restores shard state on restart.
**Thread Safety:** WAL manager serialises writes with internal mutex; reads are concurrent.
**Failure Mode:** WAL write failure → shard operation rejected; WAL corruption → recovery mode with manual repair tooling.

### Critical Integration: Sharding → Distributed Coordinator (Consensus)
**Files:** `src/sharding/distributed_coordinator.cpp` ↔ `include/sharding/consensus_module.h`, `include/sharding/consensus_factory.h`
**Contract:** Distributed coordinator delegates leader election and quorum decisions to the consensus module (Raft-based via `consensus_factory`). Shard topology changes require quorum agreement before commit.
**Thread Safety:** Consensus module manages its own election-state mutex; coordinator observes consensus outcomes via callback.
**Failure Mode:** Loss of quorum → coordinator enters read-only safe mode; leader re-election is automatic via Raft timeout.

---

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