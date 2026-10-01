# WAVE A Module Owner Quick-Start Guide

## TL;DR — What You Need to Do Right Now

**Each CRITICAL module owner has specific tasks for this week (Sept 2-8). This guide tells you exactly what to do.**

---

## TRANSACTION Module Owner

### Your 3 Gaps (8 days total effort)

| Gap | AC | Tests | Due | Effort |
|-----|----|----|-----|--------|
| **AC-6: Crash-Recovery** | 6 criteria | 12 | Sept 9 | 3 days |
| **AC-9/10: SAGA Idempotency** | 7 criteria | 14 | Sept 12 | 3 days |
| **AC-5: Timeout Semantics** | 3 criteria | 6 | Sept 8 | 2 days |

### What to Do TODAY (Sept 2-3)

1. **✅ Read your roadmap** (5 min):
   - Open: `src/transaction/ROADMAP.md` lines 22-24 (your 3 gaps)
   - Open: `src/transaction/ROADMAP.md` lines 52-59 (detailed ACs + test names)

2. **✅ Check test files exist** (10 min):
   - Run: `ls -la tests/transaction/test_transaction_*phase*.cpp`
   - Expected files:
     - `test_transaction_distributed_phase2.cpp` (AC-4/5/6) ✓
     - `test_transaction_saga_compensation_phase2.cpp` (AC-8/9/10) ✓
     - `test_transaction_fault_injection_phase3.cpp` (AC-11/12/13) ✓

3. **✅ Review implementation plan** (15 min):
   - Open: `ai_working/CRITICAL_MODULES_IMPLEMENTATION_PLAN.md`
   - Find "TRANSACTION" section → read your 3 gaps + test requirements

4. **✅ Try building** (30 min):
   - Run: `cmake --preset dev-nosandbox && cmake --build build-dev-nosandbox --target themis_transaction 2>&1 | head -30`
   - If it builds: ✓ Proceed to step 5
   - If it fails: Report error → escalate to blockers team (fmt, spdlog, etc. missing)

5. **✅ Run existing tests** (15 min):
   - Run: `cd build-dev-nosandbox && ctest -R "transaction" --verbose 2>&1 | tail -50`
   - Note pass/fail rates → report in Monday checkpoint

### Your First Task: AC-6 Crash-Recovery (Sept 2-8)

**Acceptance Criteria (6 total):**
1. Recovery under SIGKILL: Coordinator recovers from abrupt shutdown
2. Deterministic Rollback: Same crash scenario → same sequence
3. No Orphaned Locks: All locks released post-recovery
4. Contention Stress: ≥30s sustained contention, deterministic recovery
5. WAL Determinism: WAL entries in logical order (not timestamps)
6. Idempotent WAL Replay: Same WAL replayed N times → identical state

**Tests Required (12 total):**
- 3 Unit tests: RecoverFromSnapshot, StateStorage::Load, error handling
- 4 Integration tests: SIGKILL + recovery cycles with participants
- 3 Chaos tests: Network partition, unavailability, clock skew
- 2 Determinism tests: Scenario N times → identical sequence

**Implementation Guide:**
1. Open: `tests/transaction/test_coordinator_crash_recovery.cpp` (stub created Sept 2)
2. Implement each test → Replace `EXPECT_TRUE(false) << "TODO"` with real test code
3. Reference existing tests for patterns:
   - `tests/transaction/test_transaction_lifecycle_phase1.cpp` (WAL usage)
   - `tests/transaction/test_transaction_isolation_contention_phase1.cpp` (chaos patterns)
4. Verify tests compile + run locally
5. Commit with: `git commit -m "feat: Implement TRANSACTION AC-6 crash-recovery tests (12 tests)"`

**Success Criteria:**
- [ ] All 12 tests compile cleanly
- [ ] All 12 tests pass on develop branch CI
- [ ] No flakiness (run 5 times → same results)
- [ ] CI evidence captured → reported in Monday checkpoint

---

## GPU Module Owner

### Your 2 Gaps (18 days total effort)

| Gap | AC | Tests | Due | Effort |
|-----|----|----|-----|--------|
| **AC-1/2/3/5: Wrapper Adoption** | 5 criteria | 11 | Sept 15 | 8 days |
| **AC-4: Determinism** | 3 criteria | 5 | Sept 30 | 10 days* |

*Hardware-dependent; may slip if GPU access unavailable

### What to Do TODAY (Sept 2-3)

1. **✅ Read your roadmap** (5 min):
   - Open: `src/gpu/ROADMAP.md` lines 34-42 (Phase C pre-req)

2. **✅ Review implementation plan** (15 min):
   - Open: `ai_working/CRITICAL_MODULES_IMPLEMENTATION_PLAN.md`
   - Find "GPU" section → read your 2 gaps + test requirements

3. **✅ Get CUDA call audit baseline** (30 min):
   - Run: `python3 tools/ci/gpu_cuda_call_audit.py --output baseline_audit_2026_09_02.json 2>&1`
   - Expected: Output JSON with `unchecked_call_count: 340`
   - Save report → needed for final audit verification

4. **✅ Check wrapper infrastructure exists** (10 min):
   - Verify: `include/gpu/cuda_raii.h` ✓
   - Verify: `include/gpu/gpu_safe_raii.h` ✓
   - Verify: `include/gpu/gpu_raii_wrappers.hpp` ✓

### Your First Task: Wrapper Adoption (Sept 2-15)

**Acceptance Criteria (5 total):**
1. Audit Count ≤170: Verified via tools/ci/gpu_cuda_call_audit.py
2. RAII Wrapper Adoption: All new calls wrapped via CudaStreamGuard<T>
3. Audit Metadata: Each remaining 170 calls has inline comment explaining why
4. Wrapper Overhead: ≤5% latency overhead vs raw CUDA
5. No Regressions: All existing GPU tests pass

**Tests Required (11 total):**
- 5 Unit tests: CudaStreamGuard, CudaEventGuard, CudaDeviceMemoryGuard, error_check, exception safety
- 4 Integration tests: KernelExecutor, TensorAllocator, MemoryPool, audit count verification
- 1 Performance test: wrapper overhead measurement
- 1 Audit test: final count ≤170 + all have comments

**Implementation Guide:**
1. Open: `tests/gpu/test_cuda_wrapper_integration.cpp` (stub created Sept 2)
2. Implement each test → Replace `EXPECT_TRUE(false) << "TODO"` with real test code
3. In production code, adopt wrappers:
   - Replace raw `cudaStreamCreate()` with `CudaStreamGuard<cudaStream_t> stream(...)`
   - Replace raw `cudaMalloc()` with `CudaDeviceMemoryGuard device_mem(...)`
   - For remaining raw calls → add inline comment: `// UNCHECKED: <reason>`
4. Run audit: `python3 tools/ci/gpu_cuda_call_audit.py --output audit_2026_09_XX.json`
5. Verify: unchecked_call_count ≤ 170
6. Commit with: `git commit -m "feat: GPU wrapper adoption (340→170 CUDA calls, 11 tests)"`

**Success Criteria:**
- [ ] All 11 tests compile cleanly
- [ ] All 11 tests pass on GPU CI (if available; CPU mock otherwise)
- [ ] CUDA audit: unchecked_call_count ≤ 170
- [ ] All 170 remaining calls have inline audit comments
- [ ] Performance test: wrapper overhead ≤ 5%
- [ ] CI evidence captured → reported in Monday checkpoint

**Blocker Alert**: GPU determinism testing (AC-4) requires A100/H100 hardware. If unavailable by Sept 15, defer to Sept 20+ pending provisioning.

---

## QUERY Module Owner

### Your 1 Gap (35 days total effort, spans multiple teams)

| Phase | AC | Tests | Due | Effort |
|-------|----|----|-----|--------|
| **Design (Sept 2-10)** | 6 criteria | Design spec | Sept 10 ✅ | 5 days |
| **Phase 1-6 Implementation (Sept 15-Oct 30)** | - | 25 tests | Oct 30 | 30 days |

### What to Do TODAY (Sept 2-3)

1. **✅ Read your roadmap** (5 min):
   - Open: `src/query/ROADMAP.md` lines 74-80 (FTS executor backend)

2. **✅ Review design specification** (60 min):
   - Open: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` (25-test strategy, 6 implementation phases)
   - Read sections 1-3 (current state, architecture, data structures)
   - Read section 4 (API surface definition)
   - Read section 5 (performance targets)

3. **✅ Verify parser is complete** (15 min):
   - Run: `grep -n "parseSearchClause" src/query/fts_parser.cpp | head -3`
   - Expected: Implementation present (not stub)
   - Verify FTS tokens in lexer (SEARCH, PHRASE, NEAR, etc.)

4. **✅ Understand what's missing** (15 min):
   - Gap 1: FtsIndexLookup not implemented (no index query mechanism)
   - Gap 2: BM25Scorer incomplete (no actual scoring logic)
   - Gap 3: Index data structures not defined (on-disk layout TBD)
   - Gap 4: Thread safety not enforced (need shared_mutex)
   - Gap 5: Performance gates undefined (need ≤100ms SLA)

### Your First Task: Design Review + Approval (Sept 3-10)

**This is a GATE — implementation cannot start until design is approved.**

**What You Do:**
1. Schedule technical committee review (Sept 3-10)
   - Reviewers needed: @query-owner, @performance-owner, @thread-safety-owner
   - Review items: BM25 algorithm, index layout, thread safety model, error handling
   - Decision required: ON or AFTER Sept 10

2. Prepare review materials (Sept 3-5):
   - Print/share: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md`
   - Prepare 30-min presentation covering sections 2-7
   - Prepare discussion points: index format (binary vs RocksDB), thread-safety model, performance targets

3. Resolve design questions (Sept 6-9):
   - Questions from review → address in writing or design document updates
   - Design changes → document rationale + re-review if substantial

4. Get written approval (Sept 10):
   - Email approval: "@query-owner, @performance-owner, @thread-safety-owner approve design"
   - Archive approval email → link in evidence bundle

### Your Second Task: Phase 1 Implementation (Sept 15-22)

**Starting ONLY AFTER design approved (Sept 10).**

**Acceptance Criteria (3 for Phase 1):**
1. Index file I/O working: load header, vocabulary, posting lists
2. FtsIndexLookup operational: keyword → posting list lookup with error handling
3. Performance baseline: index load time measured for 100K-document index

**Tests Required (6 for Phase 1):**
- 2 Unit tests: file I/O round-trip, posting list parsing
- 2 Integration tests: index loading, keyword lookup
- 1 Performance test: load time benchmark
- 1 Error handling test: corrupted index recovery

**Implementation Guide:**
1. Create feature branch: `git checkout -b feature/query-fts-executor`
2. Implement in phases (see `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` section 6):
   - Phase 1 (5 days, Sept 15-22): Index loading + basic lookup
   - Phase 2 (6 days, Sept 22-28): BM25 scoring + merge
   - Phase 3 (10 days, Sept 28-Oct 8): Optimization + caching
   - Phase 4 (10 days, Oct 8-18): Thread safety + error handling
   - Phase 5 (10 days, Oct 18-28): Advanced features (parallel, timeout, snippets)
   - Phase 6 (2 days, Oct 28-30): Integration + verification

3. Commit per phase: `git commit -m "feat: QUERY FTS executor Phase 1 (index loading, 6 tests)"`

**Success Criteria:**
- [ ] Design approved by Sept 10
- [ ] Phase 1 implemented + tested by Sept 22
- [ ] All phase tests passing on CI
- [ ] Performance gate validated (index load time)
- [ ] No regressions to existing query tests

---

## Weekly Checkpoint Template (Mondays 09:00 UTC)

**Use this template to report progress each week:**

```
## MODULE: [Name] | WEEK: [Sept X-Y]

### Completed This Week
- [ ] [Gap 1 AC]: [1-2 sentence status]
- [ ] [Gap 2 AC]: [1-2 sentence status]

### Tests Status
- Total Required: [N] | Implemented: [M] | Passing: [P] | Flaky: [F]
- Details: [Brief description of any failures]

### Blockers
- [Blocker 1]: [Status + mitigation]
- [Blocker 2]: [Status + mitigation]

### Next Week Target
- [Target 1]
- [Target 2]

### CI Evidence
- Build Status: ✓ GREEN | ⚠ WARNINGS | ✗ FAILING
- Test Results: [Link to CI run]
- Coverage: [% if available]
```

**Example:**
```
## MODULE: TRANSACTION | WEEK: Sept 2-8

### Completed This Week
- [x] AC-6 (Crash-recovery): Test stubs created + crash-recovery test implementation 50% complete
- [x] AC-9/10 (SAGA): Existing tests verified (12 tests present in phase2)

### Tests Status
- Total Required: 32 | Implemented: 14 | Passing: 8 | Flaky: 1
- Details: test_coordinator_crash_recovery.cpp has 6 tests passing; 2 determinism tests have timing flakiness under load

### Blockers
- Build dependency: libfmt missing; workaround: `apt-get install libfmt-dev` ✓ RESOLVED

### Next Week Target
- Complete AC-6 tests by Sept 9
- Run full CI suite Sept 10 → capture evidence
- Start AC-5 timeout testing

### CI Evidence
- Build Status: ✓ GREEN (after fmt install)
- Test Results: https://github.com/makr-code/ThemisDB/actions/runs/XXXX
- Coverage: 87% in src/transaction/
```

---

## Important Links

**Master Documents:**
- Root ROADMAP: `ROADMAP.md` (lines 35-47: 14 CRITICAL modules)
- Implementation plan: `ai_working/CRITICAL_MODULES_IMPLEMENTATION_PLAN.md`
- Wave A status: `ai_working/WAVE_A_EXECUTION_STATUS_2026_09_02.md`

**Your Module Roadmaps:**
- TRANSACTION: `src/transaction/ROADMAP.md`
- GPU: `src/gpu/ROADMAP.md`
- QUERY: `src/query/ROADMAP.md`

**Design Specs:**
- QUERY FTS: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md`

**Test Stubs (Created Sept 2):**
- TRANSACTION: `tests/transaction/test_coordinator_crash_recovery.cpp`
- GPU: `tests/gpu/test_cuda_wrapper_integration.cpp`

---

## Questions?

**Escalation Path:**
1. Check your module roadmap (source of truth)
2. Check the design specification document
3. Check the implementation plan
4. Ask in Monday checkpoint sync
5. Escalate to module governance team

**Contact:**
- Module governance: @themis-module-governance
- Release coordination: @ga-release-owner
- Technical steering: @technical-steering-committee

---

**Document Version**: 1.0
**Generated**: 2026-09-02
**Status**: Ready for distribution to module owners
