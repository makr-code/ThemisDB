<!-- Status: current | validated: 2026-04-06 -->

# Changelog — include/sharding/

All notable changes to the **public sharding headers** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

> Implementation changelog: [`../../src/sharding/CHANGELOG.md`](../../src/sharding/CHANGELOG.md)

---

## [Unreleased]

### Planned
- `streaming_replication_v2.h` — enhanced streaming API with back-pressure hints
- `geo_partition_router.h` — geographic partition-aware routing interface

---

## [1.5.0] — 2026-03-12

### Added
- `predictive_detector.h` — ML-driven anomaly detection interface
- `gpu_erasure_coder.h` — GPU-accelerated erasure coding API (CUDA-optional)
- `slo_monitor.h` — SLO threshold monitoring and alerting interface
- `hardware_migration_manager.h` — hardware-change-driven migration interface
- `stream_protocol.h` — streaming replication protocol interface
- `multi_primary_coordinator.h` — multi-primary write coordination interface

### Changed
- `raft_consensus.h` — added `SnapshotTransfer` API for faster follower catch-up
- `sharding_manager.h` — added `DrainShard()` and `UndoDrain()` lifecycle methods
- `write_concern.h` — added `MAJORITY_FSYNC` and `ALL_NODES` concern levels
- `health_monitor.h` — added `RegisterProbe()` for custom health checks
- `consistent_hash.h` — added virtual-node weight configuration

### Fixed
- `wal_manager.h` — corrected `TruncateBefore()` parameter semantics (was off-by-one)
- `circuit_breaker.h` — half-open state transition now documented in header comment

---

## [1.4.0] — 2025-12-10

### Added
- `truetime.h` — TrueTime-style bounded uncertainty clock interface
- `distributed_time_coordinator.h` — HLC coordination across shard nodes
- `raid_optimizations.h` — RAID-like redundancy optimisation API
- `paxos_snapshot.h`, `paxos_state_persistence.h`, `paxos_wal.h` — Paxos persistence layer

### Changed
- `shard_router.h` — added `RoutingHint` parameter for latency-sensitive reads
- `replication_coordinator.h` — added `PauseReplication()` / `ResumeReplication()`
- `metrics_registry.h` — added histogram and summary metric types

---

## [1.3.0] — 2025-09-15

### Added
- `gossip_config_manager.h`, `gossip_consensus_adapter.h`, `gossip_protocol.h`
- `mtls_connection_pool.h` — pooled mTLS connections
- `signed_request.h` — HMAC / Ed25519 request signing
- `urn.h`, `urn_resolver.h` — URN addressing scheme for shard objects

### Changed
- `shard_topology.h` — extended with rack and availability-zone annotations
- `quorum_manager.h` — flexible quorum sizes (no longer hard-coded majority)

---

## [1.2.0] — 2025-06-20

### Added
- `cloud_agent.h`, `cloud_backup.h` — cloud-tier integration headers
- `orphan_detector.h` — unreferenced shard detection
- `partition_detector.h` — network partition detection interface
- `hot_spare_manager.h` — hot-standby lifecycle management

### Changed
- `auto_rebalancer.h` — added `RebalancePolicy` enum and dry-run mode
- `data_migrator.h` — added rate-limiting and pause/resume support

---

## [1.1.0] — 2025-03-10

### Added
- `two_phase_commit_coordinator.h`, `two_phase_commit_participant.h`
- `cross_shard_transaction.h`, `transaction_snapshot.h`, `transaction_wal.h`
- `remote_executor.h`, `backpressure_protocol.h`
- `shard_repair_engine.h`, `shard_resource_manager.h`

### Changed
- `consensus_module.h` — abstracted into pure-virtual interface
- `raft_consensus_adapter.h` added to decouple Raft from consumer code

---

## [1.0.0] — 2024-12-01

### Added
- Initial public header set: routing, Raft consensus, WAL, replication,
  health monitoring, and administrative API headers.
