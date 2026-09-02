# WAVE A Q3 MODULES — Detailed Staging & Acceptance Criteria Mapping
## Execution Planning Document (Sept 3-30, 2026)

**Program Alignment**: Wave A→B→C→D gate model on develop  
**Dependencies**: Blocked on TRANSACTION AC-6/9/10/5 proof (Sept 10)  
**Scope**: SERVER, STORAGE, INDEX, LLM (6 modules × ~12-15 ACs each = ~72 total ACs)  
**Timeline**: Sept 15 start (after TRANSACTION proof Sept 10) → Oct 15 completion  
**Evidence**: Per-module WAVE_A_CLOSURE_EVIDENCE_BUNDLE for each module (Sept 20-30)

---

## Module 1: SERVER
**Tier**: CRITICAL (runtime API gateway)  
**Wave A Baseline**: Phase 5 hardening COMPLETE (Sept 1); HTTP timeout + graceful shutdown validation needed

### Acceptance Criteria Mapping

| AC ID | Title | Status | Test File | Target Tests | Acceptance | Evidence |
|-------|-------|--------|-----------|--------------|-----------|----------|
| AC-S1 | HTTP Timeout Enforcement | [~] In Prog | `test_server_http_timeout_determinism.cpp` | 8 | Timeout ≤ configured duration + cascade propagation | `SERVER_AC_S1_EVIDENCE.md` |
| AC-S2 | Graceful Shutdown Drain | [~] In Prog | `test_server_graceful_shutdown.cpp` | 6 | Drain phase ≤5s, in-flight requests completed, no new accepts | `SERVER_AC_S2_EVIDENCE.md` |
| AC-S3 | Wire Protocol Retry | [x] Done | Phase 5 existing tests | 16 | Exponential backoff, max 3 retries, error consistency | `SERVER_AC_S3_EVIDENCE.md` |
| AC-S4 | Idempotency Cache Snapshot | [~] In Prog | `test_server_idempotency_safety.cpp` | 5 | Thread-local snapshots, zero-window fail-safe | `SERVER_AC_S4_EVIDENCE.md` |
| AC-S5 | ****** JWT Validation | [x] Done | Phase 5 existing tests | 12 | JTI blacklist, expiry, issuer, audience | `SERVER_AC_S5_EVIDENCE.md` |
| AC-S6 | Auth Gate Contract Frozen | [x] Done | SCH-01..SCH-20 | 20 | Fail-closed semantics, all auth paths hardened | `SERVER_AC_S6_EVIDENCE.md` |
| AC-S7 | Voice API Authorization | [~] In Prog | `test_server_voice_api_auth.cpp` | 4 | Token validation, role-based access, audit logging | `SERVER_AC_S7_EVIDENCE.md` |
| AC-S8 | Fail-Closed Reject Semantics | [~] In Prog | `test_server_fail_closed_validation.cpp` | 7 | No fallback on auth/validation error, explicit error codes | `SERVER_AC_S8_EVIDENCE.md` |

**Total Tests Required**: ~78 (16 AC-S3 existing + 62 new)  
**Test Categories**:
- HTTP timeout determinism: 8 tests (clock drift ±100ms, SLA accuracy ±50ms)
- Graceful shutdown: 6 tests (drain timing, in-flight completion, no new accepts)
- Idempotency: 5 tests (snapshot concurrency, zero-window bounds)
- Voice/JWT: 4 tests (all validation paths, expiry/issuer/aud/jti)
- Fail-closed: 7 tests (auth failure, validation error, explicit codes)

**Build Blockers** (from memory):
- libcurl headers (pki_client.cpp) — needs libcurl4-openssl-dev
- RocksDB (configure) — needs librocksdb-dev or vcpkg

**Risk**: AC-S4 (Idempotency) requires thread-safety validation under high concurrency (8+ threads)

---

## Module 2: STORAGE
**Tier**: CRITICAL (durable persistence)  
**Wave A Baseline**: AccessCoordinator integration COMPLETE (Sept 1); backup/restore fail-close validation needed

### Acceptance Criteria Mapping

| AC ID | Title | Status | Test File | Target Tests | Acceptance | Evidence |
|-------|-------|--------|-----------|--------------|-----------|----------|
| AC-ST1 | Backup Compression Fail-Close | [x] Done | Phase 4 existing tests | 4 | No silent bypass; reject when zstd/lz4 absent | `STORAGE_AC_ST1_EVIDENCE.md` |
| AC-ST2 | Backup Encryption Fail-Close | [x] Done | Phase 4 existing tests | 4 | No silent bypass; reject when OpenSSL absent | `STORAGE_AC_ST2_EVIDENCE.md` |
| AC-ST3 | GGML Context Forwarding | [~] In Prog | `test_storage_ggml_context_binding.cpp` | 6 | Real ggml allocation, type registration once-only | `STORAGE_AC_ST3_EVIDENCE.md` |
| AC-ST4 | SecuritySignatureManager Fail-Close | [x] Done | Phase 4 existing tests | 3 | No implicit memory fallback; explicit opt-in test-only | `STORAGE_AC_ST4_EVIDENCE.md` |
| AC-ST5 | Remote S3/GCS/Azure Manifest | [~] In Prog | `test_storage_remote_backup_manifest.cpp` | 8 | Manifest + blob upload, integrity validation | `STORAGE_AC_ST5_EVIDENCE.md` |
| AC-ST6 | TieredStorageManager Promotion | [x] Done | Phase 4 existing tests | 5 | PromotionListener callbacks, hot-pattern detection | `STORAGE_AC_ST6_EVIDENCE.md` |
| AC-ST7 | WAL Replay Determinism | [~] In Prog | `test_storage_wal_replay_determinism.cpp` | 7 | 50x replay identical state, no silent truncation | `STORAGE_AC_ST7_EVIDENCE.md` |
| AC-ST8 | MVCC Snapshot Isolation | [~] In Prog | `test_storage_mvcc_snapshot_isolation.cpp` | 8 | Concurrent read/write, version consistency | `STORAGE_AC_ST8_EVIDENCE.md` |

**Total Tests Required**: ~45 (11 existing + 34 new)  
**Test Categories**:
- Backup fail-close: 8 tests (compression, encryption, no bypass)
- GGML binding: 6 tests (context forwarding, type registration, allocation)
- Remote backup: 8 tests (manifest + blobs, integrity check, retry)
- WAL/MVCC: 15 tests (replay determinism 50x, snapshot isolation, version consistency)

**Build Blockers**:
- RocksDB headers + libs (librocksdb-dev)
- GGML headers (if available, else stub)
- OpenSSL headers (libssl-dev)

**Risk**: AC-ST7 (WAL Replay) assumes crash-safe write semantics; may require chaos testing

---

## Module 3: INDEX
**Tier**: CRITICAL (query execution)  
**Wave A Baseline**: FTS executor design approval pending (Sept 3-10); traditional query determinism validation needed

### Acceptance Criteria Mapping

| AC ID | Title | Status | Test File | Target Tests | Acceptance | Evidence |
|-------|-------|--------|-----------|--------------|-----------|----------|
| AC-I1 | FTS Parser Complete | [x] Done | Phase 4 existing tests | 12 | Query parser AST correctness, error recovery | `INDEX_AC_I1_EVIDENCE.md` |
| AC-I2 | FTS Executor Design Approval | [~] In Prog | Design review gate (Sept 3-10) | N/A | Architecture review, interface contracts frozen | `INDEX_AC_I2_EVIDENCE.md` |
| AC-I3 | Traditional Query Determinism | [~] In Prog | `test_index_query_determinism.cpp` | 10 | 50x replay identical results, no sort instability | `INDEX_AC_I3_EVIDENCE.md` |
| AC-I4 | B-Tree Concurrency | [~] In Prog | `test_index_btree_concurrency.cpp` | 8 | No deadlock, no data corruption under contention | `INDEX_AC_I4_EVIDENCE.md` |
| AC-I5 | Index Rebuild Consistency | [~] In Prog | `test_index_rebuild_consistency.cpp` | 6 | Rebuild preserves query semantics, no data loss | `INDEX_AC_I5_EVIDENCE.md` |
| AC-I6 | Range Query Correctness | [~] In Prog | `test_index_range_query_correctness.cpp` | 7 | Boundary conditions, inclusive/exclusive, empty ranges | `INDEX_AC_I6_EVIDENCE.md` |
| AC-I7 | Bloom Filter Accuracy | [~] In Prog | `test_index_bloom_accuracy.cpp` | 5 | False positive rate, no false negatives, size bounds | `INDEX_AC_I7_EVIDENCE.md` |
| AC-I8 | Composite Index Ordering | [~] In Prog | `test_index_composite_index_ordering.cpp` | 6 | Multi-column sort order preservation, ties handled | `INDEX_AC_I8_EVIDENCE.md` |

**Total Tests Required**: ~54 (12 existing + 42 new)  
**Test Categories**:
- Query determinism: 10 tests (50x replay, sort stability, floating-point precision)
- B-Tree concurrency: 8 tests (insert/delete contention, no deadlock, no corruption)
- Rebuild: 6 tests (rebuild preserves semantics, no data loss, consistency)
- Range/Bloom/Composite: 18 tests (boundaries, false positive rate, multi-column order)

**Build Blockers**:
- RocksDB headers
- Boost headers (test framework)

**Risk**: AC-I2 (FTS design approval) is critical path blocker; delays cascade to STORAGE/LLM

**Dependency Gate**: AC-I2 approval must be received by Sept 10 to unblock Sept 15 start

---

## Module 4: LLM
**Tier**: CRITICAL (model serving)  
**Wave A Baseline**: GPU wrapper adoption in progress (parallel Sept 7-15); model integrity + LLM judge hardening needed

### Acceptance Criteria Mapping

| AC ID | Title | Status | Test File | Target Tests | Acceptance | Evidence |
|-------|-------|--------|-----------|--------------|-----------|----------|
| AC-L1 | Model Integrity Gate | [x] Done | Phase 5 existing tests | 6 | SHA-256 gate before load, fail-closed on mismatch | `LLM_AC_L1_EVIDENCE.md` |
| AC-L2 | LLMJudge Unavailable Handling | [x] Done | Phase 5 existing tests | 5 | No mock fallback, explicit llm_unavailable, isMockMode()=false | `LLM_AC_L2_EVIDENCE.md` |
| AC-L3 | CUDA Wrapper Adoption | [~] In Prog | `test_llm_cuda_wrapper_adoption.cpp` | 12 | KernelExecutor, TensorAllocator, MemoryPool wrappers | `LLM_AC_L3_EVIDENCE.md` |
| AC-L4 | GPU Determinism (A100/H100) | [~] In Prog | `test_llm_gpu_determinism_a100.cpp` | 8 | 50x replay identical outputs, FP-tolerance ≤1e-5 | `LLM_AC_L4_EVIDENCE.md` |
| AC-L5 | Model Quantization Accuracy | [~] In Prog | `test_llm_quantization_accuracy.cpp` | 7 | Int8/Int4 accuracy loss <2%, streaming compatible | `LLM_AC_L5_EVIDENCE.md` |
| AC-L6 | Token Generation Determinism | [~] In Prog | `test_llm_token_generation_determinism.cpp` | 6 | Seeded generation, beam-search reproducibility | `LLM_AC_L6_EVIDENCE.md` |
| AC-L7 | Streaming Output Consistency | [~] In Prog | `test_llm_streaming_consistency.cpp` | 5 | Streaming vs batch output parity, no token skip | `LLM_AC_L7_EVIDENCE.md` |
| AC-L8 | Model Loading Timeout | [~] In Prog | `test_llm_model_loading_timeout.cpp` | 4 | Load completes within 30s or explicit timeout, no hang | `LLM_AC_L8_EVIDENCE.md` |

**Total Tests Required**: ~53 (11 existing + 42 new)  
**Test Categories**:
- GPU/CUDA: 12 tests (wrapper adoption, kernel binding, memory management)
- Determinism: 14 tests (GPU determinism 50x replay, token generation seed, beam search)
- Quantization: 7 tests (accuracy loss <2%, streaming compat)
- Streaming: 5 tests (streaming/batch parity, no token skip)
- Model ops: 4 tests (loading timeout, no hang)

**Build Blockers**:
- CUDA toolkit headers + libs (optional, CPU fallback available)
- libcurl headers (for remote model download)

**Risk**: AC-L4 (GPU determinism) requires A100/H100 hardware; CPU mock fallback is mitigation

**Dependency**: Parallel with GPU CUDA audit (Sept 7-15); wrapper adoption planning ready

---

## Integration Dependency Matrix

```
TRANSACTION (AC-6/9/10/5 proof)
    ↓
    ├─→ SHARDING (blocked until Sept 10)
    │       ↓
    │       └─→ DISTRIB_KNOWLEDGE (blocked until Sept 20)
    │
    ├─→ SERVER (AC-S1/2/4/7/8 soft blockers; can start Sept 15 with mocks)
    │
    ├─→ STORAGE (AC-ST3/5/7/8 depend on QUERY FTS design; can start Sept 15 with stubs)
    │
    ├─→ INDEX (AC-I2 FTS design approval is critical path; blocks I3-I8; starts Sept 15 if I2 approved by Sept 10)
    │
    └─→ LLM (AC-L3/4 depend on GPU CUDA audit baseline; parallel Sept 7-15, starts implementation Sept 15)
```

**Hard Blockers** (cannot start without prerequisite):
- ⏳ INDEX AC-I2 (FTS design approval) — if delayed past Sept 6, must escalate
- ⏳ TRANSACTION AC-6 proof (crash-recovery) — Sept 10 target

**Soft Blockers** (can start with mocks/stubs):
- SERVER AC-S1/2/4 (can mock STORAGE layer until Sept 15)
- STORAGE AC-ST5/7/8 (can mock INDEX until FTS executor delivered)
- LLM AC-L4 (can use CPU mock if GPU unavailable)

---

## Wave A Q3 Execution Schedule (Sept 15 - Oct 15)

### Week 1 (Sept 15-21): Foundation + Evidence Assembly
- [ ] Distributed: Each module owner creates evidence bundle template stub
- [ ] SERVER: AC-S1/2 implementation + test execution
- [ ] STORAGE: AC-ST3/5 implementation + test execution
- [ ] INDEX: AC-I3 implementation (if AC-I2 approved)
- [ ] LLM: AC-L3 GPU wrapper adoption + AC-L4 determinism baseline
- **Deliverable**: 4 module evidence stubs with test counts + pass/fail

### Week 2 (Sept 22-28): Chaos + Hardening
- [ ] SERVER: AC-S4/7/8 + chaos timeout scenarios
- [ ] STORAGE: AC-ST7 WAL determinism (50x replay) + AC-ST8 MVCC
- [ ] INDEX: AC-I4/5/6/7/8 remaining tests + FTS design review integration
- [ ] LLM: AC-L5/6/7/8 model ops + streaming consistency
- **Deliverable**: Evidence bundles updated with chaos results + determinism proofs

### Week 3-4 (Sept 29-Oct 15): Sign-Off + GA Closure
- [ ] Distributed: Final evidence bundle review + risk assessment
- [ ] ROOT ROADMAP: Mark all Q3 ACs complete (7/7 modules WAVE_A_READY)
- [ ] GA PROMOTION: Submit Q3 evidence to GA_PROMOTION_SIGN_OFF.md §10
- [ ] RELEASE GATE: Q3 modules freeze for Release Candidate (Oct 20)
- **Deliverable**: 4 signed-off evidence bundles + root ROADMAP update

---

## Test Execution Automation

### Command: Distributed Test Suite Execution
```bash
# Run all Q3 WAVE module tests with parallel test runner
ctest --build-dir /tmp/themis-build \
  --tests-regex "(SERVER|STORAGE|INDEX|LLM)_(AC_|Test_)" \
  --parallel 8 \
  --output-on-failure \
  --label-regex "wave_a" \
  -V 2>&1 | tee /tmp/wave_a_q3_test_execution_$(date +%Y-%m-%d).log

# Expected output:
# - Total tests: ~230 (78 SERVER + 45 STORAGE + 54 INDEX + 53 LLM)
# - Pass rate: ≥95% required for Wave A exit
# - Duration: ~15-20 min (parallel execution)
```

### Build Verification Checklist
```bash
# 1. Configure all modules with WAVE_A flag
cmake --preset community-release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_WAVE_A=ON \
  -DCMAKE_BUILD_TYPE=Release \
  2>&1 | tee /tmp/wave_a_configure.log

# 2. Build test targets (parallel)
cmake --build /tmp/themis-build \
  --target test_server_http_timeout_determinism \
  --target test_storage_ggml_context_binding \
  --target test_index_query_determinism \
  --target test_llm_cuda_wrapper_adoption \
  --parallel 4 \
  2>&1 | tee /tmp/wave_a_build.log

# 3. Execute test suite with labels
ctest --build-dir /tmp/themis-build \
  --label-regex "wave_a_q3" \
  --output-on-failure \
  -V 2>&1 | tee /tmp/wave_a_test_results.log
```

---

## Risk Assessment

### Technical Risks
1. **AC-I2 (FTS Design Approval)** — Critical path blocker
   - Mitigation: Escalate to steering committee Sept 6 if feedback delayed
   - Fallback: INDEX starts without FTS executor (traditional query only Sept 15-22)

2. **AC-L4 (GPU Determinism)** — Requires A100/H100 hardware
   - Mitigation: CPU mock fallback documented
   - Fallback: Skip A100-specific tests; run CPU baseline (pass/fail)

3. **Build Dependency Hell** — Multiple optional dependencies
   - Mitigation: Diagnostic presets (community-release-allow-missing-*) for missing libs
   - Fallback: Run subset of tests without optional modules

### Schedule Risks
1. **TRANSACTION AC-6 Delay** — If Sept 10 proof missed, cascades to SHARDING start
   - Mitigation: Parallel GPU audit + QUERY review (Sept 7-10) unaffected
   - Buffer: 3-day slip tolerance (SHARDING starts Sept 13 at latest)

2. **Test Execution Flakiness** — Timing-dependent tests in CI
   - Mitigation: 50x replay determinism tests require stable clock (≤±100ms jitter)
   - Fallback: Increase test timeout; document environment assumptions

3. **Evidence Bundle Review Lag** — Human sign-off bottleneck
   - Mitigation: Automate evidence collection (JSON + PDF export)
   - Fallback: Parallel review (3 bundles in parallel by Sept 28)

---

## Files to Create (Immediate)

1. `tests/server/test_server_http_timeout_determinism.cpp` (8 tests)
2. `tests/server/test_server_graceful_shutdown.cpp` (6 tests)
3. `tests/server/test_server_idempotency_safety.cpp` (5 tests)
4. `tests/storage/test_storage_ggml_context_binding.cpp` (6 tests)
5. `tests/storage/test_storage_remote_backup_manifest.cpp` (8 tests)
6. `tests/storage/test_storage_wal_replay_determinism.cpp` (7 tests)
7. `tests/storage/test_storage_mvcc_snapshot_isolation.cpp` (8 tests)
8. `tests/index/test_index_query_determinism.cpp` (10 tests)
9. `tests/index/test_index_btree_concurrency.cpp` (8 tests)
10. `tests/llm/test_llm_cuda_wrapper_adoption.cpp` (12 tests)
11. `tests/llm/test_llm_gpu_determinism_a100.cpp` (8 tests)
12. Plus: Evidence bundle templates (8 files × 4 modules = 32 bundle stubs)

**Total Implementation**: ~230 tests + 32 evidence templates = **262 production-quality artifacts**

---

## Sign-Off Criteria for Wave A Q3 Exit

✅ **All 4 modules MUST pass EVERY criterion**:

| Criterion | Validation | Owner | Target |
|-----------|-----------|-------|--------|
| AC Coverage | All 8 ACs per module implemented | Module Owner | Sept 30 |
| Test Pass Rate | ≥95% of tests pass in CI | CI Owner | Sept 30 |
| Determinism Proof | 50x replay consistency for timing-dependent ACs | Module Owner | Sept 28 |
| Chaos Evidence | Fault injection + recovery documented | DevOps/Owner | Sept 28 |
| Security Review | No high-severity findings in GAP scan | Security | Sept 28 |
| Performance Baseline | Benchmarks within SLA bounds | Perf Owner | Sept 28 |
| Documentation | API docs + runbooks updated | Tech Writer | Sept 30 |
| Sign-Off | Human approvals + merge to develop | Release Manager | Oct 1 |

---

## Next Steps (Sept 3-4)

1. **TRANSACTION Build Verification** (parallel track, Sept 4-5)
   - Run all 44 TRANSACTION tests (AC-6/9/10/5)
   - Capture build logs + test results
   - If green → unblock SHARDING team (ready Sept 20)

2. **INDEX FTS Design Review Committee** (Sept 3-5)
   - Review FTS executor design spec
   - Provide feedback by Sept 6
   - Approval gate: Sept 10 (critical path blocker)

3. **Module Owner Distribution** (Sept 3-4)
   - Create WAVE_A_OWNER_ASSIGNMENTS.md (who owns SERVER/STORAGE/INDEX/LLM)
   - Send per-module quick-start guides
   - Kick-off team sync (Mondays 09:00 UTC starting Sept 8)

4. **Evidence Bundle Template Staging** (Sept 3-4)
   - Create 4 template stubs (one per module)
   - Define JSON + PDF export format
   - Distribute to module owners

---

## Dependencies & Blockers Summary

**TRANSACTION AC-6 Proof** (Sept 10)
  ↓
**INDEX AC-I2 Design Approval** (Sept 10)
  ↓
**Q3 Modules Implementation** (Sept 15 start)
  ↓
**Evidence Assembly + Sign-Off** (Sept 30)
  ↓
**Wave A Q3 COMPLETE** (Oct 1)
  ↓
**Release Candidate Freeze** (Oct 20)

