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
- [~] Formal source code analysis of consensus/cross-shard invariants (Target: Q2 2026)
  - [x] S0/S1 invariant violations identified in `raft_consensus.cpp`, `paxos_consensus.cpp`, `gossip_protocol.cpp`, `cross_shard_transaction.cpp`, `raft_wal_integration.cpp`, `distributed_transaction_manager.cpp`, `transaction_wal.cpp` (see `AUDIT.md`)
  - [ ] Code fixes for PAX-1, PAX-2, PAX-3, GOS-1, CST-1, CST-2, CST-3, RWALI-1, RWALI-2 (S0 critical) (Target: Q2 2026)
  - [ ] Code fixes for PAX-4, RAFT-1, RLOG-1, 2PC-2, DTM-1, DTM-2, DTM-3 (S1 high) (Target: Q2 2026)
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

### Phase 6: Source Code Analysis — Consensus and Cross-Shard Invariants (Status: In Progress 🚧)

> **Scope:** Systematic source-level analysis of `src/sharding/` consensus and distributed
> transaction code to identify invariant violations. All findings below are grounded in actual
> function-level code inspection (see `AUDIT.md` for full details with code excerpts and fix
> guidance). No CI/CD tooling is required to resolve these items.

#### Phase 6.1: S0 Critical — Code Fixes Required (Target: Q2 2026)

- [ ] **PAX-1 — Fix Paxos non-recursive mutex self-deadlock (`paxos_consensus.cpp`)**
  - `executePreparePhase()` holds `state_mutex_` then calls `executeAcceptPhase()` which
    re-acquires the same mutex → all Paxos proposals hang permanently.
  - Fix: split into finer-grained per-instance locks or restructure call chain.
  - Affected: `paxos_consensus.cpp` `executePreparePhase()` L548, `executeAcceptPhase()` L650
- [ ] **PAX-2 — Fix Paxos leader election: replace deterministic node-ID sort with quorum-based ballot exchange (`paxos_consensus.cpp`)**
  - Current code unconditionally appoints the lexicographically smallest `node_id_` as leader
    without any Paxos Phase 1 messaging; split-brain is possible.
  - Affected: `paxos_consensus.cpp` `leaderElectionThread()` L513–527
- [ ] **PAX-3 — Enforce WAL write failure as hard error in all Paxos phases (`paxos_consensus.cpp`)**
  - WAL exceptions are caught and swallowed ("graceful degradation") in all three phases.
    After restart the node can accept lower-ballot proposals it previously rejected.
  - Affected: `executePreparePhase()` L543, `executeAcceptPhase()` L644, `broadcastCommit()` L748
- [ ] **GOS-1 — Fix Gossip `addPeer`→`syncWithTopology` deadlock (`gossip_protocol.cpp`)**
  - `addPeer()` acquires `peers_mutex_` then calls `syncWithTopology()` which re-acquires it.
  - Affected: `gossip_protocol.cpp` `addPeer()` L120, `syncWithTopology()` L527 (also `removePeer()`)
- [ ] **CST-1 — Fix dangling reference UB in `commit()` (`cross_shard_transaction.cpp`)**
  - `auto& txn = it->second` taken from map; lock released; concurrent `abort()` can erase
    the entry; `execute2PC(txn)` then operates on freed memory → UB.
  - Affected: `cross_shard_transaction.cpp` `commit()` L375–402
- [ ] **CST-2 — Fix dangling reference UB in `abort()` (`cross_shard_transaction.cpp`)**
  - Same pattern as CST-1 in `abort()` L440–473.
- [ ] **CST-3 — Fix stale reference after re-lock in `executeSaga()` (`cross_shard_transaction.cpp`)**
  - `auto& txn` reference reused at L688 after unlock/relock cycle; map may have been modified.
  - Affected: `cross_shard_transaction.cpp` `executeSaga()` L522–691
- [ ] **RWALI-1 — Fix `raft_wal_integration.cpp::write()` self-deadlock**
  - `write()` holds `mutex_` across a 5-second busy-wait loop;
    `onAppendEntriesResponse()` cannot acquire the same mutex → every write times out.
  - Affected: `raft_wal_integration.cpp` `write()` L42–96
- [ ] **RWALI-2 — Fix hardcoded cluster-size-3 in `hasQuorum()` (`raft_wal_integration.cpp`)**
  - Quorum function hardcodes cluster size = 3; wrong for any other configuration.
  - Affected: `raft_wal_integration.cpp` `hasQuorum()` L179–185

#### Phase 6.2: S1 High — Required Before Production (Target: Q2 2026)

- [ ] **PAX-4 — Implement `runAcceptor()` message processing (`paxos_consensus.cpp`)**
  - Current implementation is an empty sleep loop; distributed Paxos never processes
    incoming Prepare/Accept messages.
- [ ] **PAX-5 — Make `current_round_` atomic (`paxos_consensus.cpp`)**
  - `++current_round_` on plain `uint64_t` from concurrent threads → data race.
- [ ] **RAFT-1 — Atomicize leader check and log append / commit in `propose()` (`raft_consensus.cpp`)**
  - `isLeader()` check and detached-thread `setCommitIndex()` are not atomic; no rollback
    of appended-but-uncommitted entries on leadership loss.
- [ ] **RLOG-1 — Fix `getLastLogIndex()` to return `snapshot_index_` after compaction (`raft_log.cpp`)**
  - Returns 0 after compaction; next entry gets index 1, colliding with compacted range.
- [ ] **2PC-2 / DTM-2 — Broadcast ABORT to participants during in-doubt recovery**
  - Both `TwoPhaseCommitCoordinator::recoverInDoubtTransactions()` and
    `DistributedTransactionManager::recoverInDoubtTransactions()` log ABORT to WAL but
    never notify participants → they remain PREPARED indefinitely, holding locks.
- [ ] **DTM-1 — Replace fake remote-participant COMMIT vote with real RPC stub (`distributed_transaction_manager.cpp`)**
  - `runPhase1Unlocked()` unconditionally returns COMMIT for participants without a
    registered callback, bypassing 2PC safety.
- [ ] **DTM-3 — Implement or stub `isParticipantAlive()` with a real health check**
  - Currently always returns `true`; timed-out participants are never detected.

#### Phase 6.3: S2/S3 Medium/Low — Hardening (Target: Q3 2026)

- [ ] **RAFT-2 — Protect `replication_callback_` with mutex in `propose()` detached thread** (`raft_consensus.cpp`)
- [ ] **RAFT-3 — Add rollback path for uncommitted log entries when quorum is not reached** (`raft_consensus.cpp`)
- [ ] **GOS-2 — Implement actual signature verification in `verifyMessage()`** (`gossip_protocol.cpp`)
- [ ] **GOS-3 — Thread-local or per-thread `std::mt19937` in `selectRandomPeers()`** (`gossip_protocol.cpp`)
- [ ] **PAX-6 — Protect all `cluster_nodes_` accesses with `state_mutex_`** (`paxos_consensus.cpp`)
- [ ] **CST-4 — Reject startup if `transaction_log_path_` is unconfigured (no `/tmp` fallback)** (`cross_shard_transaction.cpp`)
- [ ] **CST-6 — Implement actual 3PC PreCommit RPC or remove 3PC claim** (`cross_shard_transaction.cpp`)
- [ ] **DTM-4 — Add explicit WAL flush before Phase 2 broadcast** (`distributed_transaction_manager.cpp`)
- [ ] **TWAL-1 — Protect `current_lsn_` with a mutex or make it `std::atomic`** (`transaction_wal.cpp`)
- [ ] **TWAL-2 — Replace magic numbers 130–138 with named `TransactionWALEntryType` range constants** (`transaction_wal.cpp`)
- [ ] **RLOG-2 — Add bounds check in `setCommitIndex()`** (`raft_log.cpp`)

#### Phase 6.4: Cross-Cutting Code Fixes

- [ ] **CC-1 — Enforce WAL flush as hard error (not warn+continue) across all consensus layers**
- [ ] **CC-4 — Gate gossip-driven topology mutations behind Raft membership change protocol**
- [ ] **CC-5 — Consolidate 2PC coordinator implementations or enforce a shared recovery interface**

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
- [ ] **[PAX-1] Paxos `state_mutex_` re-entrant deadlock fixed** (`paxos_consensus.cpp`)
- [ ] **[PAX-2] Paxos leader election replaced with quorum-based ballot exchange** (`paxos_consensus.cpp`)
- [ ] **[PAX-3] WAL failure is a hard error in all Paxos phases** (`paxos_consensus.cpp`)
- [ ] **[GOS-1] Gossip `addPeer`→`syncWithTopology` deadlock fixed** (`gossip_protocol.cpp`)
- [ ] **[CST-1/CST-2/CST-3] Dangling-reference UB fixed in `commit()`/`abort()`/`executeSaga()`** (`cross_shard_transaction.cpp`)
- [ ] **[RWALI-1] Raft WAL `write()` self-deadlock resolved** (`raft_wal_integration.cpp`)
- [ ] **[RWALI-2] `hasQuorum()` uses actual cluster size from configuration** (`raft_wal_integration.cpp`)
- [ ] **[DTM-1] Remote 2PC participant voting uses real RPC, not unconditional COMMIT vote** (`distributed_transaction_manager.cpp`)

## Known Issues & Limitations

- `[?]` Cross-shard RPC (`sharding/rpc/`) read path (`readEntity`) not yet gRPC — reads currently routed via HTTP `RemoteExecutor`. Write path uses `ReplicateData` gRPC since 2026-04-12.
- `[~]` Raft snapshot compaction not yet wired — WAL growth unbounded for long-running Raft deployments.
- `[?]` Adaptive rebalancer not yet implemented; rebalancing is currently manual-only.
- `[?]` Focused chaos tests are in CI, but full cluster-level chaos/failover scenarios are not yet part of the default production-readiness gate.
- `[!]` **PAX-1** `paxos_consensus.cpp::executePreparePhase()` holds `state_mutex_` and calls `executeAcceptPhase()` which re-acquires it — non-recursive mutex deadlock; all Paxos proposals hang permanently.
- `[!]` **PAX-2** `paxos_consensus.cpp::leaderElectionThread()` performs no quorum-based election; smallest node-ID wins unconditionally — split-brain risk.
- `[!]` **PAX-3** `paxos_consensus.cpp` WAL write failures silently ignored in all three Paxos phases — durability not guaranteed.
- `[!]` **GOS-1** `gossip_protocol.cpp::addPeer()` deadlocks when topology is configured (`peers_mutex_` re-acquired via `syncWithTopology()`).
- `[!]` **CST-1/CST-2/CST-3** `cross_shard_transaction.cpp::commit()`/`abort()`/`executeSaga()`: dangling map references after lock release — UB under concurrent access.
- `[!]` **RWALI-1** `raft_wal_integration.cpp::write()` holds `mutex_` during 5-second quorum wait; `onAppendEntriesResponse()` can never run — every write times out.
- `[!]` **RWALI-2** `raft_wal_integration.cpp::hasQuorum()` hardcodes cluster-size = 3 — wrong for any other configuration.
- `[!]` **DTM-1** `distributed_transaction_manager.cpp::runPhase1Unlocked()` unconditionally returns COMMIT vote for remote participants — 2PC safety bypassed for all remote nodes.

| # | Description | Status |
|---|-------------|--------|
| 1 | `HardwareMigrationManager::captureRingSnapshotLocked()` uses total virtual-node count as a per-shard sentinel; per-shard vnode counts are not individually exposed by `ConsistentHashRing`. | Acceptable for v1 — stability is guaranteed because `replaceEndpoint` never touches the ring. |
| 2 | Drain period tracking is configuration-only; active connection draining is not yet implemented. | Planned (Phase 5). |
