<!-- Status: current | validated: 2026-04-06 -->

# include/sharding/ — Architecture

> Public header interfaces for the ThemisDB Sharding module.
> Implementation details live in [`../../src/sharding/`](../../src/sharding/).

---

## Overview

The `include/sharding/` directory exposes the complete public API surface of
ThemisDB's distributed sharding subsystem.  Consumers depend only on these
headers; concrete implementations reside exclusively in `../../src/sharding/`.

The sharding module provides:
- **Consistent-hash & locality-aware routing** for shard placement
- **Raft & Paxos consensus** for replicated state management
- **mTLS-secured RPC transport** between shard nodes
- **Write-Ahead Log (WAL)** infrastructure for durability and recovery
- **Distributed transactions** (2PC + cross-shard coordination)
- **Auto-rebalancing, hot-spare management, and predictive fault detection**
- **Operational observability** (Prometheus metrics, SLO monitoring)

---

## Design Principles

1. **Interface Segregation** — each header exposes a narrow, focused contract;
   callers include only what they need.
2. **Implementation Independence** — headers contain no RocksDB, gRPC, or
   consensus-library includes; those are confined to `src/sharding/`.
3. **Dependency Injection** — factory headers (`consensus_factory.h`,
   `sharding_interfaces.h`) decouple creation from usage, enabling unit tests
   with mock implementations.
4. **Observability by Default** — every long-running subsystem (router,
   rebalancer, WAL, replication) publishes metrics through `metrics_registry.h`
   and `prometheus_metrics.h`.
5. **Security at the Transport Layer** — all inter-shard communication uses
   mTLS (`mtls_client.h`, `mtls_connection_pool.h`, `secure_transport_client.h`,
   `signed_request.h`); clear-text paths are not supported.

---

## Interface Inventory

| Header | Primary Classes / Interfaces | Purpose |
|--------|------------------------------|---------|
| `adaptive_shard_router.h` | `AdaptiveShardRouter` | Load-aware, adaptive shard selection |
| `admin_api.h` | `AdminApi` | Administrative HTTP/gRPC API surface |
| `admin_operations.h` | `AdminOperations` | Cluster management commands |
| `auto_rebalancer.h` | `AutoRebalancer` | Automatic shard load rebalancing |
| `auto_recovery_manager.h` | `AutoRecoveryManager` | Failure detection & self-healing |
| `backpressure_protocol.h` | `BackpressureProtocol` | Flow-control signalling between nodes |
| `capability_matcher.h` | `CapabilityMatcher` | Hardware/feature capability negotiation |
| `circuit_breaker.h` | `CircuitBreaker` | Fault-isolation for downstream calls |
| `cloud_agent.h` | `CloudAgent` | Cloud-provider integration agent |
| `cloud_backup.h` | `CloudBackup` | Cloud-tier backup coordination |
| `consensus_factory.h` | `ConsensusFactory` | Factory for Raft/Paxos instances |
| `consensus_module.h` | `ConsensusModule` | Abstract consensus interface |
| `consistent_hash.h` | `ConsistentHashRing` | Rendezvous / consistent-hash ring |
| `cross_shard_transaction.h` | `CrossShardTransaction` | Multi-shard ACID transaction context |
| `data_migrator.h` | `DataMigrator` | Live shard-data migration |
| `distributed_coordinator.h` | `DistributedCoordinator` | Global distributed operation coordinator |
| `distributed_time_coordinator.h` | `DistributedTimeCoordinator` | Hybrid logical clock coordination |
| `distributed_transaction.h` | `DistributedTransaction` | Distributed transaction primitives |
| `gossip_config_manager.h` | `GossipConfigManager` | Config propagation via gossip |
| `gossip_consensus_adapter.h` | `GossipConsensusAdapter` | Gossip-backed consensus adapter |
| `gossip_protocol.h` | `GossipProtocol` | Epidemic membership & state gossip |
| `gpu_erasure_coder.h` | `GpuErasureCoder` | GPU-accelerated erasure coding |
| `hardware_migration_manager.h` | `HardwareMigrationManager` | Hardware-change-driven data migration |
| `health_check.h` | `HealthCheck` | Liveness & readiness probe interface |
| `health_monitor.h` | `HealthMonitor` | Continuous health aggregation |
| `hot_spare_manager.h` | `HotSpareManager` | Hot-standby shard lifecycle |
| `locality_aware_router.h` | `LocalityAwareRouter` | Rack/AZ/region-aware routing |
| `metadata_shard.h` | `MetadataShard` | Metadata shard storage interface |
| `metadata_snapshot.h` | `MetadataSnapshot` | Point-in-time metadata snapshot |
| `metadata_wal.h` | `MetadataWal` | WAL for metadata operations |
| `metrics_registry.h` | `MetricsRegistry` | Central metrics registration |
| `mtls_client.h` | `MtlsClient` | mTLS client connection |
| `mtls_connection_pool.h` | `MtlsConnectionPool` | Pooled mTLS connections |
| `multi_primary_coordinator.h` | `MultiPrimaryCoordinator` | Multi-primary write coordination |
| `operational_metrics.h` | `OperationalMetrics` | Per-operation latency/throughput metrics |
| `orphan_detector.h` | `OrphanDetector` | Detects unreferenced data shards |
| `partition_detector.h` | `PartitionDetector` | Network partition detection |
| `paxos_consensus.h` | `PaxosConsensus` | Multi-Paxos consensus implementation API |
| `paxos_snapshot.h` | `PaxosSnapshot` | Paxos state snapshot interface |
| `paxos_state_persistence.h` | `PaxosStatePersistence` | Durable Paxos state storage |
| `paxos_wal.h` | `PaxosWal` | WAL for Paxos log entries |
| `predictive_detector.h` | `PredictiveDetector` | ML-driven anomaly & failure prediction |
| `prometheus_metrics.h` | `PrometheusMetrics` | Prometheus exposition interface |
| `quorum_manager.h` | `QuorumManager` | Dynamic quorum configuration |
| `raft_configuration.h` | `RaftConfiguration` | Raft cluster configuration |
| `raft_consensus.h` | `RaftConsensus` | Raft consensus API |
| `raft_consensus_adapter.h` | `RaftConsensusAdapter` | Adapter between Raft & consensus module |
| `raft_log.h` | `RaftLog` | Raft log interface |
| `raft_shard_manager.h` | `RaftShardManager` | Shard management via Raft |
| `raft_state.h` | `RaftState` | Raft persistent state |
| `raft_wal_integration.h` | `RaftWalIntegration` | WAL integration for Raft entries |
| `raid_optimizations.h` | `RaidOptimizations` | RAID-like redundancy optimisations |
| `rebalance_operation.h` | `RebalanceOperation` | Rebalance job descriptor |
| `redundancy_strategy.h` | `RedundancyStrategy` | Pluggable redundancy policies |
| `remote_executor.h` | `RemoteExecutor` | Remote computation dispatch |
| `replica_consistency.h` | `ReplicaConsistency` | Replica consistency levels |
| `replica_topology.h` | `ReplicaTopology` | Replica placement topology |
| `replication_coordinator.h` | `ReplicationCoordinator` | Cross-replica coordination |
| `secure_transport_client.h` | `SecureTransportClient` | Encrypted transport abstraction |
| `shard_capabilities.h` | `ShardCapabilities` | Per-shard capability advertisement |
| `shard_durability.h` | `ShardDurability` | Durability guarantees interface |
| `shard_load_detector.h` | `ShardLoadDetector` | Real-time load detection |
| `shard_repair_engine.h` | `ShardRepairEngine` | Automated shard repair |
| `shard_resource_manager.h` | `ShardResourceManager` | CPU/memory/IO resource management |
| `shard_router.h` | `ShardRouter` | Core shard routing interface |
| `shard_rpc_client.h` | `ShardRpcClient` | RPC client for shard communication |
| `shard_rpc_client_adapter.h` | `ShardRpcClientAdapter` | Adapter over RPC client |
| `shard_rpc_server.h` | `ShardRpcServer` | RPC server for shard requests |
| `shard_topology.h` | `ShardTopology` | Cluster topology descriptor |
| `sharding_interfaces.h` | `IShardingEngine`, `IRouter` | Core sharding abstractions |
| `sharding_manager.h` | `ShardingManager` | Top-level sharding lifecycle manager |
| `signed_request.h` | `SignedRequest` | HMAC/Ed25519-signed RPC request |
| `slo_monitor.h` | `SloMonitor` | SLO tracking and alerting |
| `stream_protocol.h` | `StreamProtocol` | Streaming replication protocol |
| `transaction_snapshot.h` | `TransactionSnapshot` | MVCC snapshot for transactions |
| `transaction_wal.h` | `TransactionWal` | WAL for distributed transactions |
| `truetime.h` | `TrueTime` | TrueTime-style bounded uncertainty clock |
| `two_phase_commit_coordinator.h` | `TwoPhaseCommitCoordinator` | 2PC coordinator |
| `two_phase_commit_participant.h` | `TwoPhaseCommitParticipant` | 2PC participant |
| `urn.h` | `Urn` | Uniform Resource Name for shards/records |
| `urn_resolver.h` | `UrnResolver` | URN-to-shard location resolver |
| `wal_applier.h` | `WalApplier` | WAL log replay engine |
| `wal_manager.h` | `WalManager` | WAL lifecycle management |
| `wal_shipper.h` | `WalShipper` | WAL streaming to replicas |
| `write_concern.h` | `WriteConcern` | Write durability acknowledgement policy |

---

## Module Relationships

```
sharding_manager.h
  ├── consensus_factory.h → raft_consensus.h / paxos_consensus.h
  ├── shard_router.h → consistent_hash.h / locality_aware_router.h
  ├── replication_coordinator.h → replica_topology.h / replica_consistency.h
  ├── wal_manager.h → wal_applier.h / wal_shipper.h
  ├── distributed_coordinator.h → two_phase_commit_coordinator.h
  └── health_monitor.h → health_check.h / slo_monitor.h
```

---

## Implementation Reference

See [`../../src/sharding/`](../../src/sharding/) for all `.cpp` implementations.
See [`../../docs/src/sharding/`](../../docs/src/sharding/) for detailed design documentation.
