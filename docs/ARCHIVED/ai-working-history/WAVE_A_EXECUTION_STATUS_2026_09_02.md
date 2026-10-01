# WAVE A Execution Status — 14 CRITICAL Modules (2026-09-02 Baseline)

## Executive Summary

**Program Status**: Wave A Immediate (Weeks 1-4, Sept 2-30)
- 3 CRITICAL modules enter implementation phase (TRANSACTION, GPU, QUERY)
- 6 infrastructure blockers identified and prioritized
- 95+ concrete acceptance criteria defined with test mappings
- ~260 total tests required across 14 CRITICAL modules

---

## IMMEDIATE (Week 1-2: Sept 2-15)

### 🔴 TRANSACTION (Crash-Recovery + SAGA + Timeout)
**Roadmap Status**: Lines 22-24 (in-progress, build verification pending)
**Gap Analysis**: 3 concrete ACs, 32 required tests, 8-day effort

| Gap | AC Count | Tests | Status | Target |
|-----|----------|-------|--------|--------|
| AC-6: Crash-recovery + WAL | 6 | 12 | Stub created | Sept 9 |
| AC-9/10: SAGA idempotency | 7 | 14 | Existing (phase 2) | Sept 12 |
| AC-5: Timeout semantics | 3 | 6 | Existing (phase 1) | Sept 8 |

**Verification Needed**:
- [ ] Existing tests build cleanly (test_transaction_distributed_phase2.cpp, test_transaction_saga_compensation_phase2.cpp, test_transaction_fault_injection_phase3.cpp)
- [ ] CI execution confirms zero failures (pending)
- [ ] Crash-recovery tests implemented + execute successfully
- [ ] WAL determinism validation under chaos

**Blockers**:
- None (tests exist; need build + execution verification)

**Owner**: @transaction-module-owner | **Effort**: 8 days | **FTE**: 1

---

### 🟡 GPU (CUDA Wrapper Adoption + Determinism)
**Roadmap Status**: Lines 34-42 (Phase C pre-req: 340→170 reduction, IN PROGRESS)
**Gap Analysis**: 2 concrete ACs, 16 required tests, 18-day effort

| Gap | AC Count | Tests | Status | Target |
|-----|----------|-------|--------|--------|
| AC-1/2/3/5: Wrapper adoption | 5 | 11 | Design + stubs | Sept 15 |
| AC-4: Determinism validation | 3 | 5 | Blocked on hardware | Sept 30 |

**Verification Needed**:
- [ ] CUDA call audit baseline (tools/ci/gpu_cuda_call_audit.py) → confirm 340 unchecked calls
- [ ] Wrapper adoption in KernelExecutor, TensorAllocator, MemoryPool
- [ ] All 170 remaining calls have inline "UNCHECKED: reason" comments
- [ ] Determinism testing on A100/H100 GPUs (requires hardware access)

**Blockers**:
- **🔴 Hardware access**: Determinism testing requires A100 or H100 GPU (not available in current environment)
  - **Workaround**: Run determinism baseline on CPU mock; defer hardware validation to Sept 20+ after provisioning
- **🟡 Audit tool availability**: Ensure tools/ci/gpu_cuda_call_audit.py executable in CI environment

**Owner**: @gpu-module-owner | **Effort**: 18 days | **FTE**: 1-2

---

### 🟠 QUERY (FTS Executor Backend + Design Approval)
**Roadmap Status**: Lines 74-80 (Parser complete; executor backend MISSING; design in progress)
**Gap Analysis**: Design phase + 2 phases implementation, 25 required tests, 35-day effort

| Phase | AC Count | Tests | Status | Target |
|-------|----------|-------|--------|--------|
| Design (Sept 2-10) | 6 | Design spec | Draft complete | Sept 10 ✅ |
| Phase 1: Index Loading (Sept 15-22) | 3 | 6 | Not started | Sept 22 |
| Phase 2-6: Core+Optimization (Sept 22-30) | - | 19 | Not started | Oct 30 |

**Verification Needed**:
- [ ] **GATE: Technical committee approval of design spec** (Sept 3-10)
  - Reviewers: @query-owner, @performance-owner, @thread-safety-owner
  - Review items: BM25 algorithm, index layout, thread safety, performance targets
- [ ] Design-to-code walkthrough (Sept 10-12)
- [ ] Phase 1 implementation + unit tests (Sept 15-22)
- [ ] Phase 2-3 implementation + integration tests (Sept 22-28)
- [ ] Phase 4-6 optimization, error handling, determinism (Sept 28-30)

**Blockers**:
- **🔴 Design approval pending**: Cannot start implementation until Sept 10 design review complete
  - **Risk**: Design review delay → implementation slips into Q4
- **🟡 Index format decision**: On-disk format not yet finalized (spec proposes binary format; alternative: RocksDB prefix-tree)

**Owner**: @query-module-owner | **Effort**: 35 days | **FTE**: 3 (parallel phases)

---

## Q3 WAVE (Week 3-4: Sept 15-30)

### 🟠 SERVER (Security Audit Closure)
**Roadmap Status**: Blocking GA certification
**Gap**: ACL + RBAC verification, security compliance validation
**Effort**: 12 days | **FTE**: 1-2 | **Owner**: @security-owner

**Verification Needed**:
- [ ] Third-party security audit execution (vendor confirmation)
- [ ] Findings classification + remediation
- [ ] Evidence bundle: SECURITY_AUDIT_2026_Q3.md

---

### 🟠 STORAGE (AccessCoordinator Wiring)
**Roadmap Status**: Blocking distributed query execution
**Gap**: Coordinator not wired into AccessControl enforcement
**Effort**: 10 days | **FTE**: 1 | **Owner**: @storage-owner

**Verification Needed**:
- [ ] AccessCoordinator integration in QueryExecutor
- [ ] Permission check enforcement + error handling
- [ ] Integration tests: 8+ tests

---

## Infrastructure Blockers (Sept 2-30)

| Blocker | Impact | Priority | Action | Target |
|---------|--------|----------|--------|--------|
| CI Build Verification | ALL 14 modules | P0 🔴 | Test compile on develop; identify missing deps | Sept 5 |
| GPU Hardware Access | GPU determinism testing | P1 🟡 | Provision A100/H100 or defer to Sept 20 | Sept 15 |
| QUERY Design Approval | QUERY implementation | P1 🟡 | Schedule technical committee review | Sept 10 |
| Audit Tool Access | GPU wrapper audit | P2 🟠 | Verify tools/ci/gpu_cuda_call_audit.py callable in CI | Sept 8 |
| Dependencies (fmt, spdlog, etc.) | TRANSACTION tests | P2 🟠 | Install missing Ubuntu packages | Sept 3 |

---

## CI Execution Evidence Tracking

**Status**: Pending (to be populated as tests execute)

| Module | Test Suite | Build Status | Execution Status | Pass Rate | Evidence Bundle |
|--------|-----------|---|---|---|---|
| TRANSACTION | phase1/phase2/phase3 | TBD | TBD | TBD | TBD |
| GPU | wrapper/audit/performance | TBD | TBD | TBD | TBD |
| QUERY | parser/design_review | TBD | TBD | TBD | TBD |

---

## Weekly Checkpoints

### Week 1 (Sept 2-8)
- [ ] Sept 2: CI build verification for TRANSACTION, GPU, QUERY
- [ ] Sept 3-4: QUERY design review kick-off (technical committee)
- [ ] Sept 5: GPU CUDA call audit baseline execution
- [ ] Sept 6: TRANSACTION crash-recovery test implementation starts
- [ ] Sept 8: Infrastructure blocker triage + remediation

### Week 2 (Sept 9-15)
- [ ] Sept 10: QUERY design approval (gate: must pass before implementation)
- [ ] Sept 12: TRANSACTION crash-recovery tests complete + CI GREEN
- [ ] Sept 15: GPU wrapper adoption phase starts (parallel with QUERY Phase 1)
- [ ] Sept 15: QUERY Phase 1 implementation begins

### Week 3-4 (Sept 16-30)
- [ ] Weekly performance gate validation (query, GPU latency, TRANSACTION throughput)
- [ ] Weekly CI evidence capture (test results, audit reports)
- [ ] Parallel execution tracking: Q3 WAVE modules (SERVER, STORAGE)

---

## Dependencies & Critical Path

```
QUERY Design Spec (Sept 2-10)
  └─→ QUERY Design Approval (Sept 10) [GATE]
      └─→ QUERY Phase 1 Implementation (Sept 15-22)
          └─→ QUERY Phase 2-6 (Sept 22-Oct 30)

TRANSACTION Tests Exist (phase1/2/3)
  └─→ TRANSACTION CI Build Verification (Sept 5) [GATE]
      └─→ TRANSACTION Crash-Recovery Tests (Sept 6-9)
          └─→ TRANSACTION CI Execution (Sept 10-12)
              └─→ TRANSACTION Evidence Bundle (Sept 12)

GPU CUDA Audit Baseline (Sept 5)
  └─→ GPU Wrapper Adoption (Sept 9-15)
      └─→ GPU Determinism Testing (Sept 20+, hardware-dependent)
          └─→ GPU Evidence Bundle (Sept 30)

CRITICAL PATH: Design Approval (QUERY) + CI Verification (TRANSACTION) + Audit Baseline (GPU)
```

---

## Success Metrics (Wave A Exit Criteria)

### TRANSACTION ✅
- [ ] 3/3 gaps resolved (AC-6, AC-9/10, AC-5)
- [ ] 32/32 tests passing on CI
- [ ] 0 data loss under crash+contention scenarios
- [ ] WAL determinism verified (idempotent replay)

### GPU 🟡
- [ ] 50% reduction: 340 → ≤170 unchecked calls
- [ ] All 170 remaining calls have audit comments
- [ ] Wrapper overhead ≤ 5% on representative workloads
- [ ] Determinism validated (hardware permitting)

### QUERY 🟠
- [ ] Design approved + implementation started
- [ ] Phase 1 (index loading) complete + tested
- [ ] BM25 scoring ≤100ms on 100K documents

---

## Appendix: File References

**Test Stubs Created (Sept 2):**
- `tests/transaction/test_coordinator_crash_recovery.cpp` (12 tests)
- `tests/gpu/test_cuda_wrapper_integration.cpp` (11 tests)

**Design Specifications Created (Sept 2):**
- `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` (25-test strategy, 6 phases)

**Implementation Plan (Sept 1):**
- `ai_working/CRITICAL_MODULES_IMPLEMENTATION_PLAN.md` (detailed per-gap breakdown)

**Roadmap References (Source of Truth):**
- `ROADMAP.md` (root, lines 35-47: 14 CRITICAL modules)
- `src/transaction/ROADMAP.md` (lines 22-24: 3 gaps)
- `src/gpu/ROADMAP.md` (lines 34-42: Phase C pre-req)
- `src/query/ROADMAP.md` (lines 74-80: executor backend)

---

**Generated**: 2026-09-02 05:55 UTC
**Status**: Ready for team assignment and execution
**Next Action**: Distribute to module owners; schedule week-1 checkpoint (Sept 8)
