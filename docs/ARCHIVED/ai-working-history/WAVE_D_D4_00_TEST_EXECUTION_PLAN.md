# Wave D D4-00 — Release Critical Test Execution Plan

**Date:** 2026-08-17  
**Status:** 🟡 STAGED FOR BUILD COMPLETION + TEST EXECUTION  
**Scope:** Execute 155+ `release_critical` labeled tests across Wave A modules and supporting modules  
**Branch:** copilot/implement-real-sourcecode-to-close-gaps (PR #5962)

---

## Executive Summary

Once the Wave D D4-00 build completes successfully, the release_critical test suite must be executed to verify:

1. **Wave A modules** (Transaction, Sharding, Replication) — hardening and Phase 2-6 correctness
2. **Supporting modules** (Process, Failover, Updates) — Phase 1-6 completeness and deterministic behavior
3. **Integration tests** — cross-module resilience and degradation handling

**Expected Test Count:** 155+ tests  
**Expected Execution Time:** 60-90 minutes  
**Success Criteria:** 155+ tests report PASS, 0 failures

---

## 1. Test Suite Organization

### 1.1 Wave A Modules (Priority 1)

#### Sharding (96 tests)
**File:** `tests/sharding/CMakeLists.txt`  
**Labels:** `release_critical;sharding_p6;sharding`

Test Categories:
- `test_sharding_phase6_hardening` — Phase 6 hardening edge cases
- `test_sharding_fault_injection` — Fault injection + recovery
- `test_sharding_retrieval_multi_shard_exact` — Multi-shard exact consistency
- `test_sharding_thread_safety_lock_order` — Thread safety + consensus

**Acceptance:** All 96 tests PASS

#### Transaction (35 tests)
**File:** `tests/transaction/CMakeLists.txt`  
**Labels:** `release_critical;transaction_p2_p3;transaction`

Test Categories:
- `test_transaction_phase2` — Phase 2 coordinator hardening
- `test_transaction_phase3_fault_injection` — Phase 3 fault injection
- `test_transaction_crash_recovery` — Crash recovery semantics

**Acceptance:** All 35 tests PASS

#### Replication (24 tests)
**File:** `tests/replication/CMakeLists.txt`  
**Labels:** `release_critical;replication_p4_p5;replication`

Test Categories:
- `test_replication_geographic_placement` — Geographic placement policy
- `test_replication_failover_integration` — Failover integration
- `test_replication_wal_shipping` — WAL async shipping + durability

**Acceptance:** All 24 tests PASS

### 1.2 Supporting Modules (Priority 2)

#### Failover (8 tests)
**File:** `tests/failover/CMakeLists.txt`  
**Labels:** `release_critical;failover`

Test Categories:
- `test_failover_phase2_focused` — Phase 2 focused tests
- `test_failover_phase3_chaos` — Phase 3 chaos validation

**Acceptance:** All 8 tests PASS

#### Process (72+ tests)
**File:** `tests/process/CMakeLists.txt`  
**Labels:** `release_critical;process`

Test Categories:
- `test_process_phase1_basics` — Phase 1 lifecycle
- `test_process_phase2_scaling` — Phase 2 resource management
- `test_process_phase3_error_handling` — Phase 3 error handling
- `test_process_phase4_monitoring` — Phase 4 observability
- `test_process_phase5_performance` — Phase 5 benchmarks
- `test_process_phase6_sla` — Phase 6 SLA verification

**Acceptance:** All 72+ tests PASS

#### Updates (20+ tests)
**File:** `tests/updates/CMakeLists.txt`  
**Labels:** `release_critical;updates`

Test Categories:
- `test_updates_rollback_hardening` — Rollback semantics
- `test_updates_diagnostics` — Diagnostic emission
- `test_updates_isolation` — Isolation model
- `test_updates_state_machine` — State machine correctness

**Acceptance:** All 20+ tests PASS

### 1.3 Integration Tests (Priority 3)

#### Cross-Module Integration (40+ tests)
**File:** `tests/integration/CMakeLists.txt`  
**Labels:** `release_critical;pipeline_integration;resilience`

Test Categories:
- `test_pipeline_transaction_sharding` — Transaction + Sharding coordination
- `test_pipeline_failover_recovery` — Failover + recovery orchestration
- `test_pipeline_degradation_handling` — Degradation + fallback

**Acceptance:** All 40+ tests PASS

---

## 2. Test Execution Commands

### 2.1 Full Release Critical Suite

**Command:**
```bash
cd /home/runner/work/ThemisDB/ThemisDB
ctest --preset community-release -L release_critical \
  --output-on-failure -j 4 --timeout 300
```

**Expected Output:**
- 155+ test results
- 0 failures
- Execution time < 90 minutes

**Labels Matched:**
- `release_critical` — Primary filter
- `transaction_p2_p3`, `sharding_p6`, `replication_p4_p5` — Wave A
- `failover`, `process`, `updates` — Supporting modules
- `pipeline_integration`, `resilience` — Integration

### 2.2 Module-Specific Test Runs (Optional Isolation)

**Sharding Only:**
```bash
ctest --preset community-release -L "sharding_p6" \
  --output-on-failure -j 4 --timeout 300
```

**Transaction Only:**
```bash
ctest --preset community-release -L "transaction_p2_p3" \
  --output-on-failure -j 4 --timeout 300
```

**Replication Only:**
```bash
ctest --preset community-release -L "replication_p4_p5" \
  --output-on-failure -j 4 --timeout 300
```

**Supporting Modules Only:**
```bash
ctest --preset community-release -L "failover|process|updates" \
  --output-on-failure -j 4 --timeout 300
```

---

## 3. Success Criteria

### 3.1 Test Execution

- ✅ **155+ tests execute** — All registered release_critical tests must run
- ✅ **155+ tests PASS** — 100% pass rate required
- ✅ **0 failures** — Zero test failures
- ✅ **No timeouts** — All tests complete within --timeout 300 seconds
- ✅ **No hangs** — No infinite loops or deadlocks detected
- ✅ **Execution time < 90 minutes** — Full suite within time budget

### 3.2 Sanitizer Findings

- ✅ **TSAN clean** — Zero thread-safety violations
- ✅ **ASan clean** — Zero memory errors
- ✅ **UBSan clean** — Zero undefined behavior
- ✅ **No new warnings** — Same warning set as baseline

### 3.3 Code Quality

- ✅ **Doxygen coverage** — ≥95% public API documentation
- ✅ **No regressions** — Same or improved from Wave 9 baseline
- ✅ **No new CRITICAL findings** — Same gap register status as 2026-08-04

---

## 4. Evidence Collection (Post-Test)

Once tests complete successfully, collect:

### 4.1 Build Artifacts
1. **Build log** — CMake config + compilation output
   - Target: Save `build-community-debug-allow-missing-rocksdb/build.log` or cmake output
   - Verify: 1479 targets compiled, 0 errors

2. **Build statistics**
   - Build time (minutes)
   - Peak memory usage
   - Compiler warnings (count)

### 4.2 Test Results
1. **CTest output** — Full test execution log
   - Target: `ctest --output-on-failure` stdout/stderr
   - Format: Human-readable + machine-parseable results

2. **Test summary**
   - Tests passed: 155+
   - Tests failed: 0
   - Execution time (minutes)
   - Slowest tests (top 5)

### 4.3 Code Quality Metrics
1. **Doxygen coverage**
   - Target: `docs/DOXYGEN_COVERAGE_REPORT.md`
   - Verify: ≥99% header files, >72% overall

2. **Sanitizer summary**
   - TSAN/ASan/UBSan findings: 0 new

### 4.4 Regression Verification
1. **Module gap registers** — No new CRITICAL findings
   - `src/server/MODULE_GAPS.md`
   - `src/llm/MODULE_GAPS.md`
   - `src/sharding/MODULE_GAPS.md`
   - `src/transaction/MODULE_GAPS.md`
   - `src/replication/MODULE_GAPS.md`

2. **Benchmark gates** — No regressions vs Wave 9 baseline
   - Wave 9 SOA-01 (auth p99 ≤ 150 µs)
   - Wave 9 SMC-04 (RTO ≤ 5000 µs)
   - Wave 9 CFR-05 (cluster rejoin ≤ 2000 µs)

---

## 5. Documentation Updates (Post-Test)

Once test execution completes with success:

### 5.1 Update GA Promotion Sign-Off

**File:** `docs/governance/GA_PROMOTION_SIGN_OFF.md`

**Section 2.4 Batch D (Update D-4 or add D-11):**
```markdown
| D-11 | Wave D D4-00 Build Verification + Test Execution | 155+ release_critical tests PASS, build successful | ✅ PASS (2026-08-17) |
```

**Section 3 Promotion Checklist (Mark Complete):**
```markdown
- [x] `develop` HEAD passes the full `release_critical` CTest suite (no failures)
- [x] Build successful (1479 targets compiled, 0 errors)
- [x] 155+ release_critical tests PASS (0 failures)
- [x] Sanitizer findings clean (TSAN/ASan/UBSan)
```

### 5.2 Create Wave D Completion Report

**File:** `ai_working/WAVE_D_D4_00_COMPLETION_REPORT.md`

**Contents:**
- Build statistics (time, targets, errors)
- Test results (count, pass/fail, execution time)
- Evidence summaries (doxygen, sanitizers, benchmarks)
- Risk assessment for GA promotion
- Recommendations for v2.4.1

---

## 6. Estimated Timeline

| Phase | Duration | Status |
|-------|----------|--------|
| Build completion | ~90-150 min | ⏳ IN PROGRESS |
| Test execution | ~60-90 min | ⏳ Pending build |
| Evidence collection | ~15 min | ⏳ Pending tests |
| Documentation update | ~10 min | ⏳ Pending evidence |
| Human review + sign-off | TBD | ⏳ Pending docs |
| **Total** | **~4-5 hours** | **⏳ IN PROGRESS** |

---

## 7. Failure Recovery Plan

### If Build Fails

**Action:** Stop, investigate root cause, fix, retry
- Check build log for specific error
- Verify all dependencies installed
- Check CMake configuration
- Refer to `WAVE_D_D4_00_BUILD_ENVIRONMENT_STATUS.md` for troubleshooting

### If Tests Fail

**Action:** Stop, categorize failure, fix, retry affected tests
- Isolate failing test module (sharding/transaction/replication/etc.)
- Check if regression or new issue
- Review code changes in PR #5962
- Revert and retry if necessary

### If Sanitizers Report Issues

**Action:** Review findings, determine impact, escalate if CRITICAL
- TSAN violations → thread safety issue → fix + retry
- ASan/UBSan findings → memory safety issue → fix + retry
- If unrelated to PR changes → escalate to security team

---

## 8. Success Path to GA Promotion

```
Build Complete ✅
       ↓
Test Execution ✅
       ↓
155+ Tests PASS ✅
       ↓
Collect Evidence ✅
       ↓
Update GA_PROMOTION_SIGN_OFF.md ✅
       ↓
Section 9 Human Approver Sign-Off ⏳
       ↓
Create v2.4.0 GA Tag 🎉
       ↓
Release v2.4.0 to Production 🚀
```

---

**Next Action:** Wait for build completion, then execute this test plan.

**Document Version:** 1.0 (2026-08-17)  
**Branch:** copilot/implement-real-sourcecode-to-close-gaps  
**PR:** #5962
