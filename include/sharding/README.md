> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Sharding Module Headers

This directory contains public header files for the ThemisDB sharding module.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../src/sharding/README.md · ../../src/sharding/ROADMAP.md · ../../src/sharding/FUTURE_ENHANCEMENTS.md -->

## Purpose

Public interfaces and declarations for sharding, distributed consensus, replication, and cluster management functionality.

## Primary API Entry Points

- `consensus_factory.h` + `consensus_module.h` — select and drive consensus (`Raft`/`Gossip`/`Paxos`).
- `shard_router.h` + `adaptive_shard_router.h` — routing entry points for key/query placement.
- `cross_shard_transaction.h` — cross-shard transaction coordinator contract (2PC/3PC/SAGA/Percolator-style).
- `shard_repair_engine.h` + `redundancy_strategy.h` — anti-entropy orchestration and erasure-coding repair APIs.
- `admin_api.h` + `prometheus_metrics.h` — operational/admin integration surfaces.

## Configuration Surfaces (Selected)

The sharding public API exposes many config structs; the most commonly integrated entry points are:

- `ConsensusConfig` (`consensus_module.h`)
- `CrossShardTransactionConfig` (`cross_shard_transaction.h`)
- `RepairConfig` (`shard_repair_engine.h`)
- `AdaptiveShardRouter::AdaptiveConfig` (`adaptive_shard_router.h`)
- `RaftConfig` (`raft_state.h`)
- `MetadataShardConfig` (`metadata_shard.h`)
- `PartitionDetectorConfig` (`partition_detector.h`)
- `WALManagerConfig` / `TransactionWALConfig` (`wal_manager.h`, `transaction_wal.h`)

Use module-level docs in `src/sharding/` for runtime behavior and operational constraints.

## Headers

### Consensus

- `consensus_factory.h` — Consensus algorithm factory (Raft/Paxos selection)
- `consensus_module.h` — Generic consensus module interface
- `paxos_consensus.h` — Paxos consensus implementation
- `paxos_snapshot.h` — Paxos snapshot management
- `paxos_state_persistence.h` — Paxos durable state
- `paxos_wal.h` — Paxos write-ahead log integration
- `raft_consensus.h` — Raft consensus implementation
- `raft_consensus_adapter.h` — Raft adapter for module integration
- `raft_configuration.h` — Raft cluster configuration
- `raft_log.h` — Raft replicated log
- `raft_shard_manager.h` — Raft-based shard management
- `raft_state.h` — Raft node state machine
- `raft_wal_integration.h` — Raft/WAL bridge
- `quorum_manager.h` — Quorum calculation and tracking
- `multi_primary_coordinator.h` — Active-active multi-primary coordination
- `epoch_fencing.h` — Epoch-based leader fencing

### Routing & Topology

- `shard_router.h` — Request routing to shards
- `adaptive_shard_router.h` — Adaptive load-aware shard router
- `consistent_hash.h` — Consistent hash ring
- `locality_aware_router.h` — Locality/zone-aware routing
- `smart_routing.h` — ML-based smart routing <!-- TODO: verify -->
- `shard_topology.h` — Cluster topology representation
- `replica_topology.h` — Replica placement topology

### Transactions

- `cross_shard_transaction.h` — Cross-shard transaction coordination
- `distributed_transaction.h` — Distributed transaction interface
- `two_phase_commit_coordinator.h` — 2PC coordinator
- `two_phase_commit_participant.h` — 2PC participant
- `transaction_snapshot.h` — Distributed snapshot management
- `transaction_wal.h` — Transaction WAL integration

### Replication & WAL

- `replication_coordinator.h` — Replication lifecycle coordinator
- `replica_consistency.h` — Consistency level enforcement
- `wal_manager.h` — WAL lifecycle management
- `wal_applier.h` — WAL entry application
- `wal_shipper.h` — WAL shipping to replicas
- `metadata_wal.h` — Metadata WAL
- `write_concern.h` — Write durability/concern levels

### Health & Recovery

- `health_check.h` — Shard health check interface
- `health_monitor.h` — Continuous health monitoring
- `circuit_breaker.h` — Circuit breaker for shard calls
- `auto_recovery_manager.h` — Automatic failure recovery
- `hot_spare_manager.h` — Hot spare activation
- `shard_repair_engine.h` — Shard data repair
- `orphan_detector.h` — Orphaned shard data detection
- `partition_detector.h` — Network partition detection
- `predictive_detector.h` — Predictive failure detection

### Rebalancing & Migration

- `auto_rebalancer.h` — Automatic shard rebalancing
- `data_migrator.h` — Shard data migration
- `rebalance_operation.h` — Rebalance operation descriptor
- `hardware_migration_manager.h` — Hardware replacement migration

### Metadata & Config

- `metadata_shard.h` — Metadata shard interface
- `metadata_snapshot.h` — Metadata snapshot
- `gossip_config_manager.h` — Gossip-based config propagation
- `gossip_consensus_adapter.h` — Gossip/consensus adapter
- `gossip_protocol.h` — Gossip protocol implementation
- `distributed_coordinator.h` — Distributed coordination primitives
- `distributed_time_coordinator.h` — Distributed clock coordination
- `truetime.h` — TrueTime API for globally ordered timestamps

### Capacity & Resources

- `shard_capabilities.h` — Shard hardware capability registry
- `capability_matcher.h` — Capability-aware shard selection
- `shard_load_detector.h` — Shard load measurement
- `shard_resource_manager.h` — Shard resource quota management
- `shard_durability.h` — Durability policy enforcement
- `redundancy_strategy.h` — Data redundancy strategy
- `raid_optimizations.h` — RAID-style storage optimizations

### Network & Security

- `shard_rpc_client.h` — Shard RPC client
- `shard_rpc_client_adapter.h` — RPC client adapter
- `shard_rpc_server.h` — Shard RPC server
- `mtls_client.h` — mTLS shard client
- `mtls_connection_pool.h` — mTLS connection pool
- `secure_transport_client.h` — Encrypted shard transport
- `pki_shard_certificate.h` — Per-shard PKI certificate management
- `signed_request.h` — Request signing for inter-shard calls
- `backpressure_protocol.h` — Backpressure signaling protocol
- `stream_protocol.h` — Streaming protocol for bulk shard transfers

### Administration & Metrics

- `admin_api.h` — Sharding admin API
- `admin_operations.h` — Administrative shard operations
- `metrics_registry.h` — Sharding metrics registry
- `operational_metrics.h` — Runtime operational metrics
- `prometheus_metrics.h` — Prometheus metrics exporter
- `slo_monitor.h` — SLO tracking and alerting

### Cloud & GPU

- `cloud_agent.h` — Cloud provider integration agent
- `cloud_backup.h` — Cloud-native shard backup
- `gpu_erasure_coder.h` — GPU-accelerated erasure coding

### Identifiers

- `urn.h` — Uniform Resource Name definitions
- `urn_resolver.h` — URN resolution to shard endpoints

### Remote Execution

- `remote_executor.h` — Remote shard query execution
- `sharding_interfaces.h` — Core sharding interfaces
- `sharding_manager.h` — Top-level sharding manager

## Usage

### Include and initialize consensus

```cpp
#include "sharding/consensus_factory.h"
#include "sharding/consensus_module.h"

themis::sharding::ConsensusConfig cfg;
cfg.type = themis::sharding::ConsensusType::RAFT;
cfg.node_id = "node-a";
cfg.cluster_nodes = {"node-a", "node-b", "node-c"};

auto consensus = themis::sharding::ConsensusFactory::create(cfg);
consensus->initialize(cfg.node_id, cfg.cluster_nodes);
consensus->start();
```

### Include and configure repair

```cpp
#include "sharding/shard_repair_engine.h"

themis::sharding::RepairConfig cfg;
cfg.enable_auto_repair = true;
cfg.scan_interval = std::chrono::seconds(300);
```

## Implementation

See `../../src/sharding/` for the implementation code.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Troubleshooting

- Header not found (`sharding/...`): verify `${THEMISDB_INCLUDE_DIR}` and target include directories.
- Ambiguous types between modules: prefer explicit `themis::sharding::...` qualification in integration code.
- Transaction compile errors after upgrades: re-check `CrossShardTransactionConfig` fields and protocol enum usage.
- Consensus integration mismatch: ensure `ConsensusConfig::cluster_nodes` and `node_id` are both populated.

## Related Docs

- Runtime module overview: [`../../src/sharding/README.md`](../../src/sharding/README.md)
- Architecture: [`../../src/sharding/ARCHITECTURE.md`](../../src/sharding/ARCHITECTURE.md)
- Module roadmap: [`../../src/sharding/ROADMAP.md`](../../src/sharding/ROADMAP.md)
- Module future enhancements: [`../../src/sharding/FUTURE_ENHANCEMENTS.md`](../../src/sharding/FUTURE_ENHANCEMENTS.md)
- Distributed architecture overview: [`../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md`](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)
