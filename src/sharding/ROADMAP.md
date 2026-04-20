> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Sharding Production Readiness Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

**Status:** 🚧 In Progress — Phase 4.1 (Epoch Fencing) complete; Phase 4.2 (Automatic Failover Orchestration) complete ✅

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
| Persistent Paxos acceptor state | 🚧 In Progress |
| Automatic failover orchestration (Phase 4.2) | ✅ Complete (v2.0.0) |
| Persistent Paxos acceptor state | ✅ **Fixed (2026-04-12)** |
| Automatic failover orchestration (Phase 4.2) | 🔲 Planned |
| Consistent-hashing metadata shards | 🔲 Planned |
| Cross-shard query routing | ✅ **Implemented** |
| Adaptive rebalancer | 🔲 Planned |

## In Progress

- [~] Full RPC integration for cross-shard read/write operations (`sharding/rpc/`) (Target: Q2 2026)
  - [x] `ShardRPCClient::writeEntity()` added — uses gRPC `ReplicateData` RPC for cross-shard entity writes
  - [~] `readKey` / `readEntity` gRPC RPC not yet in proto (planned Q3 2026); reads currently routed via HTTP `RemoteExecutor`
- [~] Formal verification program for consensus and cross-shard transaction invariants (Target: Q3 2026)
  - [~] S0/S1 invariant set defined for Raft/Paxos/Gossip quorum safety, WAL recovery convergence, and cross-shard 2PC/3PC/SAGA/Percolator commit rules
  - [ ] Code-to-model abstraction boundaries mapped for `raft_consensus.cpp`, `paxos_consensus.cpp`, `gossip_protocol.cpp`, `cross_shard_transaction.cpp`, `two_phase_commit_coordinator.cpp`
  - [ ] CI model-checking gate for protocol-change PRs (TLA+/PlusCal) (Target: Q3 2026)
- [x] Persistent Paxos acceptor state (survives process restart) — **Fixed 2026-04-12**
  - `handlePrepare()` now calls `wal_->logPromise()` before returning PROMISE
  - `handleAccept()` now calls `wal_->logAccept()` before returning ACCEPTED
  - `handleCommit()` now calls `wal_->logCommit()` before broadcasting
  - `recoverFromWAL()` now restores `instances_` (promised/accepted round + value) from PROMISE/ACCEPT/COMMIT entries
  - 10 focused tests added: `PaxosPersistenceRecoveryFocusedTests` (PSR-01..PSR-10)

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
- [x] Persistent Paxos acceptor state (survives process restart) — **Fixed 2026-04-12**
  - `handlePrepare/Accept/Commit` now fsync WAL before responding
  - `recoverFromWAL()` properly restores `instances_` from WAL entries
  - `PaxosStatePersistence` unit tests: PSR-01..PSR-10
- [~] Full RPC integration for cross-shard read/write operations (`sharding/rpc/`) (Target: Q2 2026)
  - [x] `ShardRPCClient::writeEntity()` — gRPC `ReplicateData` + in-process path
  - [~] `readEntity` gRPC RPC (planned Q3 2026, reads via HTTP RemoteExecutor for now)
- [?] Complete metadata shard implementation with consistent hashing (Target: Q3 2026)
- [x] Cross-shard query routing — `ShardRouter::executeQuery/scatterGather/executeOnShards` implemented

### Phase 4: Hardening & Adaptive Rebalancing (Status: In Progress 🚧)
- [x] **Epoch-based fencing + lease management (`epoch_fencing.h` / `epoch_fencing.cpp`) — Phase 4.1 ✅ (v1.9.0)**
- [x] **Automatic failover orchestration — Phase 4.2 ✅ (v2.0.0)** (`failover/auto_failover_manager.h` / `.cpp` — 39 focused tests)
- [?] Adaptive rebalancer driven by per-shard access-pattern telemetry
- [~] Reed-Solomon repair parallelisation across repair workers (v1.6.0 — parallel scan bands, IOPS throttle, GPU flag)
- [?] Raft snapshot compaction to bound log growth
- [x] Chaos-engineering test suite (shard partition, node failure injection) — `test_sharding_chaos_focused` registered
- [x] Paxos persistence recovery tests — `test_paxos_persistence_recovery_focused` registered (PSR-01..PSR-10)

### Phase 5: Hardware Migration Support (Status: Complete ✅)
- [x] `HardwareMigrationManager` — safe endpoint replacement without altering hash-ring positions
- [x] `NodeIdentity` — logical shard identity persisted to disk, independent of physical hardware
- [x] Ring-stability validation: assert that virtual-node positions are unchanged after endpoint update
- [x] Admin API endpoint `POST /api/v1/shards/{id}/migrate-hardware` (v2.1.0)
  - `AdminAPI::setMigrationManager()` + `registerMigrateHardwareHandler()` + `handleMigrateHardware()`
  - Body: `{"new_endpoint": "host:port"}`; returns `{success, shard_id, old_endpoint, new_endpoint}`
  - Returns 501 when no manager attached; 400 on missing/empty inputs; custom handler overrides built-in path
- [x] Raft peer-address update integration (v2.1.0)
  - `ReplicaState::endpoint` field added; `RaftConsensus::updatePeerAddress(node_id, new_endpoint)` thread-safe under `replica_mutex_`; no-op + warning for unknown peers
- [x] Drain-period enforcement with in-flight request tracking (v2.1.0)
  - `HardwareMigrationManager::addInFlightRequest()` / `releaseInFlightRequest()` / `inFlightCount()`
  - `DrainGuard` RAII scope (move-only); `makeRequestGuard(shard_id)` factory
  - `waitForDrain(shard_id, timeout)` — `condition_variable`-based; `timeout=0` → skip; returns `true` if all requests drained before deadline

### Phase 6: Formal Verification of Consensus and Cross-Shard Invariants (Status: In Progress 🚧)
- [~] **Phase 1: Formal model**
  - [ ] TLA+/PlusCal model package created for consensus and cross-shard transaction state machines (Target: Q2 2026)
  - [ ] Abstraction boundaries between implementation traces and model events defined (Target: Q2 2026)
  - [ ] Safety + liveness properties formalized for quorum, commit, recovery, and reconfiguration paths (Target: Q2 2026)
- [ ] **Phase 2: Model checking**
  - [ ] Fault scenarios covered: partition, delay, reorder, crash, restart (Target: Q3 2026)
  - [ ] Bounded + unbounded critical state-space exploration runs automated (Target: Q3 2026)
  - [ ] Counterexamples exported into deterministic regression fixtures (Target: Q3 2026)
- [ ] **Phase 3: Code integration**
  - [ ] Property-based tests generated from counterexample traces (Target: Q3 2026)
  - [ ] Verification-aligned assertions added to critical commit/recovery/reconfiguration code paths (Target: Q3 2026)
  - [ ] Dedicated CI model-checking job wired into protocol-change workflows (Target: Q3 2026)
- [ ] **Phase 4: Governance**
  - [ ] Safety gates block merge of protocol changes without proof artifacts (Target: Q3 2026)
  - [ ] Review template requires impacted invariants + proof status (Target: Q3 2026)
  - [ ] Quarterly re-verification cadence for major architecture changes (Target: Q4 2026)
- [ ] **Core invariants tracked**
  - [ ] No duplicate commits for the same transaction ID
  - [ ] No acknowledged write without valid quorum/commit condition
  - [ ] WAL recovery does not cause replica state divergence
  - [ ] Reconfiguration must preserve safety properties
- [ ] **KPIs (initial)**
  - [ ] 100% of defined S0/S1 invariants formally modeled
  - [ ] 100% of discovered counterexamples captured as regression tests
  - [ ] 0 unresolved safety violations before release
  - [ ] CI verification run mandatory for protocol changes
- [ ] **Deliverables**
  - [ ] Formal specification package
  - [ ] Verification report with found/fixed counterexamples
  - [ ] Regression suite generated from model-checker outputs
  - [ ] Governance policy for protocol changes

## Conclusion
Implementing sharding requires careful planning and execution. Following this roadmap will help ensure that the ThemisDB sharding architecture is robust, scalable, and ready for production deployment.
## Production Readiness Checklist

- [x] Pluggable consensus framework (Raft/Gossip/Paxos) tested under simulated failures
- [x] Cross-shard transactions with rollback and deadlock detection
- [x] ShardRepairEngine with Reed-Solomon erasure recovery
- [x] Prometheus metrics and admin API endpoints
- [x] Persistent Paxos acceptor state survives process restart (WAL fsynced before PROMISE/ACCEPTED response)
- [x] Cross-shard query routing (scatter-gather, single-shard, cross-shard join via ShardRouter)
- [x] Chaos-engineering test suite registered and passing (`test_sharding_chaos_focused`, `test_paxos_persistence_recovery_focused`)
- [x] LLM-aware domain routing: `AdaptiveShardRouter::updateShardLLMLoad()` + LEAST_LOADED tie-breaking in `routeByDomain()` (v1.19.0, DLR-01..06)
- [x] Admin API `POST /api/v1/shards/{id}/migrate-hardware` (v2.1.0 — `AdminAPI::setMigrationManager()`, returns 501/400 on invalid inputs)
- [x] `RaftConsensus::updatePeerAddress()` — thread-safe peer endpoint update (v2.1.0)
- [x] Drain-period enforcement — `DrainGuard` RAII + `waitForDrain()` condition-variable (v2.1.0)
- [ ] RPC integration with mTLS for all cross-shard channels (write: gRPC ReplicateData ✅; read: HTTP for now)
- [ ] End-to-end cross-shard query routing verified under load (≥ 10,000 cross-shard ops/s)
- [ ] Formal safety invariants for consensus/cross-shard protocols are model-checked in CI for every protocol change
- [ ] Critical model-checker counterexamples are converted into deterministic regression tests

## Known Issues & Limitations

- `[?]` Cross-shard RPC (`sharding/rpc/`) read path (`readEntity`) not yet gRPC — reads currently routed via HTTP `RemoteExecutor`. Write path uses `ReplicateData` gRPC since 2026-04-12.
- `[~]` Raft snapshot compaction not yet wired — WAL growth unbounded for long-running Raft deployments.
- `[?]` Adaptive rebalancer not yet implemented; rebalancing is currently manual-only.
- `[?]` Focused chaos tests are in CI, but full cluster-level chaos/failover scenarios are not yet part of the default production-readiness gate.
- `[~]` Formal verification rollout for consensus + cross-shard protocols is in progress; CI gate and full counterexample-to-regression automation are not yet complete.

| # | Description | Status |
|---|-------------|--------|
| 1 | `HardwareMigrationManager::captureRingSnapshotLocked()` uses total virtual-node count as a per-shard sentinel; per-shard vnode counts are not individually exposed by `ConsistentHashRing`. | Acceptable for v1 — stability is guaranteed because `replaceEndpoint` never touches the ring. |
| 2 | Drain period tracking is configuration-only; active connection draining is not yet implemented. | Planned (Phase 5). |
