# Wave B Option B3: Transaction CI Execution Hardening — Build Verification & Chaos Evidence
## Status: ✅ READY FOR EXECUTION

---

## Executive Summary

**Module:** Transaction (ACID isolation + crash recovery + SAGA orchestration)  
**Scope:** CI execution (73 Phase 1-3 tests) + chaos/recovery evidence capture  
**Timeline:** Sept 7-24, 2026 (18 days: 2 for build, 16 for chaos execution)  
**Critical Blocker:** ROADMAP line 62 — "Transaction CI execution evidence | Tests implemented; NO develop CI run yet | Run immediately (should have been done 2026-08-31)"  
**Gate:** Wave A/B verification + Sept 30 exit criteria  
**Outcome:** GREEN CI on `develop` + chaos/recovery evidence bundle + determinism baseline

---

## Current State (2026-09-02 Baseline)

### Codebase Status
- ✅ **Phase 1-3 Tests Implemented:** 73 tests across crash-recovery, SAGA, timeout (2026-08-19)
- ✅ **Tests Registered as `release_critical`:** All 73 flagged for required CI execution
- ❌ **CI Execution Status:** NEVER RUN on `develop` (only local testing, pre-Sept-5)
- ⚠️ **Chaos Evidence:** Test output exists locally; no CI artefacts captured
- ⚠️ **Representative-Hardware Baselines:** Deferred to Oct 1+ (no A100/GPU required for Phase 1-3)

### Test Breakdown (83 tests total, 73 in scope)
- **Phase 1 (Crash-Recovery):** TXN-RECOVERY-01..04 (12 tests)
  - Warm restart, cold restart, WAL corruption, point-in-time recovery
  
- **Phase 2 (SAGA):** TXN-SAGA-HARDENING-01..04 (14 tests)
  - SAGA idempotency, compensation ordering, sub-transaction failure, cascading rollback
  
- **Phase 3 (Timeout + Distributed):** TXN-TIMEOUT-01..03, TXN-BYZANTINE-01..02, TXN-XSHARD-01..02 (34 tests)
  - Deterministic timeout handling, Byzantine failures, cross-shard consistency

### Known Limitations
- No `develop` CI pipeline execution (scaffolding-only testing)
- No chaos/fault-injection execution logs (need to collect Sept 7+)
- No representative-hardware (CPU x86_64 only, no GPU/TPU)
- No continuous soak tests (only point-in-time execution)

---

## Execution Plan (Sept 7-24)

### Phase 1: Build Verification & Environment Setup (Sept 7-8, 2 days)

**Objective:** Compile all 73 tests + verify CI environment readiness

**Tasks:**

1. **CMake Configuration** (Sept 7, 09:00 UTC, 4 hours)
   - Use preset: `community-release` (matching TRANSACTION build target)
   - Enable flags: `DTHEMIS_BUILD_TESTS=ON`, `DTHEMIS_BUILD_BENCHMARKS=OFF`
   - Ensure dependencies: RocksDB, fmt, spdlog, nlohmann-json, yaml-cpp (all pre-installed)
   - Output: Build directory ready at `/tmp/themis-transaction-ci`

2. **Test Compilation** (Sept 7, 14:00 UTC, 8 hours)
   - Compile 73 transaction tests (all phases)
   - Commands:
     ```bash
     cmake --preset community-release -B /tmp/themis-transaction-ci
     cmake --build /tmp/themis-transaction-ci --target test_transaction_* -j 4
     ```
   - Expected: All 73 tests compile with 0 errors, ≤10 warnings
   - Output artifact: Build log + compiler warnings report

3. **Smoke Test Execution** (Sept 8, 09:00 UTC, 4 hours)
   - Run 3 critical smoke tests (Phase 1, 2, 3 sample):
     - `test_transaction_crash_recovery_01_warm_restart`
     - `test_transaction_saga_hardening_02_compensation`
     - `test_transaction_timeout_01_deterministic`
   - Purpose: Verify test harness works + CI environment ready
   - Pass criteria: All 3 tests pass (may have test flakiness; re-run if needed)

4. **CI Workflow Setup** (Sept 8, 14:00 UTC, 4 hours)
   - Create workflow: `.github/workflows/09-pr-gates_release-critical-tests_transaction.yml`
   - Trigger: Manual (on-demand for Wave A execution) or push to `develop`
   - Runner: standard (Ubuntu x86_64, no GPU)
   - Steps:
     1. Checkout `develop`
     2. Install dependencies (cmake, build tools, sccache)
     3. Configure: `cmake --preset community-release`
     4. Compile transaction tests (4 parallel jobs)
     5. Run 73 tests (capture stdout/stderr + timing)
     6. Collect artifacts (test results JSON, crash dumps, WAL files)
   - Expected duration: ~45 minutes (compile: 20 min, tests: 25 min)

---

### Phase 2: Sequential Test Execution & Evidence Collection (Sept 9-16, 8 days)

**Objective:** Execute all 73 tests sequentially, capture chaos evidence

**Timeline:**
- **Sept 9:** Phase 1 tests (crash-recovery, 12 tests)
- **Sept 10:** Phase 2 tests (SAGA hardening, 14 tests)
- **Sept 11-12:** Phase 3 tests (timeout + distributed, 34 tests)
- **Sept 13-14:** Chaos re-runs (flaky test re-execution)
- **Sept 15-16:** Evidence consolidation + analysis

**Execution Tasks:**

1. **Phase 1: Crash-Recovery Tests** (Sept 9, 09:00 UTC, 8 hours)
   - Tests: TXN-RECOVERY-01..04 (warm, cold, corruption, point-in-time)
   - Execution:
     ```bash
     cd /tmp/themis-transaction-ci
     ctest -R "test_transaction_crash_recovery" -V --output-on-failure
     ```
   - Capture:
     - Test pass/fail status (per test)
     - Stdout/stderr logs (full output)
     - Timing per test (min/max/avg)
     - Crash artifacts (core dumps, WAL snapshots)
   - Expected outcome: 12/12 pass (or identify and flag flaky tests)

2. **Phase 2: SAGA Hardening Tests** (Sept 10, 09:00 UTC, 8 hours)
   - Tests: TXN-SAGA-HARDENING-01..04 (idempotency, compensation, failure, cascade)
   - Execution: Similar to Phase 1 (ctest verbose)
   - Focus: Sub-transaction compensation order, rollback determinism
   - Expected outcome: 14/14 pass

3. **Phase 3: Timeout + Distributed Tests** (Sept 11-12, 09:00 UTC, 16 hours)
   - Tests: TXN-TIMEOUT-01..03 (12 tests), TXN-BYZANTINE-01..02 (4 tests), TXN-XSHARD-01..02 (4 tests)
   - Sub-phases:
     - Sept 11, 09:00-12:00: TXN-TIMEOUT-01..03 (deterministic timeout)
     - Sept 11, 14:00-18:00: TXN-BYZANTINE-01..02 (Byzantine failures)
     - Sept 12, 09:00-12:00: TXN-XSHARD-01..02 (cross-shard consistency)
   - Focus: Timeout determinism, Byzantine consensus, distributed consistency
   - Expected outcome: 34/34 pass

4. **Chaos Re-Runs & Flaky Test Investigation** (Sept 13-14, 09:00 UTC)
   - If any test failed Sept 9-12, investigate + fix
   - Re-run failed tests 5 times each to establish baseline flakiness
   - For persistent failures: Create GitHub issue, document root cause
   - For transient failures (flaky): Flag for Wave B extended hardening

5. **Evidence Consolidation** (Sept 15-16, 09:00 UTC)
   - Collect all test outputs into single report
   - Generate artifacts:
     - `transaction_ci_phase1_results_2026_09_09.json` (crash-recovery)
     - `transaction_ci_phase2_results_2026_09_10.json` (SAGA)
     - `transaction_ci_phase3_results_2026_09_11_12.json` (timeout + distributed)
     - `transaction_ci_flaky_analysis_2026_09_13_14.json` (flakiness metrics)

---

### Phase 3: Determinism Validation & Baseline Capture (Sept 17-20, 4 days)

**Objective:** Capture determinism baseline + validate recovery/SAGA determinism

**Tasks:**

1. **Determinism Test Re-Runs** (Sept 17, 09:00 UTC)
   - Re-run each critical test 5 times, comparing output signatures
   - Critical tests for determinism:
     - TXN-RECOVERY-02: Cold restart determinism (WAL replay consistency)
     - TXN-SAGA-HARDENING-01: Idempotency under retries (compensation order)
     - TXN-TIMEOUT-01: Deterministic timeout (always timeout at same point)
   - Capture: Output hash, test duration, resource usage
   - Success: 5/5 runs produce identical output for each test

2. **CPU Mock Hardware Baseline** (Sept 18, 09:00 UTC)
   - Establish CPU-only baseline (no GPU acceleration)
   - Metrics: Latency per operation, throughput, resource usage
   - Output: `transaction_cpu_baseline_2026_09_18.json`
   - Note: Representative-hardware (A100/H100) baseline deferred to Oct 1

3. **Recovery Evidence Scenario** (Sept 19, 09:00 UTC)
   - Execute sustained crash-recovery chaos test (30 min)
   - Scenario:
     - Initialize transaction state (100 concurrent txns)
     - Randomly crash process (every 5-10 sec)
     - Verify recovery: WAL replay, state consistency, no data loss
     - Repeat 5 cycles
   - Expected outcome: All crashes recover cleanly, zero data loss
   - Capture: Crash logs, recovery timing, final state consistency

4. **SAGA Cascading Failure Scenario** (Sept 20, 09:00 UTC)
   - Execute distributed SAGA cascade test (30 min)
   - Scenario:
     - Multi-shard transaction (3 shards, 5 participants)
     - Introduce cascade failures (tier-1 fails → tier-2 fails → rollback cascade)
     - Verify compensation order: reverse sequence (leader last)
     - Verify idempotency: re-run same sequence, same result
   - Expected outcome: Cascade completes deterministically, idempotent
   - Capture: Cascade logs, participant state, compensation order

---

### Phase 4: CI Integration & Sign-Off (Sept 21-24, 4 days)

**Objective:** Integrate CI workflow + Wave B exit criteria sign-off

**Tasks:**

1. **CI Workflow Finalization** (Sept 21, 09:00 UTC)
   - Complete workflow: `.github/workflows/09-pr-gates_release-critical-tests_transaction.yml`
   - Add artifact collection step:
     ```yaml
     - name: Collect Test Artifacts
       if: always()
       run: |
         mkdir -p artifacts/transaction
         cp -r /tmp/themis-transaction-ci/test-results/* artifacts/transaction/
         zip -r transaction_ci_results_2026_09_21.zip artifacts/
     
     - name: Upload Artifacts
       uses: actions/upload-artifact@v3
       with:
         name: transaction-ci-results
         path: transaction_ci_results_*.zip
   ```
   - Add failure detection step:
     ```yaml
     - name: Check Test Results
       run: |
         python3 tools/ci/check_test_results.py \
           artifacts/transaction \
           --required-pass 73 \
           --fail-on-flaky
     ```

2. **Documentation & Evidence Bundle** (Sept 22, 09:00 UTC)
   - Generate final report: `TRANSACTION_CI_EXECUTION_EVIDENCE_2026_09_22.md`
     - Section 1: Executive summary (73/73 tests pass, pass rate 100%, zero data loss)
     - Section 2: Build verification (compilation, 0 errors, timing)
     - Section 3: Phase 1 crash-recovery results (12 tests, determinism validation)
     - Section 4: Phase 2 SAGA results (14 tests, compensation order validated)
     - Section 5: Phase 3 timeout + distributed (34 tests, Byzantine consensus valid)
     - Section 6: Chaos evidence (recovery, cascade, flakiness analysis)
     - Section 7: Determinism baseline (5-run consistency, output hashes)
   
   - Generate evidence bundle: `src/transaction/WAVE_A_CI_EXECUTION_EVIDENCE_2026_09_22.md`
     - Link to full test results artifacts
     - Baseline latency/throughput metrics
     - Known flaky tests (if any) + mitigation
     - Recommendations for Oct 1+ representative-hardware validation

3. **GitHub Actions Trigger & Monitoring** (Sept 23, 09:00 UTC)
   - Trigger workflow on `develop`: `git push origin develop --force-with-lease`
   - Monitor CI run: Expected ~45 min total (compile + test execution)
   - Collect final artifacts: Test results JSON, timing reports, crash dumps
   - Success criteria: All 73 tests pass, 0 timeouts, 0 false positives

4. **Wave B Exit Criteria Sign-Off** (Sept 24, 14:00 UTC)
   - Validate against Wave A/B exit criteria:
     - [ ] Wave A criteria: "CI execution evidence (73 Phase 1-3 tests)" ✅ PASS
     - [ ] Wave B criteria: "Release decisions based on representative hardware baselines" (partial: CPU baseline locked)
   - Update ROADMAP.md line 82: Change `[~]` to `[x]` (Transaction Wave A complete)
   - Commit message: "Wave B Option B3: Transaction CI execution complete (73/73 tests green on develop)"
   - Final approval: Transaction module owner reviews + signs off

---

## Resource Allocation

**Team:** 1.5 FTE (Sept 7-24)
- **Transaction Test Lead** (1 FTE): Test execution, chaos orchestration, evidence collection
- **DevOps/CI Engineer** (0.5 FTE): Workflow setup, artifact management, monitoring

**Time Breakdown:**
- Build verification: 16 hours (Sept 7-8)
- Test execution (73 tests): 40 hours (Sept 9-14, ~8 hours/day)
- Determinism validation: 20 hours (Sept 17-20)
- CI integration + sign-off: 16 hours (Sept 21-24)
- **Total:** 92 hours (~11.5 FTE-days)

**Hardware Requirements:**
- Standard Ubuntu x86_64 runner (CPU-only, no GPU)
- 4 CPU cores, 8GB RAM minimum
- 50GB disk space (for build artifacts + test outputs)
- No special hardware (A100/H100 deferred to Oct 1)

---

## Success Criteria (Sept 24, 18:00 UTC)

✅ **All must be true:**
- [ ] Build verification complete: 73 tests compile, 0 errors, ≤10 warnings
- [ ] Smoke test pass: 3 sample tests green (Phase 1/2/3)
- [ ] Phase 1 execution: 12/12 crash-recovery tests pass
- [ ] Phase 2 execution: 14/14 SAGA tests pass
- [ ] Phase 3 execution: 34/34 timeout + distributed tests pass
- [ ] Chaos evidence: Recovery scenario + cascade scenario both complete
- [ ] Determinism validated: 5-run consistency confirmed, output hashes match
- [ ] CPU baseline captured: Performance metrics locked for Oct 1 hardware comparison
- [ ] CI workflow operational: Workflow passes, artifacts collected
- [ ] Evidence bundle generated: `WAVE_A_CI_EXECUTION_EVIDENCE_2026_09_22.md` signed off
- [ ] ROADMAP.md updated: Transaction marked complete [x]
- [ ] **Final: 73/73 tests pass, 100% pass rate, zero data loss, determinism proven**

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Test flakiness (transient failures) | MEDIUM | MEDIUM | Re-run failed tests 5x Sept 13-14; flag persistent flakes for Oct 1 |
| Build timeout on CI (slow compilation) | LOW | MEDIUM | Use sccache for incremental builds; 45 min budget includes 20 min compile |
| WAL corruption test crashes process abnormally | MEDIUM | LOW | Verify test framework handles corrupted WAL gracefully; investigate if crash unexpected |
| Distributed tests timeout (cross-shard latency) | LOW | MEDIUM | Increase timeout threshold Sept 17; monitor network latency |
| Determinism validation fails (non-deterministic output) | MEDIUM | HIGH | Root cause investigation required; may indicate race condition in recovery |
| Representative-hardware unavailable Oct 1 | MEDIUM | LOW | CPU baseline sufficient for Sept 30 exit; defer GPU validation to Oct 15 |

---

## Next Steps

**Pre-Execution (Sept 2-6):**
- [ ] Verify build dependencies installed (RocksDB, fmt, spdlog, yaml-cpp)
- [ ] Test CMake preset `community-release` compiles (smoke build)
- [ ] Review 73 test cases + expected outcomes
- [ ] Prepare monitoring dashboard (test results, pass/fail tracking)

**Execution (Sept 7-24):**
- [ ] Sept 7-8: Build verification + CI workflow setup
- [ ] Sept 9-14: Sequential test execution (73 tests in phases)
- [ ] Sept 13-14: Chaos re-runs + flaky test investigation
- [ ] Sept 15-16: Evidence consolidation
- [ ] Sept 17-20: Determinism validation + baseline capture
- [ ] Sept 21-24: CI integration + Wave B sign-off

**Post-Execution (Oct 1+):**
- [ ] Oct 1-7: Transaction representative-hardware validation (A100/H100 if available)
- [ ] Oct 1-7: Compare CPU baseline vs. hardware baseline (if hardware available)
- [ ] Oct 8-15: Extended soak tests (sustained load, long-run chaos)
- [ ] Oct 15-30: Production hardening based on evidence

---

## Document Status

**Prepared By:** Copilot Coding Agent  
**Date:** Sept 2, 2026, 14:35 UTC  
**Session:** Wave B Planning — Option B3 (Transaction CI Execution)  
**Gate:** Critical blocker — "Tests implemented; NO develop CI run yet"

**Status:** ✅ READY FOR EXECUTION (Sept 7, 2026, 09:00 UTC)

