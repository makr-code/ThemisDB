# Sub-Agent Orchestration Protocol — Stream A (Q3 2026 Tensor Hardening)

**Status**: LAUNCH PROTOCOL (Pre-Deployment)  
**Created**: 2026-08-08T18:23:01Z  
**Target Deployment**: Immediate (Aug 8, 2026)  
**Scope**: Tensor Module Stream A Q3 Hardening & Diagnostics  
**Parent Plan**: `TENSOR_IMPLEMENTATION_MASTER_PLAN.md`  

---

## Executive Summary

This document defines the **parallel sub-agent orchestration protocol** for Stream A execution (Aug 7–31, 2026). Three specialized agents run in parallel across three implementation blocks (A1/A2/A3), with explicit coordination, dependency management, and weekly integration checkpoints.

**Pilot Workflow**: Flash Attention E8 validation (background agent launched 2026-08-08T18:23:01Z) validates the orchestration framework before full Stream A deployment.

---

## Agent Registry & Roles

### A1: Concurrent Hardening (themisdb-implementer)
- **Agent ID**: `tensor-a1-hardening`
- **Repository Role**: Core implementation and hardening
- **Primary Deliverables**:
  - Concurrency fixes in `tensor_index_manager.cpp`, `tensor_ingestion_bridge.cpp`
  - Test suite: `test_tensor_index_manager_concurrent_focused.cpp` (TNCI-01..24)
  - Test suite: `test_tensor_ingestion_bridge_concurrent_focused.cpp` (TNIC-01..16)
  - Report: `CONCURRENT_HARDENING_FINDINGS.md`
- **Success Criteria**:
  - All tests pass 100+ iterations
  - ThreadSanitizer clean (no race conditions)
  - Throughput scales with thread count
- **Duration**: Aug 7–21, 2026 (1-2 weeks)
- **Status**: 🟡 PLANNED

### A2: Diagnostics Unification (themisdb-reviewer)
- **Agent ID**: `tensor-a2-diagnostics`
- **Repository Role**: Validation and hardening review
- **Primary Deliverables**:
  - Unified `emitDiagnostic()` in `tensor_error_handling.cpp`
  - Updates to diagnostic calls in `tensor_index_manager.cpp`, `tensor_core_bridge.cpp`, `tensor_fingerprint_graph.cpp`
  - Test suite: `test_tensor_diagnostics_integration_focused.cpp` (TDIAG-01..24)
  - Report: `DIAGNOSTIC_AUDIT.md`
- **Success Criteria**:
  - >95% error path coverage
  - Consistent diagnostic format
  - Zero silent failures
- **Duration**: Aug 7–21, 2026 (1-2 weeks, parallel with A1)
- **Status**: 🟡 PLANNED

### A3: Benchmark Baselining (task runner)
- **Agent ID**: `tensor-a3-benchmarks`
- **Repository Role**: Performance validation and reporting
- **Primary Deliverables**:
  - Enhanced/validated benchmarks: `bench_tensor_fingerprint_graph.cpp`, `bench_tensor_deduplication_manager.cpp`
  - Baseline report: `TENSOR_Q3_BENCHMARK_BASELINE.md` (p95/p99 locked values)
  - Validation against `FUTURE_ENHANCEMENTS.md` targets
- **Success Criteria**:
  - All targets met
  - p95/p99 stable (<2% variance)
  - Baseline reproducible
- **Duration**: Aug 7–27, 2026 (independent track, can start immediately)
- **Status**: 🟡 PLANNED

---

## Parallel Execution Model

### Block Dependencies

```
A1 (Concurrent Hardening)  ┐
A2 (Diagnostics)          ├─→ Integration (Aug 22-31) → PR Merge (Aug 30-31)
A3 (Benchmarks)           ┘
```

**Dependency Rules**:
- A1 and A2 have **interface dependencies**: diagnostic calls in A1 code must use interfaces defined in A2
- A3 (benchmarks) is **independent**: can run in parallel, but feeds into Stream B validation (Sept 1+)
- No blocking dependencies between A1, A2, A3: all three can start immediately on Aug 7

### Coordination Points

**Weekly Sync** (Wednesdays 2026-08-14, 2026-08-21, 2026-08-28):
- A1 status: test count, pass rate, ThreadSanitizer findings
- A2 status: error paths catalogued, diagnostic format validated
- A3 status: benchmark runs completed, p95/p99 measurements
- Risk register update
- Escalation path for blockers

**Integration Checkpoint** (Aug 22-31):
- All A1/A2/A3 blocks feature-complete
- Tests passing on current `develop` baseline
- Performance targets validated
- Merge PR to `develop`: Aug 30-31

---

## Launch Protocol

### Pre-Launch Checklist (NOW — 2026-08-08T18:23:01Z)

- [x] Pilot validation started: **Flash Attention E8 workflow** (general-purpose agent)
  - Validates agent infrastructure and reporting mechanisms
  - Expected completion: 2026-08-08 (async, <4 hours)
  - Output: `ai_working/FLASH_ATTENTION_E8_PILOT_VALIDATION.md`

- [ ] Confirm agent infrastructure availability:
  - `themisdb-implementer` role enabled
  - `themisdb-reviewer` role enabled
  - Task runner (benchmarks) infrastructure ready
  - CI/CD gates functional (build, test, benchmark, security)

- [ ] Confirm baseline environment:
  - `develop` branch current and passing `release_critical` CI
  - CMake preset `windows-release` or `linux-release` ready
  - CTest labels registered: `tensor;hardening;phase1` (or renamed per stream)
  - Benchmark infrastructure ready: microbench, macro-bench, graph profiling

### Agent Launch Sequence (PROPOSED — Aug 8, 2026, 18:30 UTC)

**Phase 1 — Immediate Launch (Aug 8, 2026, 18:30 UTC)**

1. **A1 Agent Spin-up** (`themisdb-implementer`, `tensor-a1-hardening`)
   - Task: Begin concurrent hardening analysis in tensor_index_manager.cpp
   - Expected Duration: 10-14 days (Aug 8-21)
   - Deliverable: CONCURRENT_HARDENING_FINDINGS.md (draft by Aug 15)
   - Success Metric: First 50+ tests passing, ThreadSanitizer zero-race baseline

2. **A2 Agent Spin-up** (`themisdb-reviewer`, `tensor-a2-diagnostics`)
   - Task: Design unified diagnostic interface and audit error paths
   - Expected Duration: 7-10 days (Aug 8-18)
   - Deliverable: DIAGNOSTIC_AUDIT.md (draft by Aug 14)
   - Success Metric: Error path mapping >95% complete, diagnostic format frozen

3. **A3 Agent Spin-up** (`task`, `tensor-a3-benchmarks`)
   - Task: Validate existing benchmarks and collect baseline measurements
   - Expected Duration: 10-14 days (Aug 8-21)
   - Deliverable: TENSOR_Q3_BENCHMARK_BASELINE.md (draft by Aug 18)
   - Success Metric: All target measurements complete, p95/p99 stable

**Phase 2 — Integration & Closure (Aug 22-31, 2026)**

- Merge A1/A2/A3 findings into coherent PR
- Final test pass on `develop` CI gates
- Prepare for Stream B launch (Sept 1)

---

## Coordination Framework

### Communication Protocol

**Agent-to-Agent Interface**:
- A1 ↔ A2: Diagnostic interface (`tensor_error_handling.cpp`, `emitDiagnostic()` contract)
- A1 → A3: Test performance measurements feed baseline calibration
- A2 → A3: Diagnostic overhead must stay <5% of benchmark runtime

**Agent-to-Human Interface**:
- Weekly async standup via status in this file (UPDATED section below)
- Escalation: Blocker → `makr-code` via GitHub issue or direct notification
- Final checkpoint: Aug 29 (all A1/A2/A3 ready for merge review)

### Risk Management

**A1 Risks**:
- ThreadSanitizer may report false positives in lock-free data structures → requires domain expert review
- **Mitigation**: Keep lock-free patterns minimal; pre-approved list in code review

**A2 Risks**:
- Diagnostic format change may break downstream log parsers → requires careful versioning
- **Mitigation**: Version diagnostic messages; provide migration helper

**A3 Risks**:
- Hardware-variance in benchmark results may exceed 2% tolerance → requires stable CI environment
- **Mitigation**: Lock benchmark frequency, CPU governor, thermal profile on CI runner

---

## Status Tracking (LIVE)

### Latest Report (2026-08-08T18:23:01Z)

| Agent | Block | Status | Progress | Next Checkpoint |
|-------|-------|--------|----------|-----------------|
| `tensor-a1-hardening` | A1 | 🟡 PLANNED | — | Aug 15 (draft findings) |
| `tensor-a2-diagnostics` | A2 | 🟡 PLANNED | — | Aug 14 (draft audit) |
| `tensor-a3-benchmarks` | A3 | 🟡 PLANNED | — | Aug 18 (draft baseline) |
| **PILOT** | Flash E8 | 🔵 IN PROGRESS | Started 18:23Z | 2026-08-08 23:59Z |

### Weekly Checkpoints

**Week 1 (Aug 7-13)**:
- [ ] A1: Concurrency analysis in progress, initial test cases defined
- [ ] A2: Error path inventory started, diagnostic interface draft
- [ ] A3: Benchmark infrastructure validated, measurement protocol finalized
- [ ] Pilot: Flash Attention E8 validation report delivered

**Week 2 (Aug 14-20)**:
- [ ] A1: CONCURRENT_HARDENING_FINDINGS.md draft; 50+ tests passing
- [ ] A2: DIAGNOSTIC_AUDIT.md draft; error paths >95% catalogued
- [ ] A3: TENSOR_Q3_BENCHMARK_BASELINE.md draft; p95/p99 locked
- [ ] Integration: Dependencies validated (A1 → A2 interface ready)

**Week 3 (Aug 21-27)**:
- [ ] A1: Final test suite complete (TNCI-01..24 + TNIC-01..16); all pass
- [ ] A2: All error paths instrumented; zero silent failures validated
- [ ] A3: Benchmark suite finalized; performance targets confirmed PASS
- [ ] PR: Integration PR `feature/tensor-q3-hardening` created

**Week 4 (Aug 28-31)**:
- [ ] PR Review: Code review and sign-off
- [ ] Final CI Gate: All `release_critical` tests PASS on PR
- [ ] Merge: Merge to `develop` (Aug 30-31)

---

## Deliverables Checklist

### A1 Deliverables (Concurrent Hardening)

- [ ] `src/llm/tensor_index_manager.cpp` — concurrency hardening applied
- [ ] `src/llm/tensor_ingestion_bridge.cpp` — bounded concurrency controls added
- [ ] `tests/llm/test_tensor_index_manager_concurrent_focused.cpp` — TNCI-01..24 tests
- [ ] `tests/llm/test_tensor_ingestion_bridge_concurrent_focused.cpp` — TNIC-01..16 tests
- [ ] `ai_working/CONCURRENT_HARDENING_FINDINGS.md` — issue tracking report

### A2 Deliverables (Diagnostics Unification)

- [ ] `src/llm/tensor_error_handling.cpp/h` — unified `emitDiagnostic()` helper
- [ ] `src/llm/tensor_index_manager.cpp` — diagnostic call integration
- [ ] `src/llm/tensor_core_bridge.cpp` — diagnostic call integration
- [ ] `src/llm/tensor_fingerprint_graph.cpp` — diagnostic call integration
- [ ] `tests/llm/test_tensor_diagnostics_integration_focused.cpp` — TDIAG-01..24 tests
- [ ] `ai_working/DIAGNOSTIC_AUDIT.md` — error path mapping report

### A3 Deliverables (Benchmark Baselining)

- [ ] `benchmarks/llm/bench_tensor_fingerprint_graph.cpp` — enhanced/validated
- [ ] `benchmarks/llm/bench_tensor_deduplication_manager.cpp` — enhanced/validated
- [ ] `ai_working/TENSOR_Q3_BENCHMARK_BASELINE.md` — baseline report with gates
- [ ] Benchmark CI gates: GATE-TN-A3-01..06 defined and PASS

### Integration Deliverables

- [ ] PR `feature/tensor-q3-hardening` created and linked
- [ ] All A1/A2/A3 artifacts integrated into `ai_working/` or source directories
- [ ] Weekly sync notes consolidated into `STREAM_A_Q3_2026_TRACKING.md` (update)

---

## Next Phase Transition (Stream B Launch Sept 1)

Upon successful Stream A closure (Aug 30-31), the orchestration framework immediately transitions to Stream B (Q4 2026 Determinism & Coverage):

- B1: Edge Case Determinism (`themisdb-implementer`, `tensor-b1-edges`)
- B2: Stress Coverage (`general-purpose`, `tensor-b2-stress`)
- B3: P95/P99 Validation (`task`, `tensor-b3-validation`)

**Prerequisites for Stream B Launch**:
- Stream A PR merged to `develop`
- All A1/A2/A3 acceptance criteria met
- Weekly sync notes reflect zero-risk closure
- CI gates `release_critical` green for 48+ hours

---

## Appendix A: Reference Documents

- **Parent Plan**: `TENSOR_IMPLEMENTATION_MASTER_PLAN.md`
- **Stream A Tracking**: `STREAM_A_Q3_2026_TRACKING.md`
- **Tensor ROADMAP**: `src/tensor/ROADMAP.md`
- **Agent Definitions**:
  - `themisdb-implementer`: `.github/agents/themisdb-implementer.agent.md`
  - `themisdb-reviewer`: `.github/agents/themisdb-reviewer.agent.md`
- **CI Configuration**: `.github/workflows/09-pr-gates_release-critical-tests.yml`
- **CMake Targets**: `tests/tensor/CMakeLists.txt`, `benchmarks/tensor/CMakeLists.txt`

---

## Appendix B: Pilot Validation Workflow (Flash Attention E8)

**Pilot Status**: RUNNING (started 2026-08-08T18:23:01Z)  
**Expected Output**: `ai_working/FLASH_ATTENTION_E8_PILOT_VALIDATION.md`  
**Validation Purpose**: Confirm agent infrastructure works before full Stream A deployment  
**Success Criterion**: Pilot completes with actionable validation report; orchestration framework confirmed stable

---

**Document Owner**: @makr-code  
**Last Updated**: 2026-08-08T18:23:01Z  
**Next Review**: 2026-08-14 (Week 1 checkpoint)
