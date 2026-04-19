<!-- Status: current | validated: 2026-04-19 -->

# include/sharding/ — Audit Report

| Field | Value |
|-------|-------|
| **Last Audit Date** | 2026-04-19 |
| **Auditor** | ThemisDB Core Team |
| **Audit Type** | Public Header Surface Review |
| **Status** | ✅ Pass |
| **Total Headers** | 88 |
| **Issues Found** | 0 critical · 0 high · 2 informational |

---

## Summary

| Category | Count |
|----------|-------|
| Routing & topology headers | 8 |
| Consensus (Raft + Paxos) headers | 12 |
| WAL headers | 7 |
| Transaction headers | 7 |
| Replication headers | 5 |
| Transport & security headers | 6 |
| Observability headers | 5 |
| Admin & operational headers | 6 |
| Gossip & config headers | 3 |
| Recovery & repair headers | 6 |
| Resource management headers | 5 |
| Utility / supporting headers | 16 |

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `adaptive_shard_router.h` | `AdaptiveShardRouter` | ✅ Clean API |
| `admin_api.h` | `AdminApi` | ✅ Clean API |
| `admin_operations.h` | `AdminOperations` | ✅ Clean API |
| `auto_rebalancer.h` | `AutoRebalancer` | ✅ Clean API |
| `auto_recovery_manager.h` | `AutoRecoveryManager` | ✅ Clean API |
| `backpressure_protocol.h` | `BackpressureProtocol` | ✅ Clean API |
| `capability_matcher.h` | `CapabilityMatcher` | ✅ Clean API |
| `circuit_breaker.h` | `CircuitBreaker` | ✅ Clean API |
| `cloud_agent.h` | `CloudAgent` | ✅ Clean API |
| `cloud_backup.h` | `CloudBackup` | ✅ Clean API |
| `consensus_factory.h` | `ConsensusFactory` | ✅ Clean API |
| `consensus_module.h` | `ConsensusModule` | ✅ Abstract base — verified pure-virtual |
| `consistent_hash.h` | `ConsistentHashRing` | ✅ Clean API |
| `cross_shard_transaction.h` | `CrossShardTransaction` | ✅ Clean API |
| `data_migrator.h` | `DataMigrator` | ✅ Clean API |
| `distributed_coordinator.h` | `DistributedCoordinator` | ✅ Clean API |
| `distributed_time_coordinator.h` | `DistributedTimeCoordinator` | ✅ Clean API |
| `distributed_transaction.h` | `DistributedTransaction` | ✅ Clean API |
| `gossip_config_manager.h` | `GossipConfigManager` | ✅ Clean API |
| `gossip_consensus_adapter.h` | `GossipConsensusAdapter` | ✅ Clean API |
| `gossip_protocol.h` | `GossipProtocol` | ✅ Clean API |
| `gpu_erasure_coder.h` | `GpuErasureCoder` | ℹ️ CUDA path conditionally compiled |
| `hardware_migration_manager.h` | `HardwareMigrationManager` | ✅ Clean API |
| `health_check.h` | `HealthCheck` | ✅ Clean API |
| `health_monitor.h` | `HealthMonitor` | ✅ Clean API |
| `hot_spare_manager.h` | `HotSpareManager` | ✅ Clean API |
| `locality_aware_router.h` | `LocalityAwareRouter` | ✅ Clean API |
| `metadata_shard.h` | `MetadataShard` | ✅ Clean API |
| `metadata_snapshot.h` | `MetadataSnapshot` | ✅ Clean API |
| `metadata_wal.h` | `MetadataWal` | ✅ Clean API |
| `metrics_registry.h` | `MetricsRegistry` | ✅ Clean API |
| `mtls_client.h` | `MtlsClient` | ✅ Clean API |
| `mtls_connection_pool.h` | `MtlsConnectionPool` | ✅ Clean API |
| `multi_primary_coordinator.h` | `MultiPrimaryCoordinator` | ✅ Clean API |
| `operational_metrics.h` | `OperationalMetrics` | ✅ Clean API |
| `orphan_detector.h` | `OrphanDetector` | ✅ Clean API |
| `partition_detector.h` | `PartitionDetector` | ✅ Clean API |
| `paxos_consensus.h` | `PaxosConsensus` | ✅ Clean API |
| `paxos_snapshot.h` | `PaxosSnapshot` | ✅ Clean API |
| `paxos_state_persistence.h` | `PaxosStatePersistence` | ✅ Clean API |
| `paxos_wal.h` | `PaxosWal` | ✅ Clean API |
| `predictive_detector.h` | `PredictiveDetector` | ✅ Clean API |
| `prometheus_metrics.h` | `PrometheusMetrics` | ✅ Clean API |
| `quorum_manager.h` | `QuorumManager` | ✅ Clean API |
| `raft_configuration.h` | `RaftConfiguration` | ✅ Clean API |
| `raft_consensus.h` | `RaftConsensus` | ✅ Clean API |
| `raft_consensus_adapter.h` | `RaftConsensusAdapter` | ✅ Clean API |
| `raft_log.h` | `RaftLog` | ✅ Clean API |
| `raft_shard_manager.h` | `RaftShardManager` | ✅ Clean API |
| `raft_state.h` | `RaftState` | ✅ Clean API |
| `raft_wal_integration.h` | `RaftWalIntegration` | ✅ Clean API |
| `raid_optimizations.h` | `RaidOptimizations` | ℹ️ Platform-specific optimisation paths |
| `rebalance_operation.h` | `RebalanceOperation` | ✅ Clean API |
| `redundancy_strategy.h` | `RedundancyStrategy` | ✅ Clean API |
| `remote_executor.h` | `RemoteExecutor` | ✅ Clean API |
| `replica_consistency.h` | `ReplicaConsistency` | ✅ Clean API |
| `replica_topology.h` | `ReplicaTopology` | ✅ Clean API |
| `replication_coordinator.h` | `ReplicationCoordinator` | ✅ Clean API |
| `secure_transport_client.h` | `SecureTransportClient` | ✅ Clean API |
| `shard_capabilities.h` | `ShardCapabilities` | ✅ Clean API |
| `shard_durability.h` | `ShardDurability` | ✅ Clean API |
| `shard_load_detector.h` | `ShardLoadDetector` | ✅ Clean API |
| `shard_repair_engine.h` | `ShardRepairEngine` | ✅ Clean API |
| `shard_resource_manager.h` | `ShardResourceManager` | ✅ Clean API |
| `shard_router.h` | `ShardRouter` | ✅ Clean API |
| `shard_rpc_client.h` | `ShardRpcClient` | ✅ Clean API |
| `shard_rpc_client_adapter.h` | `ShardRpcClientAdapter` | ✅ Clean API |
| `shard_rpc_server.h` | `ShardRpcServer` | ✅ Clean API |
| `shard_topology.h` | `ShardTopology` | ✅ Clean API |
| `sharding_interfaces.h` | `IShardingEngine`, `IRouter` | ✅ Pure-virtual contracts verified |
| `sharding_manager.h` | `ShardingManager` | ✅ Clean API |
| `signed_request.h` | `SignedRequest` | ✅ Clean API |
| `slo_monitor.h` | `SloMonitor` | ✅ Clean API |
| `stream_protocol.h` | `StreamProtocol` | ✅ Clean API |
| `transaction_snapshot.h` | `TransactionSnapshot` | ✅ Clean API |
| `transaction_wal.h` | `TransactionWal` | ✅ Clean API |
| `truetime.h` | `TrueTime` | ✅ Clean API |
| `two_phase_commit_coordinator.h` | `TwoPhaseCommitCoordinator` | ✅ Clean API |
| `two_phase_commit_participant.h` | `TwoPhaseCommitParticipant` | ✅ Clean API |
| `urn.h` | `Urn` | ✅ Clean API |
| `urn_resolver.h` | `UrnResolver` | ✅ Clean API |
| `wal_applier.h` | `WalApplier` | ✅ Clean API |
| `wal_manager.h` | `WalManager` | ✅ Clean API |
| `wal_shipper.h` | `WalShipper` | ✅ Clean API |
| `write_concern.h` | `WriteConcern` | ✅ Clean API |
| `epoch_fencing.h` | `EpochFencing` | ✅ Reviewed |
| `pki_shard_certificate.h` | `PkiShardCertificate` | ✅ Reviewed |

---

## Findings

### ℹ️ INFO-001 — Conditional CUDA Compilation in `gpu_erasure_coder.h`

The `gpu_erasure_coder.h` header uses `#ifdef THEMIS_CUDA_ENABLED` guards.
This is expected and correct; non-GPU builds compile cleanly.  No action required.

### ℹ️ INFO-002 — Platform Branching in `raid_optimizations.h`

`raid_optimizations.h` contains `#ifdef __linux__` guards for io_uring-based
paths.  Windows/macOS builds use fallback implementations.  No action required.

---

## Next Audit

Scheduled: **2026-09-22**
