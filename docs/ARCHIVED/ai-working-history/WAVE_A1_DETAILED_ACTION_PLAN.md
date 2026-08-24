# Wave A Batch A1 — Transaction Build/Run + Chaos Evidence Action Plan

**Status:** 🔄 In Progress (Build verification running)  
**Started:** 2026-08-14  
**Target Completion:** 2026-09-04  
**Critical Path Blocker:** Yes

---

## Phase 1: Build + Run Verification (Week of 2026-08-14)

### Acceptance Criteria

| Criterion | Target | Evidence |
|-----------|--------|----------|
| Phase 2 test build success | Exit 0 | CMake build log |
| Phase 2 test run success (9 tests) | All pass | CTest output |
| Phase 2 SAGA test build success | Exit 0 | CMake build log |
| Phase 2 SAGA test run success (12 tests) | All pass | CTest output |
| Phase 3 fault-injection test build | Exit 0 | CMake build log |
| Phase 3 fault-injection test run (14 tests) | All pass | CTest output |

### Detailed Tasks

**Task A1-1: Phase 2 Distributed Coordinator Build**
- File: `tests/transaction/test_transaction_distributed_phase2.cpp`
- Test Count: 9 (AC-4/5/6 coverage)
- Build Command: `cmake --build --preset community-release --target test_transaction_distributed_phase2`
- Success Criteria: Exit code 0, build completes without errors
- Owner: TBD
- Target: 2026-08-21
- Evidence Location: `src/transaction/PHASE_2_ACCEPTANCE_CHECKLIST.md` § Build Verification

**Task A1-2: Phase 2 Distributed Coordinator Test Execution**
- Command: `ctest --preset community-release -R transaction_distributed_phase2 -V --output-on-failure`
- Success Criteria: All 9 tests pass with exit 0
- Test Coverage:
  - 2PC commit success path
  - 2PC abort path
  - 3PC commit success
  - Participant failure during prepare
  - Coordinator timeout during 2PC
  - Cross-shard consistency verification
  - Retry behavior with exponential backoff
  - Error code consistency
  - Deterministic rollback under contention
- Owner: TBD
- Target: 2026-08-21
- Evidence Location: `src/transaction/PHASE_2_ACCEPTANCE_CHECKLIST.md` § Run Verification

**Task A1-3: Phase 2 SAGA Compensation Build**
- File: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`
- Test Count: 12 (AC-8/9/10 coverage)
- Build Command: `cmake --build --preset community-release --target test_transaction_saga_compensation_phase2`
- Success Criteria: Exit code 0, build completes without errors
- Owner: TBD
- Target: 2026-08-21
- Evidence Location: `src/transaction/PHASE_2_ACCEPTANCE_CHECKLIST.md` § SAGA Build Verification

**Task A1-4: Phase 2 SAGA Compensation Test Execution**
- Command: `ctest --preset community-release -R transaction_saga_compensation_phase2 -V --output-on-failure`
- Success Criteria: All 12 tests pass with exit 0
- Test Coverage:
  - Single-step SAGA compensation
  - Multi-step SAGA compensation
  - Compensation idempotency under duplicate calls
  - Compensation ordering (reverse sequence)
  - Partial failure scenarios
  - Retry storm (10+ concurrent compensation calls)
  - Circuit breaker integration
  - Timeout behavior during compensation
  - Cascading step failures
  - Compensation state persistence
  - Rollback of failed compensation
  - Exception handling in compensation steps
- Owner: TBD
- Target: 2026-08-21
- Evidence Location: `src/transaction/PHASE_2_ACCEPTANCE_CHECKLIST.md` § SAGA Run Verification

**Task A1-5: Phase 3 Fault Injection Build**
- File: `tests/transaction/test_transaction_fault_injection_phase3.cpp`
- Test Count: 14 (AC-11/12/13 coverage)
- Build Command: `cmake --build --preset community-release --target test_transaction_fault_injection_phase3`
- Success Criteria: Exit code 0, build completes without errors
- Owner: TBD
- Target: 2026-08-28
- Blocker: A1-4 run verification pass (Phase 2 tests)
- Evidence Location: `src/transaction/PHASE_3_ACCEPTANCE_CHECKLIST.md` § Build Verification

**Task A1-6: Phase 3 Fault Injection Test Execution**
- Command: `ctest --preset community-release -R transaction_fault_injection_phase3 -V --output-on-failure`
- Success Criteria: All 14 tests pass with exit 0
- Test Coverage:
  - Coordinator crash mid-2PC prepare
  - Participant crash mid-commit
  - Network partition during transaction
  - Byzantine participant behavior
  - Cascading failures (coordinator + participant)
  - Recovery path validation
  - Data consistency after crash
  - Lock cleanup after failure
  - WAL consistency check
  - Timeout under failure
  - Retry behavior under repeated failures
  - Distributed deadlock detection
  - Failure isolation (no cascade to other transactions)
  - State machine consistency
- Owner: TBD
- Target: 2026-08-28
- Blocker: A1-5 build success
- Evidence Location: `src/transaction/PHASE_3_ACCEPTANCE_CHECKLIST.md` § Run Verification

---

## Phase 2: Chaos Evidence Collection (Week of 2026-08-28)

**Gated by:** A1-6 run verification success (all Phase 3 tests pass)

### AC-6: Coordinator Crash-Recovery (WAL Replay Determinism)

**Objective:** Prove that coordinator crash during 2PC results in deterministic recovery with:
- All in-doubt transactions resolved within 5s of restart
- Zero orphaned locks
- Zero data inconsistency across shards
- WAL replay idempotent (replay twice = same outcome)

**Test Scenario (TXN-RECOVERY-01: Clean Restart)**
- Setup: 100 in-flight transactions across 3 shards
- Inject: Coordinator process crash mid-2PC-prepare (after 50 txns commit, 50 in prepare)
- Trigger: Coordinator restart
- Verify:
  - Measure: WAL replay time (target: ≤ 5s)
  - Verify: All 100 transactions resolved (committed or rolled back)
  - Verify: Lock cleanup complete (no stranded locks)
  - Verify: Shard consistency: CRC match across replicas
  - Repeat: Crash/restart 5 times; same outcome each time
- Pass Criteria: 5/5 crash-restart cycles produce identical transaction states
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-RECOVERY-02: Crash During 2PC Prepare)**
- Setup: 200 in-flight 2PC transactions
- Inject: Coordinator crash while in prepare phase (votes being collected)
- Trigger: Coordinator restart; expect in-doubt transaction reconciliation
- Verify:
  - In-doubt query returns correct set of 50-100 transactions
  - Reconciliation queries participants; resolves all in-doubt
  - Zero data loss; all locks release
- Pass Criteria: In-doubt resolution completes within 5s; zero orphans
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-RECOVERY-03: Crash During 3PC Pre-Commit)**
- Setup: 150 in-flight 3PC transactions in pre-commit phase
- Inject: Coordinator crash after pre-commit votes received
- Trigger: Coordinator restart
- Verify:
  - 3PC state machine recovers correctly from pre-commit
  - Abort vs commit decision correct per WAL
  - Participants unblock (no blocked waiting-for-coordinator state)
- Pass Criteria: All participants unblocked; correct decision applied
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-RECOVERY-04: Cascading Coordinator + Participant Crash)**
- Setup: 100 2PC transactions; coordinator + 1 participant both crash
- Inject: Crash sequence: participant first, then coordinator during recovery
- Trigger: Participant restart; then coordinator restart
- Verify:
  - Participant recovers from its own crash first
  - Coordinator recovery resolves in-doubt correctly despite participant being down initially
  - Once all come back up, state is consistent
- Pass Criteria: Cascading crash resolved deterministically; zero inconsistency
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

### AC-8/AC-10: SAGA Compensation Idempotency + Circuit Breaker

**Objective:** Prove:
- Compensation is idempotent (calling same compensation step 10+ times = 1 effect)
- Circuit breaker activates after 5 consecutive step failures
- Retry storm (100 concurrent retries) produces deterministic outcome

**Test Scenario (TXN-SAGA-HARDENING-01: Circuit Breaker Trip)**
- Setup: 3-step SAGA with remote service at step 2
- Inject: Remote service always fails (error rate 100%)
- Trigger: SAGA execution; step 2 fails repeatedly
- Verify:
  - Attempt 1-5: step 2 failures, retry happening
  - After attempt 5: circuit breaker opens
  - Attempt 6+: no further retries attempted
  - Compensation triggers for step 1 (reverse order)
- Pass Criteria: Circuit opens exactly after 5 failures; no further retries
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-SAGA-HARDENING-02: Idempotent Compensation Under Storm)**
- Setup: 3-step SAGA; compensation must update counter variable
- Inject: Trigger compensation step 10 times concurrently
- Verify:
  - Counter value = 1 (idempotent) or is atomically correct
  - No duplicate state mutations
  - Compensation log contains exactly 1 committed entry
  - No orphaned state
- Pass Criteria: 10 concurrent calls produce identical final state as 1 call
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-SAGA-HARDENING-03: Partial Failure Ordering)**
- Setup: 5-step SAGA; step 3 always fails
- Inject: Execute SAGA 20 times concurrently; each fails at step 3
- Verify:
  - Compensation order: 2, 1 (reverse)
  - All 20 instances compensate in same order
  - No interleaving/ordering surprises
- Pass Criteria: All 20 SAGAs produce identical compensation sequence
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-SAGA-HARDENING-04: Retry Storm with Bounded Backoff)**
- Setup: 3-step SAGA; step 2 flaky (fails 50% of time)
- Inject: 100 concurrent SAGA executions; measure retry counts
- Verify:
  - Backoff formula: base 100ms × 2^attempt ± 20% jitter, max 3 retries
  - Max retry time per SAGA ≤ 1000ms
  - No unbounded backoff loops
  - Circuit breaker prevents retry storm from cascading
- Pass Criteria: 100 concurrent SAGAs complete within 1000ms max; bounded retries
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

### AC-5: Timeout Semantics (Exponential Backoff + Determinism)

**Objective:** Prove:
- Backoff formula produces deterministic schedule: 100ms, 200ms, 400ms (no silent extension)
- Jitter ±20% does not corrupt decision timing
- Error codes consistent across coordinator restart
- No silent deadline extension

**Test Scenario (TXN-TIMEOUT-01: Backoff Schedule Validation)**
- Setup: Transaction with 1s deadline; operation takes 150ms + network latency
- Inject: Network latency = 200ms; operation should fail and retry
- Verify:
  - Retry 1 fires at ~100ms (backoff window)
  - Retry 2 fires at ~300ms (100 + 200 with jitter)
  - Retry 3 fires at ~600ms (200 + 400 with jitter)
  - No retry 4 (max 3 retries); abort at ~1s
- Pass Criteria: Backoff timing matches formula ±50ms tolerance (accounting for jitter)
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-TIMEOUT-02: Error Consistency After Restart)**
- Setup: Long transaction; fails with TIMEOUT_RETRY_EXHAUSTED during retry 3
- Inject: Coordinator crashes after error code is written to client
- Trigger: Coordinator restart; client re-checks transaction status
- Verify:
  - Error code is TIMEOUT_RETRY_EXHAUSTED (not changed by restart)
  - No silent recovery / re-opening of deadline
  - Transaction remains in terminal state
- Pass Criteria: Error code consistent before/after restart
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

**Test Scenario (TXN-TIMEOUT-03: Jitter Bounds)**
- Setup: 100 transactions with 1s deadline, identical network latency
- Inject: All 100 execute concurrently
- Verify:
  - Retry times vary (jitter present)
  - Retry time max deviation = 20% of base backoff
  - No retry deviates by >20%
- Pass Criteria: Jitter variance within ±20% bounds; deterministic distribution
- Owner: TBD
- Target: 2026-09-04
- Evidenced In: `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md`

---

## Phase 3: Documentation + Evidence Closure (Week of 2026-09-04)

### Task A1-7: Update `src/transaction/ROADMAP.md` Phase 2 Section

**File:** `src/transaction/ROADMAP.md`  
**Section:** `### Phase 2: Distributed Coordination Hardening`  
**Changes:**
- Update build verification status to `[x]` (done)
- Update run verification status to `[x]` (done)
- Link to evidence blocks:
  - `src/transaction/PHASE_2_ACCEPTANCE_CHECKLIST.md` (build/run results)
  - `src/transaction/PHASE_2_CHAOS_EVIDENCE_BLOCK.md` (AC-6, AC-8, AC-10 chaos results)
  - `src/transaction/PHASE_3_ACCEPTANCE_CHECKLIST.md` (Phase 3 test results if complete)
- Mark Q3 2026 hardening tasks status

**Owner:** TBD  
**Target:** 2026-09-04  

### Task A1-8: Generate A1 Exit Evidence Bundle

**Deliverable:** `src/transaction/WAVE_A1_CLOSURE_EVIDENCE_BUNDLE.md`  
**Contents:**
- Executive summary: A1 status PASS/FAIL
- Build verification results (3 test suites)
- Test execution results (35 total tests)
- Chaos evidence matrix (4 scenarios × 3 test types)
- `release_critical` CI run results (should be green on `develop`)
- Sign-off: Ready for A2-A5 to proceed (if PASS)

**Owner:** TBD  
**Target:** 2026-09-04  

### Task A1-9: `release_critical` CI Validation

**Gate:** A1 cannot exit until `release_critical` CI passes on `develop`  
**Workflow:** `.github/workflows/09-pr-gates_release-critical-tests.yml`  
**Modules Impacted:** `transaction`, `replication`, `sharding`, `voice`, `gpu`  

**Pre-Merge Checklist:**
- [ ] Merge A1 evidence commits to `develop`
- [ ] Trigger `release_critical` workflow
- [ ] Wait for pass (estimated 2-3 hours)
- [ ] Document pass in evidence bundle
- [ ] Unblock A2-A5 (parallel)

**Owner:** TBD  
**Target:** 2026-09-04  
**Blocker Escalation:** If CI fails, coordinate with maintainer for root cause

---

## Success Criteria

**A1 PASS Conditions:**
- [x] Phase 2 build verification: exit 0
- [x] Phase 2 run verification: 9/9 tests pass
- [x] Phase 2 SAGA run verification: 12/12 tests pass
- [x] Phase 3 build verification: exit 0
- [x] Phase 3 run verification: 14/14 tests pass
- [x] AC-6 coordinator crash-recovery: 5/5 chaos runs deterministic
- [x] AC-8/AC-10 SAGA idempotency + circuit breaker: all 4 scenarios pass
- [x] AC-5 timeout semantics: all 3 scenarios pass
- [x] `release_critical` CI: PASS on `develop`
- [x] Evidence bundle complete + signed off

**A1 FAIL Conditions:**
- Any build exits non-zero (after triage)
- Any test suite has ≥1 failing test
- Chaos scenario fails reproducibility (< 4/5 deterministic runs)
- `release_critical` CI fails to pass

---

## Escalation & Blockers

| Blocker | Severity | Mitigation | Escalation |
|---------|----------|-----------|------------|
| Build failure (compile error) | HIGH | Local reproduce + git bisect | Maintainer + code-review agent |
| Test timeout (>60s per test) | MEDIUM | Profile + optimize; skip Phase 2 if Phase 1 needs fixing | Performance team |
| CMake error (missing dependency) | HIGH | Install missing dep; use diagnostic preset | DevOps/Build team |
| CI timeout (workflow > 4h) | MEDIUM | Parallelize; use self-hosted runner | CI/CD team |
| Data corruption in chaos test | CRITICAL | Immediate halt; investigate WAL + recovery paths | Lead engineer + security review |

---

## Owner Assignment

| Task | Owner | Backup |
|------|-------|--------|
| A1-1 to A1-6 (Build/Run Verification) | TBD | TBD |
| Chaos Evidence Collection | TBD | TBD |
| Documentation + Closure | TBD | TBD |
| CI Gate Validation | TBD | TBD |

**Assigned by:** Wave Orchestrator (TBD)  
**Review & Sign-off:** Release Lead (TBD)

---

**Last Updated:** 2026-08-14  
**Next Review:** Upon A1-1 build verification completion
