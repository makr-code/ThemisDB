# Sharding Production Readiness Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

**Status:** 🚧 In Progress — Phase 4.1 (Epoch Fencing) complete; Phase 4.2 planned

| Component | Status |
|-----------|--------|
| Pluggable consensus (Raft/Gossip/Paxos) | ✅ Production-ready |
| Cross-shard transaction coordinator (2PC/3PC/SAGA/Percolator) | ✅ Production-ready |
| Distributed deadlock detection | ✅ Production-ready |
| ShardRepairEngine (Reed-Solomon, anti-entropy) | ✅ Production-ready |
| Prometheus metrics + Admin API | ✅ Production-ready |
| Build system audit (all .cpp registered in CMake) | ✅ Complete (March 2026) |
| Focused standalone test targets (32 targets) | ✅ Complete (March 2026) |
| **Epoch-based fencing + lease management (Phase 4.1)** | ✅ **Complete (v1.9.0)** |
| RPC integration (cross-shard read/write) | 🚧 In Progress |
| Persistent Paxos acceptor state | ✅ **Complete (v1.9.1, 2026-04-12)** |
| Automatic failover orchestration (Phase 4.2) | 🔲 Planned |
| Consistent-hashing metadata shards | 🔲 Planned |
| Cross-shard query routing | 🔲 Planned |
| Adaptive rebalancer | 🔲 Planned |

## In Progress

- [~] Full RPC integration for cross-shard read/write operations (`sharding/rpc/`) (Target: Q2 2026)
- [x] Persistent Paxos acceptor state (survives process restart) — WAL durability fix (Issue: #4592) (2026-04-12)
  - `handlePrepare()` calls `wal_->logPromise()` before returning `true`
  - `handleAccept()` calls `wal_->logAccept()` before returning `true`
  - `recoverFromWAL()` restores `instances_` from `PROMISE`/`ACCEPT`/`COMMIT` entries
  - `broadcastCommit()` already called `wal_->logCommit()` (no change)
  - 10 focused tests PSR-01…PSR-10 in `tests/test_paxos_persistence_recovery.cpp`
- [x] `ShardRPCClient::writeEntity()` — gRPC `ReplicateData` cross-shard write (Issue: #4593) (2026-04-12)
  - Uses gRPC `ReplicateData` RPC; in-process simulation returns `{success:true, replicated_count:1}`
  - `handleWriteEntityGrpc()` sends a single `Entity` via `ReplicateRequest`
  - Wired into `sendRequestGrpc()` routing as `"write_entity"`

## Planned Features

- [ ] Complete metadata shard implementation with consistent hashing (Target: Q3 2026)
- [ ] End-to-end cross-shard query routing layer (Target: Q3 2026)
- [ ] gRPC transport with mTLS for all inter-shard RPC channels (Target: Q3 2026)
- [ ] Adaptive rebalancer driven by per-shard access-pattern telemetry (Target: Q4 2026)
- [~] Reed-Solomon repair parallelisation across repair workers (v1.6.0, parallel scan bands via ThreadPoolManager, IOPS throttle, GPU flag, SLO progress — in progress)
- [ ] Raft snapshot compaction to bound log growth (Target: Q4 2026)
- [ ] Chaos-engineering test suite (shard partition, node failure injection) (Target: Q4 2026)

## Introduction
Sharding is a database architecture pattern that involves breaking a database into smaller, more manageable pieces called shards. A comprehensive sharding strategy is essential for scaling applications effectively. This roadmap outlines the implementation phases for preparing the ThemisDB sharding architecture for production.

## Implementation Phases

### Phase 1: Consensus & Transaction Coordination (Status: Completed ✅)
- [x] Pluggable consensus framework supporting Raft, Gossip, and Paxos strategies
- [x] ConsensusFactory – runtime strategy selection and configuration
- [x] Cross-shard transaction coordinator with 2PC, 3PC, SAGA, and Percolator protocols
- [x] Distributed deadlock detection (wait-for-graph cycle detection)
- [x] Metadata sharding design and data-model documentation
- [x] Architecture documentation and ADRs

### Phase 2: Repair Engine & Observability (Status: Completed ✅)
- [x] ShardRepairEngine – automated shard repair orchestration
- [x] Reed-Solomon erasure decoder via Vandermonde matrix construction
- [x] Prometheus metrics for shard health, repair ops, and consensus latency
- [x] Admin API endpoints (shard status, force-repair, rebalance trigger)

### Phase 2.5: Build System Audit (Status: Completed ✅ — March 2026)
- [x] All `src/sharding/*.cpp` files registered in `cmake/CMakeLists.txt` (added `admin_operations.cpp`, `operational_metrics.cpp`, `sharding_manager_edition.cpp`, `slo_monitor.cpp`)
- [x] All `src/sharding/*.cpp` files registered in `cmake/ModularBuild.cmake` THEMIS_SHARDING_SOURCES (added `hardware_migration_manager.cpp`, `sharding_manager_edition.cpp`, `two_phase_commit_coordinator.cpp`, `two_phase_commit_participant.cpp`)
- [x] 24 focused standalone test targets added in `tests/CMakeLists.txt` covering all 23 sharding test files
- [x] `test_shard_durability` and `test_slo_monitor` promoted from excluded to focused standalone targets
- [x] 7 additional focused targets added for `test_adaptive_shard_router`, `test_cross_shard_coordinator`, `test_cross_shard_distribution`, `test_metadata_shard`, `test_multi_shard_transactions`, `test_pki_shard_certificate`, `test_raft_shard_manager`
- [x] 7 more focused targets registered: `test_sharding_chaos`, `test_sharding_e2e`, `test_sharding_gossip`, `test_sharding_integration`, `test_sharding_interfaces`, `test_sharding_operational_metrics`, `test_sharding_uncovered`
- [x] `circuit_breaker.cpp` state-transition logging implemented via `spdlog::info`
- [x] `OrphanDetector` wired to `DistributedCoordinator` (listed as done in FUTURE_ENHANCEMENTS.md)
- [x] CI workflow added: `.github/workflows/06-infrastructure_distributed_sharding-focused-tests-ci.yml`

### Phase 3: RPC Integration & Persistent State (Status: In Progress 🚧)
- [x] Persistent Paxos acceptor state (survives process restart) — WAL durability fix (Issue: #4592) (2026-04-12)
- [x] Cross-shard write via gRPC `ReplicateData` RPC — `ShardRPCClient::writeEntity()` (Issue: #4593) (2026-04-12)
- [?] Full RPC integration for all cross-shard read/write operations (`sharding/rpc/`) (Target: Q2 2026)
- [?] Complete metadata shard implementation with consistent hashing (Target: Q3 2026)
- [?] End-to-end cross-shard query routing layer (Target: Q3 2026)

### Phase 4: Hardening & Adaptive Rebalancing (Status: In Progress 🚧)
- [x] Epoch-based fencing + lease management (`epoch_fencing.h` / `epoch_fencing.cpp`) — Phase 4.1 ✅ (v1.9.0)
- [?] Automatic failover orchestration — Phase 4.2 (planned)
- [?] Adaptive rebalancer driven by per-shard access-pattern telemetry
- [~] Reed-Solomon repair parallelisation across repair workers (v1.6.0 — parallel scan bands, IOPS throttle, GPU flag)
- [?] Raft snapshot compaction to bound log growth
- [x] Chaos-engineering test suite (shard partition, node failure injection) — `test_sharding_chaos_focused` registered

### Phase 5: Hardware Migration Support (Status: Beta 🟡)
- [x] `HardwareMigrationManager` — safe endpoint replacement without altering hash-ring positions
- [x] `NodeIdentity` — logical shard identity persisted to disk, independent of physical hardware
- [x] Ring-stability validation: assert that virtual-node positions are unchanged after endpoint update
- [ ] Admin API endpoint `/api/v1/shards/{id}/migrate-hardware` (Target: Q3 2026)
- [ ] Raft peer-address update integration (Target: Q3 2026)
- [ ] Drain-period enforcement with in-flight request tracking (Target: Q3 2026)

## Conclusion
Implementing sharding requires careful planning and execution. Following this roadmap will help ensure that the ThemisDB sharding architecture is robust, scalable, and ready for production deployment.
## Production Readiness Checklist

- [x] Pluggable consensus framework (Raft/Gossip/Paxos) tested under simulated failures
- [x] Cross-shard transactions with rollback and deadlock detection
- [x] ShardRepairEngine with Reed-Solomon erasure recovery
- [x] Prometheus metrics and admin API endpoints
- [ ] RPC integration with mTLS for all cross-shard channels
- [x] Persistent consensus state survives process restart — Paxos WAL durability fix (Issue: #4592, 2026-04-12)
- [ ] End-to-end cross-shard query routing verified under load (≥ 10,000 cross-shard ops/s)
- [ ] Chaos-engineering test suite passing (shard partition, node failure, split-brain)

## Known Issues & Limitations

- `[?]` Cross-shard RPC (`sharding/rpc/`) partially wired — `ShardRPCClient::writeEntity()` uses gRPC `ReplicateData` (Issue: #4593); full read-path and query routing still use in-process simulation.
- `[x]` Paxos acceptor state now persisted to WAL — `handlePrepare`/`handleAccept` log before returning, `recoverFromWAL()` restores on restart (Issue: #4592, 2026-04-12).
- `[?]` Adaptive rebalancer not yet implemented; rebalancing is currently manual-only.
- `[?]` Chaos test suite not yet committed to CI — shard partition failures detected only in manual test runs.

| # | Description | Status |
|---|-------------|--------|
| 1 | `HardwareMigrationManager::captureRingSnapshotLocked()` uses total virtual-node count as a per-shard sentinel; per-shard vnode counts are not individually exposed by `ConsistentHashRing`. | Acceptable for v1 — stability is guaranteed because `replaceEndpoint` never touches the ring. |
| 2 | Drain period tracking is configuration-only; active connection draining is not yet implemented. | Planned (Phase 5). |
