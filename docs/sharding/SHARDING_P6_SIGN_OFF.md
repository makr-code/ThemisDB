---
Author: ThemisDB Contributors
Created: 2026-07-22
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level4
SOT_Domain: module-behavior
Canonical_References:
  - ../../ROADMAP.md
  - ../../src/sharding/ROADMAP.md
  - ../../include/sharding/detail/two_phase_commit_contract.h
  - ../../src/sharding/cross_shard_transaction.cpp
  - ../../src/sharding/two_phase_commit_coordinator.cpp
  - ../../tests/test_two_phase_commit.cpp
  - ../governance/SHARDING_P6_RESIDUAL_RISK_ACCEPTANCE.md
Related_Issue: "#5373 (DIS-05)"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Phase 6 (P6-01 / P6-02 / P6-03) — Sign-Off Evidence Bundle

<!-- Status: historical evidence bundle; current DIS-05 status is in the dated addendum | generated: 2026-07-22 -->
<!-- Primary sources: ROADMAP.md for current aggregate; implementation/contracts and tests for behavior; src/sharding/ROADMAP.md §Phase 4 for historical P6 tracking -->
<!-- Gate: release_critical label on develop -->

## Summary

Block E (Sharding P6) delivers the final distributed-transaction consistency and fault-injection
hardening required for the v1.9.0 GA release. This document captures evidence that all three
P6 deliverables meet their acceptance criteria and are cleared for the GA gate.

> **Record status:** This is the historical 2026-07-22 P6 evidence bundle. Its original evidence
> and generated-date record are retained; the current DIS-05 scope/status is documented separately
> in the addendum below and does not establish current CTest or release-gate results.

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
| P6-01-B | Standalone `TwoPhaseCommitCoordinator`: in-doubt decision re-driven via WAL after restart | TXC-09..TXC-12 |
| P6-01-C | 2PC: idempotent COMMIT/ABORT delivery (double-delivery safe) | TXC-13..TXC-16 |
| P6-01-D | 3PC: PreCommit phase before final COMMIT; fail-closed on PreCommit failure | TXC-17..TXC-22 |
| P6-01-E | Percolator: TrueTime edge-cases and primary-lock acquisition | TXC-23..TXC-26 |
| P6-01-F | Calvin: deterministic execution ordering guaranteed | TXC-27..TXC-32 |

### WAL/Recovery Contract

- **Standalone coordinator durability:** `TwoPhaseCommitCoordinator` writes its COMMIT/ABORT decision to WAL **before** participant delivery; recovery re-drives and resolves only after all acknowledgements and a completion record.
- **Cross-shard coordinator recovery:** `CrossShardTransactionCoordinator::recoverFromWAL()` restores state and reports resume candidates; it does not automatically re-drive PREPARE/COMMIT/ABORT RPCs.
- **Participant idempotency:** repeated COMMIT or ABORT on a participant in COMMITTED/ABORTED state is a no-op (no state mutation, no error).
- **In-doubt semantics:** a durable decision is authoritative during `TwoPhaseCommitCoordinator` WAL recovery; if no decision was recorded, that coordinator conservatively aborts. `CrossShardTransactionCoordinator` recovery restores state and reports candidates but does not automatically re-drive phase two.
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
| `cross_shard_transaction` (Coordinator) | `recoverFromWAL()` | Restores state/reports candidates; no automatic phase-two re-drive | COMMIT or ABORT before participant fan-out |
| `two_phase_commit_participant` | `recoverFromWAL()` | Yes — participant ignores repeated COMMIT/ABORT | COMMITTED/ABORTED after state transition |
| `transaction_wal` | `replay()` | Yes — append idempotent by LSN | All protocol phase changes logged |
| `replication_manager` (WAL) | `applyFrom(LSN)` | Yes — re-apply from LSN is safe | Quorum commit-index entries |
| `raft_v2` (membership) | `applyEntry()` | Yes — Raft log replay is deterministic | JOINT → COMMIT membership entries |

**Rule:** No module-local WAL layer may change its `fsync`/`retention`/`replay` assumptions
without a cross-module review. Diverging durability policies are a P0 blocker.

### Boundary Evidence Attachment

**Link:** [`SHARDING_P6_CROSS_MODULE_RECOVERY_VERIFICATION.md`](./SHARDING_P6_CROSS_MODULE_RECOVERY_VERIFICATION.md)

The formal verification of this cross-module recovery contract is documented in the
cross-module recovery verification document. All 5 recovery surfaces have been verified
for idempotency and durability guarantees via targeted integration tests (TXC-09..12,
FLR-01..10, FI-16..24, FI-39..40). This completes the Batch B boundary evidence attachment
requirement.

---

## Residual Risks

| Risk | Severity | Status |
|------|----------|--------|
| DIS-05 execution divergence for `CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator` | MEDIUM | Closed for the named pair by `include/sharding/detail/two_phase_commit_contract.h`; transport/WAL/recovery remain separate. This does not cover the third 2PC-capable `DistributedTransactionCoordinator` or establish repo-wide convergence. See status update below. |
| WAL-layer drift across sharding + replication boundaries | LOW | Cross-module recovery contract documented above; no divergence detected in current codebase |
| Production re-drive timing enforcement (currently simulated synchronously) | LOW | Real-time re-drive timer integration required before cluster deployment; tracked in sharding ROADMAP Phase 5 |

---

### Current status update — 2026-11-01 (DIS-05)

The shared contract defines prepare → configured decision record → phase two; failed phase-two
acknowledgements do not convert a decision into successful completion. It covers only the named
pair and does not unify their transports, transaction lifecycles, WAL formats, or recovery. It also
does not cover `DistributedTransactionCoordinator` or establish repo-wide 2PC convergence.
`tests/test_two_phase_commit.cpp`
covers failed COMMIT and ABORT acknowledgements through both named public paths. The prior verification note recorded that the affected source and test translation units compiled,
but `TwoPhaseCommitFocusedTests` could not run because CTest's on-demand build was blocked by missing
`pugixml.hpp` in unrelated `src/security/xxe_safe_xml_parser.cpp`. This documentation-only
synchronization did not rerun a build or tests.
CrossShard WAL/snapshot recovery restores state and reports resume candidates but does not
automatically re-drive phase two. Recovery/WAL unification remains DIS-06 follow-up scope.
`DistributedTransactionCoordinator`
(`include/sharding/distributed_transaction.h`) is a separate 2PC-capable coordinator outside
DIS-05. This update supplements the original sign-off; it does not alter the historical approval
or generated-date record below.

*Document generated 2026-07-22. Human approver sign-off required in `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 before GA promotion.*
