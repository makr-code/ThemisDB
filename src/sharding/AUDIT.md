> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
# Audit Report — Sharding Module
**Last Audit:** 2026-04-21 | **Status:** 🔴 Critical — 8 S0 findings block distributed correctness

> **Note on self-reported quality scores in file headers:** Multiple consensus files carry
> banners reading "PRODUCTION-READY / Quality Score: 100.0/100". These scores do not reflect
> actual code correctness. The source code analysis below supersedes all header-level claims.

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Test Coverage | ✅ Present (unit; distributed integration coverage gaps) |
| S0 Critical / Safety Violations | 🔴 8 |
| S1 High | 🔴 10 |
| S2 Medium | ⚠️ 1 (CST-6 open) |
| S3 Low | ℹ️ 3 |
| Distributed correctness under concurrent access | 🔴 **Not guaranteed** |

## Source Files Audited (Deep Analysis — 2026-04-21)

| Component | Files | Safety Status |
|-----------|-------|---------------|
| Consensus — Raft | `raft_consensus.cpp`, `raft_log.cpp`, `raft_wal_integration.cpp` | 🔴 S0+S1 findings |
| Consensus — Paxos | `paxos_consensus.cpp` | 🔴 S0 (3×): permanent deadlock, broken election, WAL degradation |
| Consensus — Gossip | `gossip_protocol.cpp` | 🔴 S0: deadlock in `addPeer`→`syncWithTopology`; ✅ S2 GOS-2/GOS-3 fixed 2026-05-04 |
| Distributed transactions | `cross_shard_transaction.cpp`, `two_phase_commit_coordinator.cpp`, `transaction_wal.cpp` | 🔴 S0 (3×): dangling-reference UB in commit/abort/saga |
| WAL & replication | `raft_wal_integration.cpp`, `transaction_wal.cpp` | 🔴 S0: self-deadlock; ⚠️ non-atomic LSN |
| Shard routing | `adaptive_shard_router.cpp`, `consistent_hash.cpp`, `shard_router.cpp` | ✅ No critical findings |
| Shard health & repair | `circuit_breaker.cpp`, `shard_repair_engine.cpp`, `orphan_detector.cpp` | ✅ No critical findings |
| Rebalancing & migration | `hardware_migration_manager.cpp` | ✅ No critical findings |

## Findings

### S0 — Critical (Safety Violations / Undefined Behavior / Permanent Deadlock)

#### PAX-1 · `paxos_consensus.cpp` · `executePreparePhase()` → `executeAcceptPhase()`

`executePreparePhase()` acquires `state_mutex_` (non-recursive `std::mutex`) then calls
`executeAcceptPhase()`, which re-acquires the same mutex from the same thread. This is
Undefined Behavior (guaranteed deadlock on all major platforms). **All Paxos proposals in
any multi-node cluster will hang permanently.**

```cpp
// executePreparePhase(), L548: lock acquired
std::lock_guard<std::mutex> lock(state_mutex_);
...
return executeAcceptPhase(slot, proposal, value);  // L628 — called with lock held

// executeAcceptPhase(), L650: re-lock attempt on same thread
std::lock_guard<std::mutex> lock(state_mutex_);  // ← UB / deadlock
```

**Fix required:** Split `state_mutex_` into finer-grained per-instance or per-phase locks,
or restructure so `executeAcceptPhase` receives already-locked data structures by reference.

---

#### PAX-2 · `paxos_consensus.cpp` · `leaderElectionThread()` L513–527

Leader election performs no Paxos messaging, no quorum, and no epoch/ballot comparison.
Any node whose `node_id_` is the lexicographic minimum of `cluster_nodes_` unconditionally
self-promotes to `LEADER`. If `cluster_nodes_` is inconsistently populated (possible — see
PAX-6), multiple nodes can simultaneously hold `LEADER` state → **split-brain**.

```cpp
std::string expected_leader = *std::min_element(
    cluster_nodes_.begin(), cluster_nodes_.end()  // no lock on cluster_nodes_
);
bool should_be_leader = (node_id_ == expected_leader);
if (should_be_leader && !is_current_leader) {
    state_.store(ConsensusState::LEADER);   // unconditional self-promotion
    current_leader_ = node_id_;
}
```

**Fix required:** Replace with ballot-based Phase 1 leader election; all candidates must
gather a quorum of promises from other acceptors before transitioning to LEADER.

---

#### PAX-3 · `paxos_consensus.cpp` · `executePreparePhase()`, `executeAcceptPhase()`, `broadcastCommit()`

WAL write failures are caught and silently discarded in **all three Paxos phases** under the
label "graceful degradation":

```cpp
// same pattern repeated in all 3 phases:
} catch (const std::exception& e) {
    spdlog::warn("Failed to log PREPARE to WAL: {}", e.what());
    // Continue operation despite WAL failure (graceful degradation)
}
```

A node that proceeds through PREPARE/ACCEPT/COMMIT without a durable WAL record will, after
a restart, have no memory of its promises and can accept lower-ballot proposals it previously
rejected — **violating the core Paxos safety property (no two values committed for same
slot)**.

**Fix required:** WAL write failure must be a hard error in all Paxos phase handlers.
The phase must abort and return a failure response, not proceed silently.

---

#### GOS-1 · `gossip_protocol.cpp` · `addPeer()` L120 → `syncWithTopology()` L527

`addPeer()` acquires `peers_mutex_` then calls `syncWithTopology()`, which also acquires
`peers_mutex_`. Deadlock in any deployment where a `ShardTopology` is configured.
`removePeer()` has the identical pattern.

```cpp
void GossipProtocol::addPeer(const PeerInfo& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);  // ACQUIRED
    ...
    syncWithTopology();                               // CALLED WITH LOCK HELD

void GossipProtocol::syncWithTopology() {
    if (!topology_) return;   // safe only when topology_ == nullptr
    std::lock_guard<std::mutex> lock(peers_mutex_);  // DEADLOCK when topology_ != nullptr
```

**Fix required:** Extract the topology sync into a separate helper that operates on an
already-locked `peers_` snapshot, or release the lock before calling `syncWithTopology()`.

---

#### CST-1 · `cross_shard_transaction.cpp` · `commit()` L375–402

`auto& txn = it->second` takes a reference into `transactions_`, then the lock is released.
A concurrent `abort()` can erase the entry, invalidating the reference. The subsequent call
to `execute2PC(txn)` then operates on freed memory — **Undefined Behavior / memory
corruption**.

```cpp
auto& txn = it->second;   // reference into map
bool success = false;
lock.unlock();            // ← map is now unprotected; reference is dangling if erased
switch (txn.protocol) {   // UB: txn may be freed
    case TransactionProtocol::TWO_PHASE_COMMIT:
        success = execute2PC(txn);  // dangling reference passed
```

**Fix required:** Copy `txn` by value before releasing the lock, or hold the lock for the
entire dispatch. Use a `shared_ptr`-based transaction handle rather than a raw map reference.

---

#### CST-2 · `cross_shard_transaction.cpp` · `abort()` ~L440–473

Same dangling-reference pattern as CST-1: `auto& txn` taken from map, lock released, then
`txn.participants` iterated and mutated. A concurrent `commit()` or a second `abort()` call
can erase the entry while the iterator runs.

**Fix required:** Same as CST-1 — copy by value or keep the lock held across the loop.

---

#### CST-3 · `cross_shard_transaction.cpp` · `executeSaga()` ~L522–691

`auto& txn = it->second` is used at the start of the function. The function releases the
lock mid-execution (L529), performs many lines of unlocked work, then re-locks (L687) and
writes `txn.state = TransactionState::COMMITTED`. The reference `txn` is stale at L688;
another thread may have erased the entry during the unlocked section.

**Fix required:** Re-look-up the transaction after re-locking, or use a value copy.

---

#### RWALI-1 · `raft_wal_integration.cpp` · `write()` L42–96

`write()` holds `mutex_` for its entire execution, which includes a 5-second busy-wait loop
polling for quorum ACKs. `onAppendEntriesResponse()` also acquires `mutex_`. Since
`write()` never releases the lock during the poll, `onAppendEntriesResponse()` can never
run. The ACK set never grows. **Every `write()` call times out unconditionally after
5 seconds** — the Raft WAL integration layer is non-functional under any concurrent
workload.

**Fix required:** Use a `std::unique_lock` and `condition_variable` inside `write()`, or
track pending writes in an external `std::unordered_map` so `write()` can release the lock
while waiting for ACKs from `onAppendEntriesResponse()`.

---

#### RWALI-2 · `raft_wal_integration.cpp` · `hasQuorum()` L179–185

Cluster size is hardcoded to 3:

```cpp
bool RaftWALIntegration::hasQuorum(const std::set<std::string>& acks) const {
    size_t cluster_size = 3;  // In real impl, get from RaftConfiguration
    size_t quorum = (cluster_size / 2) + 1;  // = 2
    return acks.size() >= quorum;
}
```

For a 5-node cluster, quorum should be 3 but this returns true at 2 ACKs (under-majority
commit). For a 1-node cluster, quorum should be 1 but this requires 2 ACKs (write can never
succeed). **Correct operation is impossible for any cluster size other than 3.**

**Fix required:** Inject actual cluster membership size from `RaftConfiguration`; use
`config_.cluster_members.size()` or pass quorum size at construction.

---

### S1 — High

| ID | File | Function | Description |
|----|------|----------|-------------|
| PAX-4 | `paxos_consensus.cpp` | `runAcceptor()` | Acceptor thread is empty sleep loop — no incoming Prepare/Accept messages ever processed; distributed multi-node Paxos cannot function |
| PAX-5 | `paxos_consensus.cpp` | `generateProposalNumber()` | `++current_round_` on plain `uint64_t` (not `std::atomic`) from concurrent threads — data race |
| PAX-7 | `paxos_consensus.cpp` | `stop()` | `savePersistentState()` reads `instances_`/`committed_log_` without a lock after `running_=false` but before thread joins — data race |
| RAFT-1 | `raft_consensus.cpp` | `propose()` | TOCTOU: `isLeader()` check and `setCommitIndex()` in detached thread are not atomic; node can step down between check and commit; no rollback of the appended-but-uncommitted log entry |
| RLOG-1 | `raft_log.cpp` | `getLastLogIndex()` | Returns 0 after compaction (log map is empty instead of returning `snapshot_index_`); next append assigns index 1, colliding with compacted range |
| 2PC-1 | `two_phase_commit_coordinator.cpp` | `commit()` | `runPhase1()` and `runPhase2()` make blocking participant RPC calls while holding `mutex_` — all 2PC operations serialized behind a blocking network call |
| 2PC-2 | `two_phase_commit_coordinator.cpp` | `recoverInDoubtTransactions()` | In-doubt transactions with no recorded decision: marked ABORT internally but abort never broadcast to participants; they remain locked in PREPARED state indefinitely |
| DTM-1 | `distributed_transaction_manager.cpp` | `runPhase1Unlocked()` | Remote participants (registered by endpoint, no callback) unconditionally return a COMMIT vote without any RPC — the fundamental 2PC safety guarantee is bypassed for all remote nodes |
| DTM-2 | `distributed_transaction_manager.cpp` | `recoverInDoubtTransactions()` | In-doubt transactions logged ABORTED in WAL, but ABORT never broadcast to participants; same unresolved PREPARED lock accumulation as 2PC-2 |
| DTM-3 | `distributed_transaction_manager.cpp` | `isParticipantAlive()` | Always returns `true`; health detection is an unimplemented stub |

---

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| ~~RAFT-2~~ | ✅ fixed 2026-05-04 | `raft_consensus.cpp` | `setReplicationCallback()` | `replication_callback_` write now protected by `replica_mutex_`; read-side in `propose()` was already locked |
| RAFT-3 | `raft_consensus.cpp` | `propose()` | Entry appended to log before quorum is confirmed; no rollback path if ACK count falls short |
| ~~GOS-2~~ | ✅ fixed 2026-05-04 | `gossip_protocol.cpp` | `verifyMessage()` | Empty-signature and missing-key-dir paths now fail-closed when `validate_certificates=true`; real RSA-SHA256 verification wired |
| ~~GOS-3~~ | ✅ fixed 2026-05-04 | `gossip_protocol.cpp` | `selectRandomPeers()` | Replaced `static std::mt19937` with `thread_local` |
| ~~PAX-6~~ | ✅ fixed 2026-05-04 | `paxos_consensus.cpp` | Various | `cluster_nodes_` reads in `leaderElectionThread()` and `getStats()` now guarded by `state_mutex_` |
| ~~CST-4~~ | ✅ fixed 2026-05-04 | `cross_shard_transaction.cpp` | Constructor | `/tmp` fallback removed; constructor now throws `std::invalid_argument` when `transaction_log_path` is not an absolute path |
| ~~CST-5~~ | ✅ fixed 2026-05-04 | `cross_shard_transaction.cpp` | `prepare()` | `sendPrepare()` call wrapped in try/catch; exception now re-acquires the `unique_lock` before propagating |
| CST-6 | `cross_shard_transaction.cpp` | `execute3PC()` | 3PC PreCommit phase is simulated: `bool precommitted = participant.prepared;` — no actual PreCommit RPC; 3PC provides no correctness benefit over 2PC as implemented |
| ~~DTM-4~~ | ✅ fixed 2026-05-04 | `distributed_transaction.cpp` | `commit()` | COMMIT intent now written to WAL and flushed via `wal_manager_->flush()` before `retryCommitPhase()` (Phase 2 broadcast) |
| ~~TWAL-1~~ | ✅ fixed 2026-05-04 | `transaction_wal.cpp` | All `log*()` methods | `current_lsn_` updates now protected by `lsn_mutex_` |

---

### S3 — Low

| ID | File | Function | Description |
|----|------|----------|-------------|
| 2PC-3 | `two_phase_commit_coordinator.cpp` | `commit()` | COMMIT/ABORT WAL entry written twice (before Phase 2 and after); recovery must distinguish by `"phase"` field — brittle |
| RLOG-2 | `raft_log.cpp` | `setCommitIndex()` | No bounds check — commit index can be advanced past last log index by a buggy caller |
| TWAL-2 | `transaction_wal.cpp` | `readEntries()` | Magic number range `130–138` used to identify WAL entry types; silent recovery failure if enum values change |

---

## Cross-Cutting Invariant Violations

### CC-1 — "Graceful WAL degradation" collapses durability across all consensus layers

Paxos (`paxos_consensus.cpp`) silently swallows WAL write failures in all three phases.
Without a durable record of promises, the node violates the Paxos invariant
"an acceptor must remember every promise it has made across restarts." No Paxos phase
transition is durable under WAL failure.

### CC-2 — All three consensus/coordination layers bypass their quorum guarantees via distinct bugs

- **Raft WAL integration (`raft_wal_integration.cpp`):** `write()` self-deadlocks — no write ever completes.
- **Paxos (`paxos_consensus.cpp`):** `executePreparePhase()` deadlocks — all proposals hang.
- **2PC remote participants (`distributed_transaction_manager.cpp`):** remote participant votes faked — 2PC safety bypassed.

### CC-3 — WAL-before-commit ordering is inconsistently enforced

| Component | WAL before state change? |
|---|---|
| `raft_wal_integration.cpp write()` | ✅ WAL written first (but self-deadlocks before quorum) |
| `two_phase_commit_coordinator.cpp commit()` | ✅ WAL before Phase 2 — but no explicit flush |
| `distributed_transaction.cpp commit()` | ✅ WAL before Phase 2 — COMMIT intent written and flushed before `retryCommitPhase()` (DTM-4 fixed 2026-05-04) |
| `cross_shard_transaction.cpp commit()` | ⚠️ WAL writes present but not verified before lock release |
| `paxos_consensus.cpp broadcastCommit()` | ❌ WAL failure silently ignored; can broadcast commit without durable WAL entry |

### CC-4 — Gossip topology mutations bypass Raft membership change protocol

`GossipProtocol::syncWithTopology()` calls `topology_->addShard()` for every gossip-discovered
peer without going through Raft joint-consensus membership change. A rogue node advertising
itself via gossip can affect quorum calculations without authorization.

### CC-5 — Three independent 2PC implementations with incompatible state machines

`TwoPhaseCommitCoordinator`, `CrossShardTransactionCoordinator`, and
`DistributedTransactionManager` each implement 2PC with different state machines, different
WAL integration depth, and different recovery logic. A transaction begun with one coordinator
cannot be recovered by another.

---

## Compliance

- Multi-tenant data isolation via shard-key namespacing: ✅
- Distributed transaction safety under concurrent access: 🔴 **Not guaranteed — see S0 findings above**
- WAL durability before commit: 🔴 **Violated in Paxos; not enforced in Gossip topology**
- Quorum correctness: 🔴 **Hardcoded cluster size; self-deadlocked WAL integration**
