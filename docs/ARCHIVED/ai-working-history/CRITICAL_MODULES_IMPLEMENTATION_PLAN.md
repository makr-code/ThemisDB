# 🎯 14 CRITICAL MODULES — KONKRETE IMPLEMENTIERUNGS-PLAN

**Datum:** 2026-09-02  
**Ziel:** GA-Freigabe Q4 2026  
**Team:** 4–5 FTE  
**Dauer:** 15–18 Wochen  
**Stand:** Source-validiert gegen ROADMAP.md + src/*/ROADMAP.md

---

## 📋 EXECUTIVE SUMMARY

Systematische Analyse aller **72 Module** in `src/` ergab:
- **14 CRITICAL** Module blocking GA-Zertifizierung
- **6 HIGH** Priority Support-Module  
- **49+ MEDIUM/LOW** Modul needing ongoing maintenance

Das Task erfordert eine Wave A→B→C→D Gating-Struktur mit:
1. **Wellen-Ausführung** (nicht ad-hoc)
2. **Akzeptanzkriterien** pro Gap (messbar, nicht vage)
3. **Test-Coverage** Anforderungen
4. **CI Execution Evidence** Bundles

---

## 🔴 GRUPPE A: IMMEDIATE BLOCKERS (Sept 2-30)

### 1. TRANSACTION — Crash-Recovery & SAGA Determinism

**Current Status (from ROADMAP.md):**
- ✅ Phase 1 tests implemented (33 tests)
- ✅ Phase 2 distributed coordinator tests implemented (9 tests)
- ✅ Phase 3 fault injection tests implemented (14 tests)
- 🔴 **BUILD & EXECUTION VERIFICATION PENDING** — Files present but CI execution not confirmed
- 🔴 Crash-recovery validation: WAL replay under chaos scenarios not yet done
- 🔴 SAGA compensation idempotency under concurrent retries not validated
- 🔴 Timeout semantics: deterministic bounds not yet verified

**Konkrete GAPS mit ACs:**

#### Gap 1.1: Coordinator Crash-Recovery (AC-6)
**Gap:** WAL replay must resolve all in-doubt transactions within 5s of coordinator restart; deterministic rollback under ≥30s sustained contention without data loss.

**Acceptance Criteria:**
1. ✅ **Recovery under SIGKILL**: Coordinator recovers from abrupt shutdown (SIGKILL, OS crash)
   - All pending transactions recovered: 100% COMMIT or 100% ROLLBACK (no orphans)
   - Recovery-Zeit ≤ 2 Sekunden
   - **Verify**: `tests/transaction/test_coordinator_crash_recovery.cpp::test_sigkill_recovery`

2. ✅ **Deterministic Rollback**: Same crash scenario → same commit/rollback sequence
   - Clock-independent ordering (logical clocks used)
   - **Verify**: `tests/transaction/determinism/test_crash_recovery_determinism.cpp`

3. ✅ **No Orphaned Locks**: All locks released post-recovery
   - Lock manager state clean; no deadlocks
   - **Verify**: `tests/transaction/integration/test_recovery_lock_cleanup.cpp`

4. ✅ **Contention Stress**: ≥30s sustained contention, recovery deterministic
   - 100+ concurrent transactions under heavy load → crash → recovery
   - **Verify**: `tests/transaction/chaos/test_crash_under_contention.cpp`

5. ✅ **WAL Determinism**: Write-Ahead Log entries produced in deterministic order
   - Logical sequence numbers (not timestamps) used
   - **Verify**: `tests/transaction/determinism/test_wal_replay_determinism.cpp`

6. ✅ **Idempotent WAL Replay**: Same WAL segment replayed N times → identical state
   - **Verify**: `tests/transaction/recovery/test_wal_replay_idempotent.cpp`

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium | Test File |
|-----------|-------|-------|---|---|
| Unit | 3 | Coordinator::RecoverFromSnapshot, StateStorage::Load | 100% pass | `test_coordinator_recovery_unit.cpp` |
| Integration | 4 | SIGKILL + recovery cycle (5 iterations); concurrent clients | 100% pass, <2s recovery | `test_coordinator_crash_recovery.cpp` |
| Chaos | 3 | Network partition, unavailability, clock skew | No state divergence | `test_coordinator_chaos.cpp` |
| Determinism | 2 | Same input state N times → identical sequence | Variance = 0 | `test_crash_recovery_determinism.cpp` |

**Definition of Done:**
- [ ] 12 Test Cases GREEN (all categories)
- [ ] Crash recovery SOP documented
- [ ] Root `ROADMAP.md` line 22-24 updated with completion date + CI evidence
- [ ] Tag: `critical-transaction-crash-recovery-2026-09-10`

**Effort:** 3 days | **Status:** IMPLEMENTATION START TODAY

---

#### Gap 1.2: SAGA Compensation Idempotency (AC-9/AC-10)
**Gap:** SAGA compensation repeatable; identical result on 5+ retries. Compensation idempotency under concurrent retries.

**Acceptance Criteria:**
1. ✅ **Compensation Idempotency**: Calling compensation step 5× → identical final state
   - No dupe effects (e.g., charge credit card only once even if step called 5x)
   - **Verify**: `tests/transaction/saga/test_saga_compensation_idempotent.cpp`

2. ✅ **Concurrent Compensation Retries**: 10 concurrent retries of same compensation → deterministic state
   - **Verify**: `tests/transaction/saga/test_saga_concurrent_compensation.cpp`

3. ✅ **Compensation Ordering**: Partial remote failure handled; compensation order deterministic
   - **Verify**: `tests/transaction/saga/test_saga_partial_failure.cpp`

4. ✅ **Circuit Breaker Activation**: After 5 consecutive remote failures → circuit breaker trips
   - Prevents cascading failures
   - **Verify**: `tests/transaction/saga/test_saga_circuit_breaker.cpp`

5. ✅ **Retry Storm Suppression**: Exponential backoff with bounds
   - Base 100ms, factor 2×, jitter ±20%, max 3 retries
   - **Verify**: `tests/transaction/saga/test_saga_backoff_bounds.cpp`

6. ✅ **Deterministic Recovery**: Same partial-failure scenario → same compensation sequence
   - **Verify**: `tests/transaction/determinism/test_saga_determinism.cpp`

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium |
|-----------|-------|-------|---|
| Unit | 4 | Compensation idempotency, circuit breaker state | 100% pass |
| Integration | 6 | Multi-step SAGA, partial failures, concurrent retries | All paths deterministic |
| Chaos | 3 | Failure injection, retry storms, timeout cascades | Graceful degradation |
| Determinism | 1 | Identical scenario N times | Variance = 0 |

**Definition of Done:**
- [ ] 14 Test Cases GREEN
- [ ] SAGA compensation SOP documented
- [ ] Root `ROADMAP.md` line 23 updated
- [ ] Tag: `critical-transaction-saga-2026-09-10`

**Effort:** 3 days | **Status:** IMPLEMENTATION START TODAY

---

#### Gap 1.3: No Cascade Aborts (AC-7)
**Gap:** Abort of one transaction does not abort independent transactions

**Acceptance Criteria:**
1. ✅ **Isolation**: TX1-Abort ≠ TX2...TX100 Abort (concurrent, independent)
   - **Verify**: `tests/transaction/isolation/test_no_cascade_aborts.cpp`

2. ✅ **Lock Cleanup**: TX1 abort releases only its locks, not others
   - **Verify**: `tests/transaction/integration/test_abort_lock_isolation.cpp`

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium |
|-----------|-------|-------|---|
| Unit | 2 | Lock cleanup, isolation checks | 100% pass |
| Integration | 3 | 100 concurrent abort 1, rest continue | No cascade |
| Chaos | 1 | Abort under load | No collateral damage |

**Definition of Done:**
- [ ] 6 Test Cases GREEN
- [ ] Root `ROADMAP.md` line 22 updated
- [ ] Tag: `critical-transaction-cascade-2026-09-10`

**Effort:** 2 days

---

### Summary: TRANSACTION Module (8 days total)

| Gap | Effort | Target | Tests |
|-----|--------|--------|-------|
| Crash-Recovery (AC-6) | 3 days | Sept 5 | 12 tests |
| SAGA Idempotency (AC-9/10) | 3 days | Sept 7 | 14 tests |
| No Cascade (AC-7) | 2 days | Sept 8 | 6 tests |
| **TOTAL** | **8 days** | **Sept 10** | **32 tests** |

**Checkpoint:** Sept 10, 2026 — All 3 gaps GREEN, evidence bundle committed

---

### 2. GPU — CUDA Call Reduction (50% = 340 → 170)

**Current Status (from ROADMAP.md, line 34-35):**
- ✅ CUDA-call audit complete for `src/gpu/` + `include/gpu/`
- ✅ `include/gpu/cuda_raii.h` added with RAII wrappers (`CudaStreamGuard`, `CudaEventGuard`, `CudaDeviceMemoryGuard`)
- ✅ All 340 raw call sites catalogued in `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`
- ✅ Wrapper coverage confirmed for stream/event/memory
- 🟡 **WRAPPER ADOPTION IN PROGRESS** — identified sites not yet wrapped
- 🟡 **DETERMINISM VALIDATION PENDING** — GPU output parity H100 vs A100 not yet measured

**Konkrete GAPS:**

#### Gap 2.1: Wrapper Adoption (AC-1/AC-2)
**Gap:** 340 unchecked CUDA calls → ≤170 wrapped via RAII or error_check()

**Acceptance Criteria:**
1. ✅ **Audit Count ≤ 170**: Verified count via automated tool
   - **Verify**: `tools/ci/gpu_cuda_call_audit.py --output audit_report_2026-09-15.json`
   - Target: Count ≤ 170 unchecked calls

2. ✅ **RAII Wrapper Adoption**: All new calls wrapped via `CudaStreamGuard<T>` or `CudaResourceGuard<T>`
   - No raw CUDA calls without error checking or guard
   - **Verify**: Code review + grep verification

3. ✅ **Audit Metadata**: Each remaining unchecked call has inline comment explaining reason
   - Example: `// UNCHECKED: cudaDeviceGetAttribute is init-only, safe to ignore`
   - **Verify**: Audit scan pattern matching on all ≤170 remaining calls

4. ✅ **Wrapper Overhead ≤ 5%**: CudaResourceGuard RAII latency overhead
   - **Verify**: `tests/gpu/bench/wrapper_overhead.cpp`

5. ✅ **Adoption Completeness**: 100% of high-risk sites (allocation, stream, event) wrapped
   - **Verify**: Grep pattern + manual audit

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium | Test File |
|-----------|-------|-------|---|---|
| Unit | 5 | CudaResourceGuard, error_check() function | 100% pass | `test_cuda_guard_unit.cpp` |
| Integration | 4 | Wrapper adoption in KernelExecutor, TensorAllocator, MemoryPool | No leaks; audit verified | `test_cuda_wrapper_integration.cpp` |
| Performance | 1 | Wrapper overhead measurement | ≤5% overhead | `bench_wrapper_overhead.cpp` |
| Audit | 1 | Final call count audit | ≤170 calls | `test_cuda_call_audit_final.cpp` |

**Definition of Done:**
- [ ] 11 Test Cases GREEN
- [ ] `tools/ci/gpu_cuda_call_audit.py` reports count ≤ 170
- [ ] All 170 remaining calls have audit comments
- [ ] Root `ROADMAP.md` line 34 updated with completion date
- [ ] Tag: `critical-gpu-wrapper-adoption-2026-09-15`

**Effort:** 8 days | **Status:** IMPLEMENTATION START TODAY (parallel with TRANSACTION)

---

#### Gap 2.2: GPU Determinism Validation (AC-4)
**Gap:** H100 vs A100 output variance ≤ 1% FP

**Acceptance Criteria:**
1. ✅ **H100 Baseline**: Run kernel suite on H100; record output tensors
   - **Verify**: `tests/gpu/determinism/test_gpu_output_h100_baseline.cpp`

2. ✅ **A100 Parity**: Same kernels on A100; compare output ≤ 1% FP variance
   - **Verify**: `tests/gpu/determinism/test_gpu_output_a100_parity.cpp`

3. ✅ **Deterministic Seed**: All randomness seeded; outputs reproducible N times
   - **Verify**: `tests/gpu/determinism/test_gpu_determinism_repeated.cpp`

4. ✅ **Cross-Device Consistency**: Multi-device scenarios (2 GPUs) produce identical results
   - **Verify**: `tests/gpu/determinism/test_gpu_multidevice_consistency.cpp`

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium |
|-----------|-------|-------|---|
| Determinism | 4 | H100/A100 baseline + parity + repeatability | Variance ≤ 1% FP |
| Performance | 1 | Determinism SLA: <100µs overhead | <100µs overhead |

**Definition of Done:**
- [ ] 5 Test Cases GREEN on representative hardware (H100 + A100)
- [ ] Determinism evidence bundle committed
- [ ] Root `ROADMAP.md` line 34 updated
- [ ] Tag: `critical-gpu-determinism-2026-09-20`

**Effort:** 10 days (includes hardware access delay)

---

### Summary: GPU Module (18 days total)

| Gap | Effort | Target | Tests |
|-----|--------|--------|-------|
| Wrapper Adoption (AC-1/2/3/5) | 8 days | Sept 15 | 11 tests |
| Determinism (AC-4) | 10 days | Sept 20 | 5 tests |
| **TOTAL** | **18 days** | **Sept 20** | **16 tests** |

**Checkpoint:** Sept 20, 2026 — All GPU gaps GREEN, CI evidence committed

---

### 3. QUERY — FTS Executor Backend (Design 5d + Implementation 30d)

**Current Status (from ROADMAP.md, line 74-79):**
- ✅ FTS parser tokens added to lexer (2026-08-09)
- ✅ FTS structures added to aql_parser.h (2026-08-09)
- ✅ parseSearchClause() implemented (2026-08-09)
- 🔴 **EXECUTOR BACKEND MISSING** — FTS index lookup, scoring not wired
- 🔴 **PERFORMANCE GATE UNDEFINED** — ≤100ms on 100K documents not yet measured

**Konkrete GAPS:**

#### Phase 1: DESIGN (5 days, Sept 2-10)

**Gap 3.1: FTS Executor Design Spec (AC-1/AC-2)**

**Acceptance Criteria:**
1. ✅ **Formal Design Spec**: Approved by technical committee
   - Algorithms documented (tokenization, IR build, query planning, scoring)
   - Data structures defined (inverted index format, query AST, score model)
   - **Verify**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md`

2. ✅ **100% AQL v2.0 Statement Coverage**: All FTS statement types in design
   - Simple text search, phrases, boolean, proximity, regex
   - **Verify**: Design document maps each to algorithm

3. ✅ **Design Approval**: Technical committee sign-off
   - **Verify**: `ROADMAP.md` entry with approval date + approver signature

4. ✅ **Performance Model**: Algorithmic complexity analysis + hardware assumptions
   - **Verify**: Design includes big-O analysis + target hardware specs

5. ✅ **Integration Points**: Parser-to-Executor handoff documented
   - **Verify**: Data flow diagrams in design spec

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium |
|-----------|-------|-------|---|
| Design Review | 1 | Technical committee approval | Approved + signatures |
| Documentation | 4 | Spec coverage, algorithms, integration | 100% AQL v2.0 coverage |

**Definition of Done:**
- [ ] Design spec created: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md`
- [ ] Technical committee approval document
- [ ] Root `ROADMAP.md` line 74-79 updated with design completion date
- [ ] Tag: `critical-query-fts-design-2026-09-10`

**Effort:** 5 days | **Target:** Sept 10 | **Status:** START TODAY

---

#### Phase 2: IMPLEMENTATION (30 days, Sept 15-Oct 30)

**Gap 3.2: FTS Executor Backend Implementation (AC-3/AC-4/AC-5/AC-6)**

**Acceptance Criteria:**
1. ✅ **Parser Output → Executor AST**: All AQL FTS statements correctly map to internal representation
   - SearchClauseNode → ExecutorAST translation 100% coverage
   - **Verify**: `tests/query/fts/test_parser_executor_integration.cpp` (8+ statement types)

2. ✅ **Performance Gate: p95 ≤ 100ms**: Latency on 100K-document corpus
   - Measured on representative hardware (CPU: Xeon E5-2690 or equivalent, SSD)
   - **Verify**: `tests/query/bench/fts_performance_scaling.cpp::test_100k_corpus_p95`

3. ✅ **Deterministic Results**: Same query on N runs → identical result sets
   - Variance = 0 (deterministic sorting, no random sampling)
   - **Verify**: `tests/query/determinism/test_fts_determinism.cpp`

4. ✅ **Error Handling**: All error paths tested (invalid tokens, OOM, timeout)
   - Error codes ∈ [6100-6199]
   - **Verify**: `tests/query/errors/test_fts_error_paths.cpp`

5. ✅ **Relevance Ranking**: Score model validated on reference corpus
   - Recall@k ≥ 0.75 for k ∈ {1, 5, 10}
   - **Verify**: `tests/query/eval/test_fts_relevance.cpp`

6. ✅ **Index Construction**: Inverted index built correctly from 100K documents
   - **Verify**: `tests/query/index/test_fts_index_construction.cpp`

**Test-Strategie:**

| Kategorie | Count | Scope | Pass-Kriterium | Test File |
|-----------|-------|-------|---|---|
| Unit | 5 | Parser tokenization, AST building, score model | 100% pass | `test_fts_executor_unit.cpp` |
| Integration | 8 | End-to-end: AQL → Executor → Results | All statements execute correctly | `test_fts_end_to_end.cpp` |
| Performance | 2 | Latency: p50/p95/p99 on 100K; throughput QPS | p95 ≤100ms; QPS ≥500 | `bench_fts_performance.cpp` |
| Determinism | 1 | Same query N times | Variance = 0 | `test_fts_determinism.cpp` |
| Evaluation | 2 | Relevance ranking, recall@k | Recall ≥ 0.75 | `test_fts_relevance.cpp` |
| Errors | 2 | Invalid tokens, OOM, timeout | Graceful error handling | `test_fts_errors.cpp` |

**Definition of Done:**
- [ ] 20 Test Cases GREEN (all categories)
- [ ] Parser integration verified + tested
- [ ] Performance gate p95 ≤100ms validated on representative hardware
- [ ] Relevance ranking @k gates met
- [ ] `src/query/WAVE_A_CLOSURE_EVIDENCE_BUNDLE_2026-10-30.md` committed
- [ ] Root `ROADMAP.md` line 74-79 updated with completion date + evidence link
- [ ] Tag: `critical-query-fts-impl-2026-10-30`

**Effort:** 30 days (Design started Sept 2, implementation Sept 15-Oct 30)

---

### Summary: QUERY Module (35 days total: 5 design + 30 implementation)

| Phase | Gap | Effort | Target | Tests |
|-------|-----|--------|--------|-------|
| **Design** | FTS Design Spec (AC-1/2) | 5 days | Sept 10 | 5 reviews |
| **Impl** | FTS Backend (AC-3-6) | 30 days | Oct 30 | 20 tests |
| **TOTAL** | | **35 days** | **Oct 30** | **25 tests** |

**Checkpoint Phase 1:** Sept 10, 2026 — Design spec approved  
**Checkpoint Phase 2:** Oct 30, 2026 — Implementation complete + evidence committed

---

## 🟡 GRUPPE B: Q3 WAVE (Sept 7-30)

[Continued in next section — SERVER + STORAGE modules]

---

## 📊 IMMEDIATE NEXT STEPS (This Week)

### TODAY (Sept 2-3):
1. [ ] Commit implementation plan to repository
2. [ ] Start TRANSACTION Gap 1.1 (Crash-Recovery): Create `test_coordinator_crash_recovery.cpp` stub
3. [ ] Start GPU Gap 2.1 (Wrapper Adoption): Create `test_cuda_wrapper_integration.cpp` stub
4. [ ] Start QUERY Gap 3.1 (Design): Create `DESIGN_FTS_EXECUTOR_2026-09-10.md` skeleton

### Tomorrow (Sept 4-5):
5. [ ] TRANSACTION crash-recovery tests: Write 12 test cases
6. [ ] GPU wrapper audit: Run `tools/ci/gpu_cuda_call_audit.py`; identify high-risk sites
7. [ ] QUERY design: Technical committee review meeting scheduled

### This Week (Sept 6-7):
8. [ ] TRANSACTION CI execution: Build + run all 32 tests on develop
9. [ ] GPU wrapper adoption: Start wrapping identified sites (20% completion)
10. [ ] QUERY design: Finalize spec + approval signatures

---

**Status:** Ready to implement. Team assignment + CI setup required.

