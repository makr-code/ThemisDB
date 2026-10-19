---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Sharding API Contract Summary

## Purpose

This document summarizes the normative public-facing sharding contracts defined in `include/sharding/sharding_api_contract.h` and the surrounding public headers under `include/sharding/`.

## API buckets

### 1. Routing

The routing contract covers key-to-shard mapping, ring stability, and fail-safe fallback order:

- consistent hash membership is stable under a fixed topology,
- route decisions prefer the primary shard when healthy,
- fallback is bounded and must not silently choose an arbitrary shard,
- topology invalidity is a fail-closed state.

Relevant surface: `shard_router.h`, `adaptive_shard_router.h`, `locality_aware_router.h`, `consistent_hash.h`.

### 2. Coordination and quorum

The coordination contract covers cluster decision-making, leader selection, and quorum enforcement:

- the coordinator only accepts decisions that satisfy the quorum contract,
- unavailable or lost quorum triggers explicit fail-closed behavior,
- recoverable paths must remain observable and auditable.

Relevant surface: `distributed_coordinator.h`, `quorum_manager.h`, `raft_*` headers, `consensus_factory.h`.

### 3. Two-phase commit and transaction state

The transaction contract defines the safety requirements for commit and abort:

- prepare must preserve the transaction in a log-valid state,
- commit and abort are idempotent decisions,
- an in-doubt transaction must remain blocked from conflicting writes,
- a transaction must reach one of the explicit terminal states.

Relevant surface: `cross_shard_transaction.h`, `two_phase_commit_coordinator.h`, `distributed_transaction.h`, `transaction_wal.h`.

### 4. WAL and durability

The WAL model is strong: entries are ordered by LSN and durable before commit acknowledgement. Replay is deterministic and corruption is treated as a structural failure, not as a degraded success path.

Relevant surface: `wal_manager.h`, `wal_applier.h`, `transaction_wal.h`, `metadata_wal.h`.

### 5. Migration and rebalance

Migration and rebalancing are modeled as active operations with state, progress, and rollback handling:

- migration conflict is explicit,
- partial migration must be recoverable,
- throughput and verification assumptions must be maintained,
- a rebalance that cannot complete safely must fail closed or roll back.

Relevant surface: `rebalance_operation.h`, `data_migrator.h`, `auto_rebalancer.h`.

## Canonical error codes

The sharding error taxonomy is defined in `sharding_api_contract.h` and includes:

- `QUORUM_LOST`
- `COORDINATOR_FAILURE`
- `SHARD_UNAVAILABLE`
- `MIGRATION_CONFLICT`
- `WAL_CORRUPTION`
- `CONSENSUS_TIMEOUT`
- `TRANSACTION_IN_DOUBT`
- `ROUTING_RING_INVALID`
- `MIGRATION_FAULT`
- `RING_EMPTY`
- `SHARD_INDEX_OUT_OF_RANGE`
- `INTERNAL_ERROR`

These are stable for the v1.x contract and must be used consistently by all sharding components.

## Caller expectations

Callers should assume:

- no silent fallback for unavailable quorum,
- no silent partial migration,
- no hidden downgrade on WAL corruption,
- no undefined route selection if ring state is invalid,
- no successful cross-shard commit without a durable decision path.

## Summary

The sharding API is deliberately strict. The source code and header contracts ask the runtime to be explicit about route ownership, transaction state, durability, and migration outcome. That is the correct design for a module whose correctness is operationally critical.
