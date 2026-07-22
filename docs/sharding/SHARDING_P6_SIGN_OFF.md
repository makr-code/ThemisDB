# Sharding Phase 6 (P6-01 / P6-02 / P6-03) — Sign-Off Evidence Bundle

<!-- Status: current | generated: 2026-07-22 -->
<!-- Primary (Quelle der Wahrheit): src/sharding/ROADMAP.md §Phase 4 -->
<!-- Gate: release_critical label on develop -->

## Summary

Block E (Sharding P6) delivers the final distributed-transaction consistency and fault-injection
hardening required for the v1.9.0 GA release. This document captures evidence that all three
P6 deliverables meet their acceptance criteria and are cleared for the GA gate.

---

## P6-01 — 2PC/3PC Consistency Verification

**Status:** ✅ PASS  
**Test file:** `tests/sharding/test_sharding_phase6_hardening.cpp`  
**Test IDs:** TXC-01..TXC-32 (32 cases)  
**CTest label:** `release_critical sharding_p6`  
**Seed:** `kCanonicalSeed = 42`

### Acceptance Criteria

| ID | Criterion | Evidence |
|----|-----------|----------|
| P6-01-A | 2PC: coordinator drives PREPARE → vote collection → COMMIT/ABORT correctly | TXC-01..TXC-08 |
| P6-01-B | 2PC: in-doubt state re-driven via WAL replay after coordinator restart | TXC-09..TXC-12 |
| P6-01-C | 2PC: idempotent COMMIT/ABORT delivery (double-delivery safe) | TXC-13..TXC-16 |
| P6-01-D | 3PC: PreCommit phase before final COMMIT; fail-closed on PreCommit failure | TXC-17..TXC-22 |
| P6-01-E | Percolator: TrueTime edge-cases and primary-lock acquisition | TXC-23..TXC-26 |
| P6-01-F | Calvin: deterministic execution ordering guaranteed | TXC-27..TXC-32 |

### WAL/Recovery Contract

- **Coordinator durability:** decision (COMMIT or ABORT) is written to WAL **before** any participant delivery; crash-restart replays from WAL to completion.
- **Participant idempotency:** repeated COMMIT or ABORT on a participant in COMMITTED/ABORTED state is a no-op (no state mutation, no error).
- **In-doubt semantics:** PREPARE logged without COMMIT/ABORT → coordinator is considered in-doubt; recovery re-drives COMMIT if all votes were YES.
- **3PC fail-closed:** failure to deliver PreCommit to any participant triggers ABORT (not COMMIT).

---

## P6-02 — Failover Logic and Recovery-Path Hardening

**Status:** ✅ PASS  
**Test file:** `tests/sharding/test_sharding_phase6_hardening.cpp`  
**Test IDs:** FLR-01..FLR-20 (20 cases)  
**CTest label:** `release_critical sharding_p6`  
**Seed:** `kCanonicalSeed = 42`

### Acceptance Criteria

| ID | Criterion | Evidence |
|----|-----------|----------|
| P6-02-A | Coordinator crash during Phase-2 → participants stay PREPARED → recovery re-drives COMMIT | FLR-01..FLR-04 |
| P6-02-B | Coordinator crash after ABORT decision → recovery re-delivers ABORT | FLR-05..FLR-07 |
| P6-02-C | Multiple coordinator restarts on same txn → same decision (idempotent) | FLR-08..FLR-10 |
| P6-02-D | New coordinator reads WAL and drives to completion (leadership change) | FLR-11..FLR-13 |
| P6-02-E | Participant timeout detection → coordinator aborts on deadline expiry | FLR-14..FLR-16 |
| P6-02-F | Multi-txn environment: crash during one txn; others complete normally | FLR-17..FLR-20 |

### Re-drive Timing Guarantee

Coordinator must begin WAL-recovery re-drive within configurable `re_drive_timeout_ms`
(default 5,000 ms). The simulated coordinator in the test suite re-drives synchronously
on recovery call; production integration must meet the ≤5,000 ms SLA.

---

## P6-03 — Wave-8 Fault Injection

**Status:** ✅ PASS  
**Test file:** `tests/sharding/test_sharding_p6_fault_injection.cpp`  
**Test IDs:** FI-01..FI-40 (40 cases)  
**CTest label:** `release_critical sharding_p6 fault_injection`  
**Seed:** `kCanonicalSeed = 42`

### Acceptance Criteria

| Group | Criterion | Test IDs | Count |
|-------|-----------|----------|-------|
| Network Partition | Correct abort on partition; re-delivery on heal; no split-commit | FI-01..FI-15 | 15 |
| Coordinator Failure | WAL-driven recovery; exactly-once decision delivery; new coordinator takes over | FI-16..FI-25 | 10 |
| Cascade / Multi-Failure | Atomic abort across multi-failure; SAGA compensation; deterministic order; consistency invariant | FI-26..FI-40 | 15 |

### Key Invariants Verified

1. **No txn is both COMMITTED and ABORTED** (consistency invariant — FI-40 stress validation).
2. **Recovery is idempotent** (repeated recovery calls on same WAL produce same outcome — FI-24, FI-39).
3. **Orphan locks are released** after coordinator failure (FI-30, FI-36).
4. **SAGA compensation runs in reverse order** when forward step fails (FI-32).
5. **Calvin ordering is deterministic** across concurrent txn submissions (FI-35).

---

## Gate Integration

All three test suites are registered in `tests/sharding/CMakeLists.txt` under
`LABELS release_critical sharding_p6`. The `09-pr-gates_release-critical-tests.yml`
CI workflow includes the `release_critical` label in its CTest filter, meaning
every PR to `develop` that affects sharding code will trigger these suites.

```
CTest: -L "release_critical" --timeout 120 -j 1
```

---

## Cross-Module Recovery Contract

The following WAL/recovery surface mapping is established and must not diverge:

| Module | Recovery Driver | Idempotency Guarantee | WAL Entry on Decision |
|--------|----------------|----------------------|----------------------|
| `cross_shard_transaction` (Coordinator) | `recoverFromWAL()` | Yes — COMMITTED/ABORTED states absorb re-drive | COMMIT or ABORT before participant fan-out |
| `two_phase_commit_participant` | `recoverFromWAL()` | Yes — participant ignores repeated COMMIT/ABORT | COMMITTED/ABORTED after state transition |
| `transaction_wal` | `replay()` | Yes — append idempotent by LSN | All protocol phase changes logged |
| `replication_manager` (WAL) | `applyFrom(LSN)` | Yes — re-apply from LSN is safe | Quorum commit-index entries |
| `raft_v2` (membership) | `applyEntry()` | Yes — Raft log replay is deterministic | JOINT → COMMIT membership entries |

**Rule:** No module-local WAL layer may change its `fsync`/`retention`/`replay` assumptions
without a cross-module review. Diverging durability policies are a P0 blocker.

---

## Residual Risks

| Risk | Severity | Status |
|------|----------|--------|
| 2PC duplication (`CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator`) — two overlapping commit-orchestration surfaces | MEDIUM | Interface separation documented in `docs/architecture/transaction_coordinators.md`; consolidation scheduled for v2.1.0 |
| WAL-layer drift across sharding + replication boundaries | LOW | Cross-module recovery contract documented above; no divergence detected in current codebase |
| Production re-drive timing enforcement (currently simulated synchronously) | LOW | Real-time re-drive timer integration required before cluster deployment; tracked in sharding ROADMAP Phase 5 |

---

*Document generated 2026-07-22. Human approver sign-off required in `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 before GA promotion.*
