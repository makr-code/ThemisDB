> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/sharding/ARCHITECTURE.md -->

# Sharding Module — Public Header Architecture

**Module Path:** `include/sharding/`
**Implementation:** `../../src/sharding/`
**Canonical architecture doc:** [`../../src/sharding/ARCHITECTURE.md`](../../src/sharding/ARCHITECTURE.md)

---

## 1. Overview

`include/sharding/` defines the **public distributed partitioning, routing, and coordination contract** for ThemisDB. The 87 headers cover shard placement, consensus and quorum management, cross-shard transactions, repair and rebalance operations, metadata and WAL durability, transport security, and operational metrics.

For full distributed-runtime details — routing internals, consensus adapters, and repair/rebalance orchestration — see:
→ [`../../src/sharding/ARCHITECTURE.md`](../../src/sharding/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Routing and Placement

| Header | Public Type | Purpose |
|--------|------------|---------|
| `shard_router.h` / `adaptive_shard_router.h` | `ShardRouter`, `AdaptiveShardRouter` | Primary and adaptive shard routing |
| `consistent_hash.h` / `locality_aware_router.h` | `ConsistentHash`, `LocalityAwareRouter` | Placement and locality-aware route selection |
| `capability_matcher.h`, `shard_capabilities.h` | `CapabilityMatcher`, `ShardCapabilities` | Capability-aware placement decisions |
| `replica_topology.h`, `shard_topology.h` | `ReplicaTopology`, `ShardTopology` | Topology description contracts |

### 2.2 Coordination, Consensus, and Quorum

| Header | Public Type | Purpose |
|--------|------------|---------|
| `distributed_coordinator.h` / `remote_executor.h` | `DistributedCoordinator`, `RemoteExecutor` | Cross-node orchestration |
| `consensus_module.h`, `consensus_factory.h` | `IConsensusModule`, `ConsensusFactory` | Consensus abstraction and factory |
| `raft_consensus.h`, `raft_configuration.h`, `raft_state.h`, `raft_log.h` | `RaftConsensus` family | Raft configuration, log, and state contracts |
| `paxos_consensus.h`, `paxos_snapshot.h`, `paxos_state_persistence.h`, `paxos_wal.h` | `PaxosConsensus` family | Paxos-based consensus surfaces |
| `quorum_manager.h`, `epoch_fencing.h`, `truetime.h` | `QuorumManager`, `EpochFencing`, `TrueTime` | Quorum, fencing, and time coordination |

### 2.3 Transactions and Distributed Commit

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cross_shard_transaction.h` | `CrossShardTransactionCoordinator` | Cross-shard transaction orchestration and distributed wait-for reporting |
| `distributed_transaction.h` / `transaction_snapshot.h` | `DistributedTransaction`, `TransactionSnapshot` | Transaction state and snapshot exchange |
| `two_phase_commit_coordinator.h`, `two_phase_commit_participant.h` | `TwoPhaseCommitCoordinator`, `TwoPhaseCommitParticipant` | 2PC contract |
| `distributed_time_coordinator.h`, `write_concern.h` | `DistributedTimeCoordinator`, `WriteConcern` | Commit-time and durability constraints |

### 2.4 Durability, Metadata, and Replication

| Header | Public Type | Purpose |
|--------|------------|---------|
| `metadata_shard.h`, `metadata_snapshot.h`, `metadata_wal.h` | `MetadataShard` family | Metadata partition state |
| `wal_manager.h`, `wal_applier.h`, `wal_shipper.h`, `transaction_wal.h` | WAL managers | WAL persistence, apply, and ship flows |
| `replication_coordinator.h`, `replica_consistency.h`, `redundancy_strategy.h` | Replication and redundancy types | Replica coordination and durability semantics |
| `raft_wal_integration.h` | `RaftWALIntegration` | Consensus/WAL bridge |

### 2.5 Repair, Migration, and Recovery

| Header | Public Type | Purpose |
|--------|------------|---------|
| `auto_rebalancer.h`, `rebalance_operation.h`, `data_migrator.h` | Rebalance and migration types | Planned and live shard movement |
| `shard_repair_engine.h`, `orphan_detector.h`, `partition_detector.h` | Repair and detection types | Repair and consistency tooling |
| `auto_recovery_manager.h`, `hot_spare_manager.h`, `hardware_migration_manager.h` | Recovery managers | Recovery and hardware migration |
| `cloud_backup.h`, `cloud_agent.h`, `raid_optimizations.h`, `gpu_erasure_coder.h` | Auxiliary operations | Backup and advanced recovery helpers |

### 2.6 Security, Transport, and Operations

| Header | Public Type | Purpose |
|--------|------------|---------|
| `mtls_client.h`, `mtls_connection_pool.h`, `secure_transport_client.h` | Secure transport types | Inter-node secure transport |
| `pki_shard_certificate.h`, `signed_request.h` | PKI and request-signing types | Trust establishment between nodes |
| `health_monitor.h`, `health_check.h`, `slo_monitor.h` | Health and SLO types | Operational health visibility |
| `operational_metrics.h`, `metrics_registry.h`, `prometheus_metrics.h` | Metrics types | Distributed operations telemetry |
| `admin_api.h`, `admin_operations.h` | Admin surfaces | Administrative control plane |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::sharding` | Shard routing, coordination, and durability types |

---

## 4. Public Contract Notes

- Routing headers expose deterministic shard-selection contracts that higher layers can embed without depending on coordinator internals.
- `cross_shard_transaction.h` is part of the stable contract because distributed wait-for reporting and deadlock detection must be visible across module boundaries.
- Consensus, quorum, and WAL headers remain public so embedders can swap or compose deployment-specific coordination strategies.
- Repair/recovery and metrics headers provide explicit degraded-state visibility instead of hiding operational transitions inside implementation-only types.
