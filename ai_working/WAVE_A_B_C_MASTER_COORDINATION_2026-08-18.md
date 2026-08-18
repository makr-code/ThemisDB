# Wave A → B → C Master Coordination Plan
**Date:** 2026-08-18 05:15 UTC  
**Status:** 🟢 PHASE 1 LAUNCHED (Wave A Completion)  
**Coordinator:** Copilot Main Agent  

---

## Executive Summary

Comprehensive multi-wave gap closure initiative executing in parallel:
- **Phase 1 (NOW)**: Wave A Completion - Batches A-8 (Voice) & A-9 (GPU) with parallel agents
- **Phase 2 (2-3 days)**: Wave B Stabilization - Server Phase 2 & documentation closure
- **Phase 3 (Q4 2026)**: Batch 7 Launch - Critical priority modules for GA release

**Parallel Architecture**: 2 themisdb-implementer agents (Voice, GPU) + 1 themisdb-reviewer agent for code audit  
**Total Gaps Being Addressed**: 29+ CRITICAL, 50+ HIGH  
**User Preference**: Larger remediation batches per commit ("weiter"/"nächster block")

---

## PHASE 1: WAVE A COMPLETION (Current - 1-2 Days)

### Status Dashboard

| Batch | Module | Gaps (CRITICAL/HIGH) | Status | Target | Agent |
|-------|--------|------|--------|--------|-------|
| A-8 | Voice | 13/32 | 🟡 LAUNCHING | 1-2 hrs | voice-batch-a8-hardening |
| A-9 | GPU | 16/117 | 🟡 LAUNCHING | 1-2 hrs | gpu-batch-a9-safety |
| **Wave A Exit** | Multiple | - | ⏳ PENDING | Q4 2026 | TBD |

### Batch A-8: Voice Fail-Closed Hardening
**Agent ID**: voice-batch-a8-hardening  
**Target Completion**: 2026-08-18 07:00 UTC  

**CRITICAL Gaps (13 total)**:
1. Reject malformed/oversized streams (11 gaps)
   - Location: voice_assistant.cpp, voice_browser_streaming.cpp
   - Work: Add MAX_VOICE_CHUNK_SIZE validation, frame format checks
   
2. Session lifecycle validation (6 gaps)
   - Location: voice_session_manager.cpp
   - Work: Enforce CREATED→ACTIVE→CLOSED state machine
   - Implementation: std::atomic<SessionState>, guard patterns

3. Multi-session teardown safety (3 gaps)
   - Location: voice_session_manager.cpp
   - Work: Prevent deadlocks, 10ms timeout, resource cleanup

4. Audit logging for security functions (3 gaps)
   - Location: voice_assistant.cpp (lines 144, 264, 659)
   - Work: Add [AUDIT] prefix logs for authenticate() calls

**Test Coverage**: test_voice_batch_a8_focused.cpp (6+ test cases)
**Files Modified**: 5 core files + 1 test file
**Expected Outcome**: ✅ Production-ready, backward compatible, zero regressions

### Batch A-9: GPU Fallback & Timeout Safety
**Agent ID**: gpu-batch-a9-safety  
**Target Completion**: 2026-08-18 07:00 UTC  

**CRITICAL Gaps (16 total)**:
1. Unchecked CUDA calls (170 of 340 target)
   - Phase C prerequisite: 50% reduction
   - Location: query_accelerator.cpp (8 CRITICAL), unified_memory.cpp (4 CRITICAL)
   - Work: Add cudaError_t checks, error logging, CPU fallback
   
2. RAII lifecycle enforcement (30+ of 57 target)
   - Location: query_accelerator.cpp, unified_memory.cpp, memory_pool.cpp
   - Work: Create GPUMemoryHandle, GPUStreamHandle, GPUEventHandle RAII wrappers

3. Kernel timeout enforcement (5-second hard limit)
   - Location: query_accelerator.cpp, time_slice_scheduler.cpp
   - Work: Add cudaStreamAddCallback or watchdog thread for timeout
   - Fail-closed to CPU on timeout

4. CPU degradation paths (all GPU failures)
   - Location: GPU dispatch paths
   - Work: Catch all GPU errors, retry on CPU with same semantics

**Test Coverage**: test_gpu_batch_a9_safety_focused.cpp (5+ test cases, ASan/UBSan clean)
**Files Modified**: 7 core files + 1 test file
**Expected Outcome**: ✅ 50% CUDA call coverage, 30+ RAII gaps, Phase C gate unlocked

### Wave A Exit Criteria (Q4 2026 deadline)
- [ ] Deterministic chaos evidence for transaction/sharding/replication recovery
- [ ] Fail-closed behavior verification for all distributed paths
- [ ] P95/P99 hardware baselines refreshed
- [ ] `release_critical` CI green on `develop`

**Coordination**: After A-8 & A-9 complete, launch Wave A Exit Criteria verification phase (Batches A-10+)

---

## PHASE 2: WAVE B STABILIZATION (2-3 Days)

### Status Dashboard

| Module | Phase | Gaps (Remaining) | Status | Next Steps |
|--------|-------|--------|---------|-----------|
| Search | Complete | - | ✅ DONE | Documentation review |
| Sharding | Complete | - | ✅ DONE | Gate unlock (Phase C) |
| Replication | Complete | - | ✅ DONE | Deployment readiness |
| Server | Phase 2 | 34 remaining | 🟡 QUEUED | query_api_handler (15), http2_session (9) |

### Server Phase 2: Copy Overhead & Connection Pool Hardening
**Target Completion**: 2026-08-20  
**Files to Address**:
- query_api_handler.cpp (15 gaps): Connection pool pre-allocation, buffer reservation
- http2_session.cpp (9 gaps): Stream buffer management, session lifecycle
- rope_api_handler.cpp (4 gaps): Rope protocol optimization
- export_api_handler.cpp (4 gaps): Export buffer efficiency

**Expected Outcome**: ✅ +5-15% throughput on high-concurrency paths

### Wave B Documentation & Finalization
**Tasks**:
- [ ] Search: ROADMAP.md verification, test gate execution
- [ ] Sharding: Phase C gate unlock (lock hierarchy 95→0 violations verified)
- [ ] Replication: Deployment readiness checklist
- [ ] Server: Phase 2 acceptance report

**Expected Outcome**: ✅ All Wave B modules production-ready for release

---

## PHASE 3: BATCH 7 LAUNCH (Q4 2026 Critical Priority)

### Batch 7: GA Release Blocker Modules

| Module | Status | Target | Gaps (C/H) | Focus |
|--------|--------|--------|--------|--------|
| ethics_ai | Phase B | 2026-10-31 | 13/22 | ChainVisualizer, NormEvidence, CSEP tests (8+) |
| security | Phase 3 | 2026-10-31 | TBD* | TSA hardening ✅, Vault/HSM/PKI remaining |
| audit | Phase 2 | 2026-10-31 | - | Integrity verification, high-volume export, SBOM audit |
| observability | Phase 3 | 2026-10-31 | - | Distributed tracing, high-cardinality stress, alert correlation |

*Security module needs fresh full scan to replace legacy aggregate counts

### ethics_ai Detailed Gap Analysis
**CRITICAL Gaps (13)**:
- iterator_invalidation (1): ethics_selection_router.cpp:211
- argument_store data corruption (8): Uninitialized member fields
- rag_context_engine async safety (2): Thread safety issues
- model_integrity_gap (8 total HIGH): Ensure model bias detection accuracy

**Implementation Checklist**:
1. [ ] ChainVisualizer completion: End-to-end visualization of ethical reasoning chain
2. [ ] NormEvidence validation: Norm database consistency and query correctness
3. [ ] CSEP test suite: 8+ focused regression tests for EU AI Act Art. 13/22 compliance
4. [ ] Bias detector: Cross-school ethical tension resolution accuracy

**Test Strategy**: Comprehensive test suite with deterministic seeds, no external I/O
**Expected Outcome**: ✅ EU AI Act compliance verified, ready for regulatory audit

### security Module Roadmap (Post-Batch 4 TSA fix)
**Outstanding Work**:
- [ ] Full module re-scan (replace legacy aggregate counts)
- [ ] Vault integration: Credential rotation, failover
- [ ] HSM integration: Key lifecycle, distributed key management
- [ ] PKI integration: Certificate management, renewal policies
- [ ] RLS validation: Real-workload testing, policy conflict resolution

**Expected Outcome**: ✅ Production-grade key management infrastructure

### audit & observability Modules
**Key Requirements**:
- **audit**: Integrity verification, high-volume export (1000+ ops/sec), SBOM audit trail
- **observability**: Distributed tracing, high-cardinality stress (10k+ dimension values), alert correlation

**Timeline**: Start after ethics_ai Phase B completion (mid-October)

---

## Parallel Agent Coordination

### Current Parallel Execution (Phase 1)

```
Main Coordinator Thread (this)
├─ Agent: voice-batch-a8-hardening
│  ├─ Malformed/oversized stream rejection
│  ├─ Session lifecycle validation  
│  ├─ Multi-session teardown safety
│  └─ Audit logging
│
├─ Agent: gpu-batch-a9-safety
│  ├─ Unchecked CUDA calls (50% target)
│  ├─ RAII lifecycle enforcement
│  ├─ Kernel timeout guarantees
│  └─ CPU degradation paths
│
└─ Coordinating Phase 2 & 3 preparation
   ├─ Server Phase 2 gap analysis
   ├─ ethics_ai Batch 7 planning
   └─ Test & documentation coordination
```

### Execution Workflow

**T+0 hrs (Now)**: 
- ✅ Phase 1 agents launched (Voice, GPU)
- ✅ Phase 2 planning started
- ✅ Phase 3 gap analysis prepared

**T+1-2 hrs**: 
- 🟡 Phase 1 agents complete (expect commits c1, c2)
- 🔄 Code review (via themisdb-reviewer)
- 🔄 Test execution validation

**T+2-4 hrs**:
- 🟡 Phase 1 merge to develop
- ✅ Phase 2 (Server Phase 2) launch
- 🔄 Wave A Exit Criteria planning

**T+2-3 days**:
- 🟡 Phase 2 complete (Server, documentation)
- ✅ Phase 3 (ethics_ai Batch 7) ready to launch
- 📅 Q4 2026 milestone tracking

---

## Acceptance Criteria & Quality Gates

### Phase 1 (Wave A Completion)

**Voice Batch A-8**:
- ✅ 13+ CRITICAL gaps fixed (verified in source code)
- ✅ New tests passing (PASS)
- ✅ No regressions in existing voice tests
- ✅ Zero compiler warnings/errors
- ✅ Backward compatible (no public API changes)

**GPU Batch A-9**:
- ✅ 50% unchecked CUDA calls fixed (340 → 170)
- ✅ 30+ RAII lifecycle gaps resolved
- ✅ Kernel timeout enforcement verified
- ✅ All GPU failures degrade to CPU cleanly
- ✅ ASan/UBSan clean (no leaks or undefined behavior)

**Wave A Exit Criteria**:
- ✅ Transaction/Sharding/Replication chaos evidence complete
- ✅ Fail-closed behavior verified for all distributed paths
- ✅ P95/P99 hardware baselines refreshed
- ✅ `release_critical` CI green on `develop`

### Phase 2 (Wave B Stabilization)

**Server Phase 2**:
- ✅ 34/139 remaining gaps addressed (Phase 1 gaps already done)
- ✅ +5-15% throughput verified on high-concurrency paths
- ✅ Zero regressions (all tests PASS)
- ✅ Phase 2 acceptance report complete

**Wave B Documentation**:
- ✅ All module ROADMAP.md updated
- ✅ Test gates defined and executable
- ✅ Benchmark baselines established
- ✅ Production readiness checklist complete

### Phase 3 (Batch 7 Launch)

**ethics_ai**:
- ✅ 13+ CRITICAL gaps fixed
- ✅ ChainVisualizer end-to-end working
- ✅ NormEvidence validation passed
- ✅ CSEP test suite (8+) implemented and passing
- ✅ EU AI Act compliance verified

**security**:
- ✅ Fresh full-module scan completed
- ✅ Vault/HSM/PKI integration production-ready
- ✅ RLS validation in real-world scenarios
- ✅ Policy conflict resolution working

---

## Risk Mitigation

### Phase 1 Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| CUDA toolchain unavailable | Low | High | CPU fallback tests, no CUDA hardware assumption |
| Voice session deadlock | Low | High | std::scoped_lock, lock ordering validation |
| Exception safety violation | Medium | High | RAII pattern enforcement, test injection |

### Phase 2 Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Server Phase 2 regressions | Medium | Medium | Full test suite execution, performance profiling |
| Wave B gate unlock delay | Low | High | Parallel phase execution, documentation ready |

### Phase 3 Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| ethics_ai compliance gap | Medium | Critical | Regulatory review, 8+ test cases |
| security module re-scan needed | High | Medium | Schedule fresh scan immediately after Phase 1 |

---

## Timeline & Milestones

```
2026-08-18
  T+0:     Phase 1 launch (Voice A-8, GPU A-9)
  T+1-2h:  Phase 1 agents complete & review
  T+2-4h:  Phase 1 merge, Phase 2 launch
  
2026-08-20
  T+2d:    Phase 2 complete (Server Phase 2)
  T+2.5d:  Phase 3 planning complete
  
2026-08-22
  Wave A Exit Criteria planning & execution start
  
2026-10-15
  Tier 2 (performance-gate blockers) ready: server, llm, tensor
  
2026-10-31
  Batch 7 critical modules complete: ethics_ai, security, audit, observability
  
2026-11-30
  Wave B complete & Wave C starting
  
2026-12-15
  Feature completeness modules complete
  
2027-01-31
  Medium priority modules complete (hardening & cleanup)
  
2027-Q1
  Wave A → B → C consolidation & release preparation complete
```

---

## Key File References

**Phase 1**:
- src/voice/ROADMAP.md (Voice module status)
- src/gpu/ROADMAP.md (GPU module status, Phase C gates)
- tests/voice/test_voice_batch_a8_focused.cpp (new test suite)
- tests/gpu/test_gpu_batch_a9_safety_focused.cpp (new test suite)

**Phase 2**:
- src/server/ROADMAP.md (Phase 2 details)
- WAVE_B_QUICK_REFERENCE.md (Server Phase 2 breakdown)
- WAVE_B_COMPLETION_SUMMARY.md (Wave B verification report)

**Phase 3**:
- src/ethics_ai/MODULE_GAPS.md (13 CRITICAL gaps)
- src/security/MODULE_GAPS.md (residual work tracking)
- BATCH_7_13_REMAINING_MODULES_INVENTORY.md (full Batch 7+ inventory)

---

## Next Steps for Main Coordinator

1. **Monitor Phase 1 agents** (voice-batch-a8-hardening, gpu-batch-a9-safety)
   - Check for completion notifications
   - Verify commits have proper test coverage
   
2. **Prepare Phase 2 execution**
   - Launch themisdb-reviewer for code audit of Phase 1
   - Queue Server Phase 2 implementation
   
3. **Stage Phase 3 planning**
   - Coordinate ethics_ai gap analysis
   - Schedule security module fresh full-scan
   
4. **Coordinate Wave A Exit Criteria**
   - Plan chaos injection test suite
   - Baseline performance collection
   - Release_critical CI gating

---

**Status**: 🟢 PHASE 1 ACTIVE | Agents working in parallel  
**Next Update**: 2026-08-18 07:00 UTC (after Phase 1 agents complete)
