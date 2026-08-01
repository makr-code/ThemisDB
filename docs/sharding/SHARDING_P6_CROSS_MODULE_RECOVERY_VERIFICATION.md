# Sharding Phase 6 — Cross-Module Recovery Contract Verification

<!-- Status: v1.0 complete | generated: 2026-08-01 -->
<!-- Primary (Quelle der Wahrheit): docs/sharding/SHARDING_P6_SIGN_OFF.md §Cross-Module Recovery Contract -->
<!-- Gate: release_critical label on develop -->
<!-- Related Issue: #5372 (transaction coordinators architecture) -->

## Overview

This document formalizes the **boundary evidence attachment** for Batch B (Sharding Phase 6 Sign-Off)
by verifying that each module implementing the cross-module recovery contract maintains its
idempotency and durability guarantees as defined in `SHARDING_P6_SIGN_OFF.md`.

The verification covers 5 recovery surfaces:
1. `cross_shard_transaction::recoverFromWAL()` — sharding coordinator recovery
2. `two_phase_commit_participant::recoverFromWAL()` — 2PC participant recovery
3. `transaction_wal::replay()` — transaction WAL replay
4. `replication_manager::applyFrom(LSN)` — replication log application
5. `raft_v2::applyEntry()` — Raft membership entry application

---

## 1. Sharding Coordinator Recovery (`cross_shard_transaction::recoverFromWAL()`)

### Requirement

**Idempotency Guarantee:** Coordinator durability — decision (COMMIT or ABORT) is written to
WAL **before** any participant delivery; crash-restart replays from WAL to completion.

### Verification Evidence

| Test ID | File | Criteria | Status |
|---------|------|----------|--------|
| **TXC-09** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Coordinator PREPARE logged, crash before COMMIT → recovery replays COMMIT | ✅ PASS |
| **TXC-10** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Coordinator ABORT logged, crash before participant ABORT delivery → recovery replays ABORT | ✅ PASS |
| **TXC-11** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Multiple recovery cycles on same txn → idempotent (same decision every time) | ✅ PASS |
| **TXC-12** | `tests/sharding/test_sharding_phase6_hardening.cpp` | In-doubt state with partial votes → recovery determines majority and commits/aborts accordingly | ✅ PASS |
| **FLR-08** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Multiple coordinator restarts on same txn → same decision (idempotent) | ✅ PASS |
| **FLR-09** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Coordinator restart after COMMIT decision logged → re-drives participant delivery idempotently | ✅ PASS |
| **FLR-10** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Coordinator restart after ABORT decision logged → re-drives participant delivery idempotently | ✅ PASS |
| **FI-16** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Coordinator failure during prepare phase → WAL-driven recovery re-drives correctly | ✅ PASS |
| **FI-17** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Coordinator failure during commit phase → exactly-once decision delivery | ✅ PASS |
| **FI-18** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | New coordinator takes over after old coordinator failure → reads WAL and completes txn | ✅ PASS |

### Implementation Details

**File:** `include/sharding/cross_shard_transaction.h`, `src/sharding/cross_shard_transaction.cpp`

**Key Method:** `Status CrossShardTransactionCoordinator::recoverFromWAL()`

**Idempotency Mechanism:**
- WAL entries are tagged with unique `txn_id`
- Re-entry into recovery with same `txn_id` is idempotent by LSN (log sequence number)
- Decision (COMMIT/ABORT) is written atomically to WAL as single entry
- Repeated recovery calls do not re-write the decision or change participant state

**Durability Guarantee:**
- Decision durability is guaranteed by `TransactionWAL::append()` with fsync before return
- No participant is contacted until decision is durable in WAL

---

## 2. Participant Recovery (`two_phase_commit_participant::recoverFromWAL()`)

### Requirement

**Idempotency Guarantee:** Participant ignores repeated COMMIT/ABORT on already-committed/aborted state
(no state mutation, no error).

### Verification Evidence

| Test ID | File | Criteria | Status |
|---------|------|----------|--------|
| **TXC-13** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Participant receives COMMIT while already COMMITTED → idempotent, no error | ✅ PASS |
| **TXC-14** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Participant receives ABORT while already ABORTED → idempotent, no error | ✅ PASS |
| **TXC-15** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Participant PREPARED state after crash → recovery applies WAL entry, reaches COMMITTED/ABORTED state | ✅ PASS |
| **TXC-16** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Multiple COMMIT deliveries to participant → same result (idempotent) | ✅ PASS |
| **FLR-01** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Coordinator crash during Phase-2 → participants stay PREPARED → recovery on coordinator re-drives COMMIT | ✅ PASS |
| **FLR-02** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Participant isolation: one participant crashes, another continues normally | ✅ PASS |
| **FLR-03** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Participant crash after COMMIT logged → recovery replays COMMITTED state | ✅ PASS |
| **FLR-04** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Multiple participant restarts on same txn → consistent state | ✅ PASS |
| **FI-19** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Participant failure + recovery during 2PC → state machine correctly reconstructed | ✅ PASS |
| **FI-20** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Cascade failure: coordinator + participant crash, both recover → consistent outcome | ✅ PASS |

### Implementation Details

**File:** `include/sharding/two_phase_commit_participant.h`, `src/sharding/two_phase_commit_participant.cpp`

**Key Method:** `Status TwoPhaseCommitParticipant::recoverFromWAL()`

**Idempotency Mechanism:**
- Participant state machine is stored in WAL entry (PREPARED, COMMITTED, ABORTED)
- Recovery reads WAL and transitions to final state (COMMITTED or ABORTED)
- Subsequent COMMIT/ABORT RPC delivery to already-final state is accepted without error
- No side effects occur for repeated delivery (e.g., no double-write of results)

**Durability Guarantee:**
- Participant log state changes are flushed to WAL before acknowledgment to coordinator
- WAL replay is deterministic and produces same state regardless of replay count

---

## 3. Transaction WAL Replay (`transaction_wal::replay()`)

### Requirement

**Idempotency Guarantee:** Append idempotent by LSN — replaying from same LSN produces same state
regardless of how many times replay is called.

### Verification Evidence

| Test ID | File | Criteria | Status |
|---------|------|----------|--------|
| **FI-24** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Recovery is idempotent — repeated recovery calls on same WAL produce same outcome | ✅ PASS |
| **FI-39** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Multiple WAL replays on same transaction set → all replays converge to same final state | ✅ PASS |
| **FI-40** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Stress validation: No txn is both COMMITTED and ABORTED (consistency invariant verified) | ✅ PASS |
| **TXC-09** | `tests/sharding/test_sharding_phase6_hardening.cpp` | WAL entry for decision survives crash and is replayed exactly once | ✅ PASS |
| **TXC-11** | `tests/sharding/test_sharding_phase6_hardening.cpp` | Multiple recovery cycles read same WAL entries → same state | ✅ PASS |

### Implementation Details

**File:** `include/sharding/transaction_wal.h`, `src/sharding/transaction_wal.cpp`

**Key Method:** `Status TransactionWAL::replay(LSN start_lsn, ReplayCallback callback)`

**Idempotency Mechanism:**
- Each WAL entry is tagged with unique LSN
- Replay starts from specified LSN and processes entries sequentially
- Re-entry with same `start_lsn` reads same set of entries and applies same transformations
- Callback is stateless — side effects are deterministic (state transitions only)

**Durability Guarantee:**
- WAL entries are append-only and immutable
- LSN ordering is strictly maintained
- Replay callback must be idempotent (does not assume new state on repeated invocation)

---

## 4. Replication Log Application (`replication_manager::applyFrom(LSN)`)

### Requirement

**Idempotency Guarantee:** Re-apply from LSN is safe — replication log entries can be applied
multiple times without changing outcome.

### Verification Evidence

| Test ID | File | Criteria | Status |
|---------|------|----------|--------|
| **FI-21** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Replication manager applies transaction state → crash/recovery replays same entries idempotently | ✅ PASS (via cross-module integration) |
| **FI-22** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Quorum commit-index advancement → replicas apply same entries in same order | ✅ PASS (via cross-module integration) |
| **Wave 8b** | `benchmarks/wave8/w8b_stress_soak_stability_test.cpp` | Long-duration failover and promotion scenarios exercise replication log replay multiple times | ✅ PASS |

### Implementation Details

**File:** `include/replication/replication_manager.h`, `src/replication/replication_manager.cpp`

**Key Method:** `Status ReplicationManager::applyFrom(LSN start_lsn)`

**Idempotency Mechanism:**
- Replication log entries contain committed state snapshots and WAL append positions
- Application from LSN is idempotent because same entries are re-applied deterministically
- Quorum commit-index progression is monotonically increasing — re-application does not regress state

**Durability Guarantee:**
- Log entries are persisted to disk before acknowledgment to primary
- Replica state transitions are logged for recovery

**Cross-Module Invariant:**
- Replication applies entries that may contain transaction WAL state
- Must not conflict with transaction WAL replay idempotency (§3 above)
- Integration test: `FI-21/FI-22` verify both layers apply consistently

---

## 5. Raft Membership Entry Application (`raft_v2::applyEntry()`)

### Requirement

**Idempotency Guarantee:** Raft log replay is deterministic — applying same membership entries
in same order produces same configuration state regardless of how many times replay is invoked.

### Verification Evidence

| Test ID | File | Criteria | Status |
|---------|------|----------|--------|
| **FI-23** | `tests/sharding/test_sharding_p6_fault_injection.cpp` | Raft membership entry (JOINT → COMMIT) applied multiple times → deterministic configuration | ✅ PASS (via cross-module integration) |
| **Wave 9c** | `benchmarks/wave9/w9c_security_hardening_test.cpp` | Cluster topology changes + replicas re-apply membership entries → consistent ring state | ✅ PASS |

### Implementation Details

**File:** `include/replication/raft_v2.h`, `src/replication/raft_v2.cpp`

**Key Method:** `Status RaftV2::applyEntry(const RaftEntry& entry)`

**Idempotency Mechanism:**
- Raft entries contain term and index — replay checks these and idempotently applies
- Membership entries (JOINT→COMMIT transitions) are applied only once (duplicate detection)
- Configuration state transitions are logged for audit trail

**Durability Guarantee:**
- Raft log is persisted before entry is applied
- Re-application from same log position deterministically reconstructs configuration

---

## Cross-Module Synchronization Rules

To maintain the idempotency contract across module boundaries, the following rules are enforced:

1. **No mutual WAL writes:** Transaction WAL and Replication WAL must not write to the same log stream.
   - Transaction WAL writes via `TransactionWAL` (sharding module)
   - Replication WAL writes via `ReplicationManager::WALManager` (replication module)
   - Raft membership changes write to Raft log (replication module)

2. **LSN isolation:** LSN sequences are per-module-per-log:
   - `TransactionWAL::LSN` for transaction coordination
   - `ReplicationManager::WALManager::LSN` for state replication
   - `RaftV2::LogIndex` for membership

3. **Recovery priority:** On cluster recovery:
   - Raft log is applied first (determines cluster topology and commit index)
   - Replication log is applied second (determines replicated state)
   - Transaction WAL is replayed last (only affects in-doubt transactions)

4. **Idempotency contract enforcement:**
   - Each recovery surface must accept repeated invocation with same parameters
   - No "double application" detection is required; idempotency is guaranteed by design
   - Integration tests (FI-21..FI-40) exercise multiple recovery cycles to verify

---

## Formal Acceptance Statement

**Document Type:** Cross-Module Recovery Contract Verification  
**Scope:** v2.4.0 GA (v1.9.0 boundary)  
**Evidence Completeness:** 100% (all 5 surfaces verified)  
**Status:** ✅ VERIFIED — All idempotency and durability guarantees confirmed by test suite

**This document completes the Batch B boundary evidence attachment requirement.**

---

*Document generated 2026-08-01. Linked from `SHARDING_P6_SIGN_OFF.md` as cross-module recovery contract verification.*
