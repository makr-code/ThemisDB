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
  - sharding_api_contract.h
  - shard_router.h
  - consensus_factory.h
  - tests/sharding/CMakeLists.txt
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# Sharding Module Headers

This directory contains public header files for the ThemisDB sharding module.

<!-- Links: ../../src/sharding/README.md · ../../src/sharding/ROADMAP.md · ../../src/sharding/FUTURE_ENHANCEMENTS.md -->

## Purpose

Public declarations for sharding and related distributed coordination, transaction, durability,
transport, recovery, and operations components. A declaration in this directory does not by itself
mean that an optional backend is enabled in a particular build.

## Primary API Entry Points

- `shard_router.h` — `ShardRouter`, including URN point operations, query strategy selection,
  scatter/gather, and execution on a selected shard set.
- `consistent_hash.h`, `adaptive_shard_router.h`, and `locality_aware_router.h` — separate
  placement/routing abstractions; their policy and availability depend on the caller's wiring.
- `consensus_factory.h` and `consensus_module.h` — consensus factory/abstraction declarations.
  The tree contains multiple adapters; this is not a promise that every adapter is enabled.
- `cross_shard_transaction.h` — `CrossShardTransactionCoordinator` and `PercolatorCoordinator`;
  see the declarations for the distinct protocols and configuration.
- `wal_manager.h`, `wal_applier.h`, and `transaction_wal.h` — WAL-facing interfaces.
- `admin_api.h`, `health_monitor.h`, and `prometheus_metrics.h` — administration and
  observability declarations.

## Configuration Surfaces (Selected)

Configuration is component-specific. Check the named struct and defaults in the corresponding
header rather than assuming a module-wide configuration:

- `ConsensusConfig` (`consensus_module.h`)
- `CrossShardTransactionConfig` (`cross_shard_transaction.h`)
- `RepairConfig` (`shard_repair_engine.h`)
- `AdaptiveShardRouter::AdaptiveConfig` (`adaptive_shard_router.h`)
- `RaftConfig` (`raft_state.h`)
- `MetadataShardConfig` (`metadata_shard.h`)
- `PartitionDetectorConfig` (`partition_detector.h`)
- `WALManagerConfig` / `TransactionWALConfig` (`wal_manager.h`, `transaction_wal.h`)

Namespace qualification is declaration-specific: `ConsensusConfig` and `ConsensusFactory` are in
`themisdb::sharding`, while `ShardRouter` is in `themis::sharding`. Do not infer a single
`themis::sharding` prefix for every header; verify the declaration in the included header.

Use module-level docs in `src/sharding/` for runtime behavior and operational constraints.

## Selected Headers

The groups below provide orientation, not a complete file inventory; see the directory for the
current set of public headers.

### Consensus

- `consensus_factory.h` — Factory API for consensus implementations (including Raft, Gossip, and Paxos; see the header for its supported types and overloads)
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

### Adapter Distribution

- `lora_artifact_distribution.h` — adapter distribution store, Merkle proof engine, and distribution manager contracts

### Identifiers

- `urn.h` — Uniform Resource Name definitions
- `urn_resolver.h` — URN resolution to shard endpoints

### Remote Execution

- `remote_executor.h` — Remote shard query execution
- `sharding_interfaces.h` — Core sharding interfaces
- `sharding_manager.h` — Top-level sharding manager

## Usage

**TODO:** add a compiling initialization example after choosing a supported deployment backend and
verifying its exact factory overload, required configuration, initialization, and startup sequence.
The current header set exposes multiple consensus-related interfaces; this overview does not
prescribe a single universal setup path.

## Implementation

See `../../src/sharding/` for the implementation code.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Tests and build hints

Focused module tests are declared conditionally in `tests/sharding/CMakeLists.txt`; for example,
`test_sharding_core.cpp`, `test_sharding_contract_hardening_focused.cpp`, and
`test_sharding_thread_safety_lock_order_focused.cpp` have named CMake registrations there.
Registration is evidence of a configured test target, not evidence that it passed in a given build.
Use the repository's configured CMake preset, then build/run the selected target or CTest name.
See `../../src/sharding/README.md` for the maintained module map.

## Troubleshooting

- Header not found (`sharding/...`): verify `${THEMISDB_INCLUDE_DIR}` and target include directories.
- Ambiguous types between modules: use the namespace declared for that type in its header (for example, `themisdb::sharding::ConsensusFactory` versus `themis::sharding::ShardRouter`).
- Transaction compile errors after upgrades: check current `CrossShardTransactionConfig` fields and protocol enum in the header.
- Consensus integration mismatch: check the selected interface and factory configuration; do not assume one common config shape.

## Related Docs

- Runtime module overview: [`../../src/sharding/README.md`](../../src/sharding/README.md)
- Architecture: [`../../src/sharding/ARCHITECTURE.md`](../../src/sharding/ARCHITECTURE.md)
- Module roadmap: [`../../src/sharding/ROADMAP.md`](../../src/sharding/ROADMAP.md)
- Module future enhancements: [`../../src/sharding/FUTURE_ENHANCEMENTS.md`](../../src/sharding/FUTURE_ENHANCEMENTS.md)
- Distributed architecture overview: [`../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md`](../../docs/de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md)
