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

## Thread-Safety Model (Tier 1 Requirement — Critical)

**Concurrency guarantees (canonical lock ordering):**
- **Routing Decisions:** Immutable shard map after initialization; lock-free read paths via `std::shared_ptr<ShardTopology>`
- **Cross-Shard Coordination:** DualConsensusOrchestrator uses canonical lock order: `state_mutex_` (1) < `audit_mutex_` (2) < `metrics_mutex_` (3)
- **Consensus State:** RaftConsensusAdapter canonical order: `state_mutex_` (1) < `callbacks_mutex_` (2) < `cluster_mutex_` (3) < `snapshot_mutex_` (4)
- **2PC Coordinator:** Atomic transaction state transitions via `xact_state_` atomic<>; per-shard locks only when sending prepare/commit
- **Replication Coordinator:** Per-shard state protected by shard-local mutex; no global lock holds
- **WAL Manager:** Lock-free ring buffer for WAL entries; individual entry writes are atomic
- **Health Monitor:** Atomic health state updates; no blocking on health check paths

**Known thread-safety hardening (Batch 3 verified 2026-08-10):**
- Lock-ordering violations reduced from 95 → 0 (TSO/LKO/CCR test evidence)
- Cross-shard thread-safety gaps reduced from 340+ → ~102 (dual_consensus_orchestrator, replica_consistency hardened)
- `std::scoped_lock` used for atomic dual-mutex acquisition in `logGroundingOperation`
- `backgroundSyncThread` detects quorum loss and backs off; no indefinite waits
- Detached-thread hazards eliminated via explicit ownership transfer in SubagentLifecycleManager

**Fail-Closed Behavior (Wave A — Runtime Reliability):**
- **Shard Unavailable:** Fail fast with `Status::kShardUnavailable`; no indefinite retry
- **Quorum Loss:** Return `Status::kQuorumLost`; block new writes until quorum restored
- **Transaction Timeout:** Enforce per-transaction deadline; abort on overrun with `kDeadlineExceeded`
- **Rebalance Blocked:** If topology change fails, keep current routing; signal via health endpoint
- **Consensus Deadlock:** Detect via timeout; trigger failover to next consensus adapter
- **Migration Failure:** Backoff with exponential delay; trigger repair process; log error
- **Network Partition:** Detect via health checks; self-heal via anti-entropy process

---

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
  - thread-safety sign-off evidence: tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp (TSO/LKO/CCR tests)