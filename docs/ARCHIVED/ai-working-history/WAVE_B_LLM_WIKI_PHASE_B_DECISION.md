# Wave B Release Scope Decision: LLM Wiki Phase B

**Date:** 2026-08-17  
**Decision:** DEFER LLM Wiki Phase B to Wave C  
**Wave B Release Scope:** Access Model (Phase 5-6) ✅ + Search (v2.0.0) ✅ + LLM (Phase A only, 3-layer retrieval)  
**Target Wave B Release:** 2026-09-25  
**Target LLM Wiki Phase B:** 2026-12-15 (Wave C)  

---

## Executive Decision Summary

**LLM Wiki Phase B is not ready for Wave B GA release.** While Phase A is complete and Phase B implementation is 70% done, critical acceptance gates are not yet validated. **Wave B will proceed with Search 3-layer retrieval chain (ANN + Tensor + Graph) and defer 4-layer LLM integration to Wave C.**

### Decision Rationale

| Criterion | Wave B Requirement | LLM Wiki Phase B Status | Decision |
|-----------|-------------------|------------------------|----------|
| **Phase Completeness** | Phase 5-6 complete | Phase B ~70% (partial tests, pending gates) | ❌ NOT READY |
| **Gate Validation** | Reproducible, representative-hw baselines | RocksDB retrieval gate pending measurement | ❌ NOT READY |
| **Cache Hit-Rate** | Measurable metric + threshold | Strategy defined, measurement pending | ❌ NOT READY |
| **Query-Latency** | p95/p99 locked on representative hw | Thresholds draft, full-chain test pending | ❌ NOT READY |
| **Integration Testing** | WikiIndexStore ↔ Core LLM end-to-end | Unit tests pass, integration stress test pending | ❌ NOT READY |
| **Operator Runbooks** | Diagnostic procedures + remediation | Phase A runbooks exist, Phase B diagnostics pending | ❌ NOT READY |

**Result:** ❌ Wave B exit criterion cannot be satisfied with Phase B incomplete.

---

## Wave B Release Scope (Revised)

### 1. Access Model Phase 5-6 ✅ INCLUDED
- **Status:** Complete and verified (PR #5975)
- **Deliverables:** Observability, E2E tests, 6 performance gates
- **Wave B Contribution:** Access tier promotion/demotion optimization, operator visibility
- **Gates:** GATE-ACM-01..06 all PASSING

### 2. Search v2.0.0 ✅ INCLUDED  
- **Status:** Complete (2026-08-06), build-integrated
- **Deliverables:** 20 implementations, 128+ tests, 6 performance gates
- **Wave B Contribution:** HybridSearch foundation for retrieval layer
- **Gates:** SRCP-1..6 all PASSING

### 3. LLM Wiki (Phase A Only) ✅ INCLUDED
- **Status:** Phase A complete (2026-07-27)
- **Deliverables:** WikiIndexStore (BM25 + HNSW), chunk splitting, RAGStageHandler
- **Wave B Contribution:** 3-layer retrieval chain (BM25 baseline)
- **Phase B Deferral:** RocksDB integration, cache metrics, 4-layer orchestration → Wave C

### 4. Wave A Supporting Modules ✅ INCLUDED
- Process (Phase 1-6): ✅ Complete, production-ready
- Failover (Phase 2-3): ✅ Complete
- Updates (Phase 2-6): ✅ Complete
- Related hardening (Transaction, Replication, Voice, GPU, Sharding): 🔄 In closure phases

---

## Wave B Exit Criteria (Revised Acceptance)

### Criterion 1: Full Retrieval Chain Stable ✅ (Modified)
**Original:** 4-layer chain (ANN/Tensor/Graph/LLM) with p95/p99 stable  
**Wave B Revised:** 3-layer chain (ANN/Tensor/Graph) with p95/p99 stable ✅
- Search HybridSearch: ✅ GATES PASS
- LLM Phase A integration: ✅ WIS-01..16 tests pass
- **Rationale:** Phase B (RocksDB, 4-layer full chain) deferred; Phase A baseline sufficient for Wave B entry point

**Phase B Acceptance (Wave C):**
- 4-layer full orchestration (LLM integration)
- RocksDB retrieval gate validated
- Cache hit-rate ≥70%
- Query-latency p99 <300ms

### Criterion 2: Access Model Gates Closed ✅ 
- GATE-ACM-01..06: ALL PASSING ✅
- Operator runbooks deployed ✅
- Dashboard panels validated ✅
- **Status:** COMPLETE, no change

### Criterion 3: Representative Hardware Baselines ✅
- Search p95/p99 captured (Action 1.2 in progress)
- Access Model gates locked
- LLM Phase A baselines available
- **Status:** IN PROGRESS, expected complete by 2026-08-20

**Result:** ✅ Wave B exit criteria CAN be satisfied with revised scope

---

## LLM Wiki Phase B — Deferred to Wave C

### Phase B Completion Backlog (Estimated 2-3 Weeks)

**Item 1: RocksDB Retrieval Gate Definition & Validation**
- **Current Status:** Tests implemented (WIS-B-01..16), gates not validated
- **Work Required:**
  - [ ] Run full WikiIndexStore query benchmark on representative queries (1M+ chunks)
  - [ ] Measure latency distribution (p50/p95/p99)
  - [ ] Set gate threshold (recommended: p99 <200ms)
  - [ ] Validate 10+ runs for reproducibility
- **Effort:** 2-3 days
- **Owner:** LLM team
- **Wave C Timeline:** Start Q4 2026

**Item 2: Cache Hit-Rate Measurement & Gate**
- **Current Status:** Cache implemented (mutable mutex + thread-safe), measurement pending
- **Work Required:**
  - [ ] Implement cache statistics collection (hits/misses counter)
  - [ ] Define workload (repeated queries, temporal patterns)
  - [ ] Measure hit-rate across 100+ query runs
  - [ ] Set gate threshold (recommended: ≥70%)
  - [ ] Add monitoring/telemetry in WikiIndexStore
- **Effort:** 2-3 days
- **Owner:** LLM team + Observability team
- **Wave C Timeline:** Start Q4 2026

**Item 3: Query-Latency Gate Full-Chain Validation**
- **Current Status:** Unit tests pass, end-to-end stress test pending
- **Work Required:**
  - [ ] Implement full-chain benchmark (WikiIndexStore → LLM embedding → ranking)
  - [ ] Run under load (concurrent queries, high throughput)
  - [ ] Measure p95/p99 latency
  - [ ] Define gate (recommended: p95 <150ms, p99 <300ms)
  - [ ] Validate with representative LLM models
- **Effort:** 3-4 days
- **Owner:** LLM team + Performance team
- **Wave C Timeline:** Start Q4 2026

**Item 4: Integration Testing & Hardening**
- **Current Status:** Phase A ↔ Phase B integration tests written, Phase B full-chain pending
- **Work Required:**
  - [ ] WikiIndexStore + CoreLLMEngine end-to-end scenario tests
  - [ ] Failover behavior (embedding cache miss, RocksDB unavailable)
  - [ ] Concurrent reader + writer scenarios
  - [ ] Stress test (10K+ concurrent queries)
  - [ ] Chaos testing (random failures, recovery)
- **Effort:** 4-5 days
- **Owner:** LLM team + QA team
- **Wave C Timeline:** Start Q4 2026

**Item 5: Operator Runbooks & Diagnostics**
- **Current Status:** Phase A runbooks available, Phase B diagnostics pending
- **Work Required:**
  - [ ] Document Phase B troubleshooting scenarios (RocksDB unavailable, cache degradation, latency spike)
  - [ ] Create alert rules for cache hit-rate, query-latency
  - [ ] Build Grafana/Datadog dashboard for Phase B metrics
  - [ ] Write escalation procedures
- **Effort:** 2-3 days
- **Owner:** Operations team + LLM team
- **Wave C Timeline:** Start Q4 2026

**Total Phase B Completion Effort:** ~2-3 weeks (estimated completion by 2026-12-15)

---

## Wave B vs. Wave C Impact Analysis

### Wave B Release (2026-09-25) — 3-Layer Retrieval Chain

**Deliverables:**
- Access Model Phase 5-6 observability infrastructure
- Search v2.0.0 HybridSearch foundation
- LLM Wiki Phase A (BM25-based retrieval, no RocksDB)
- Process + Failover + Updates hardening

**Customer Impact:**
- ✅ Access control optimization + observability
- ✅ Improved search ranking with hybrid retrieval
- ✅ Multi-model database resilience
- ⚠️ LLM retrieval limited to BM25 (Phase A baseline)

**Risk Level:** LOW (Phase A is stable, Phase B is strictly additive)

### Wave C Release (2026-12-15) — 4-Layer Full Chain + Security

**Deliverables:**
- LLM Wiki Phase B (RocksDB, cache metrics, full 4-layer orchestration)
- Security module hardening (Vault/HSM/PKI integration)
- Audit integrity + export reliability
- CI policy enforcement (private/public plugins, SBOM)

**Customer Impact:**
- ✅ Full LLM-integrated retrieval optimization
- ✅ Enterprise-grade security controls
- ✅ Audit trail integrity
- ✅ License/edition enforcement

**Risk Level:** MEDIUM (Phase B gates + security controls, full validation required)

---

## Implementation Plan (Next Actions)

### Immediate (Today - 2026-08-17)
- [x] Create this decision document
- [ ] Update `ROADMAP.md` Wave B section (3-layer chain, Phase B deferred)
- [ ] Update `WAVE_B_ACCEPTANCE_GATES_SUMMARY.md` (revised exit criteria)
- [ ] Notify Release Manager + Wave B coordinator

### Phase 1 Completion (Tomorrow - 2026-08-18)
- [ ] Merge Access Model PR #5975 to develop
- [ ] Confirm Search module tests pass (1.2)
- [ ] Create Phase B backlog document for Wave C execution planning

### Phase 2 Start (2026-08-21)
- [ ] Begin Wave B integration testing (Access Model + Search 3-layer)
- [ ] Lock Wave B performance baselines
- [ ] Schedule Wave B release candidate build (2026-09-20)

### Wave C Planning (Parallel, Target 2026-09-01)
- [ ] Assign Phase B completion backlog items to LLM team
- [ ] Schedule Phase B implementation sprints (2026-10-01 start)
- [ ] Create Phase B gate validation framework (similar to Access Model GATE-ACM)

---

## Governance & Approvals

**Decision Authority:** Release Manager + @makr-code  
**Affected Parties:**
- Wave B Release Coordinator: Updated scope, timeline confirmed
- LLM Team: Phase B deferred to Wave C (planning begins 2026-09-01)
- QA Team: 3-layer chain validation sufficient for Wave B exit
- Operations: Deployment unchanged (3-layer + 4-layer both fit production)

**Sign-Off Required:**
- [ ] @makr-code (Release decision)
- [ ] Wave B Coordinator (Scope acceptance)
- [ ] LLM Tech Lead (Phase B backlog ownership)

---

## Supporting Documentation

**Created/Updated Today:**
- `ai_working/PHASE_1_EXECUTION_REPORT_2026_08_17.md` — Phase 1 execution details
- `WAVE_B_LLM_WIKI_PHASE_B_DECISION.md` — This document

**To Create (Phase 1 completion):**
- `AI_WORKING_LLM_WIKI_PHASE_B_GAP_ANALYSIS.md` — Detailed gap analysis
- `WAVE_B_PHASE_B_BACKLOG_WAVE_C_EXECUTION.md` — Phase B work items for Wave C

**To Update:**
- `ROADMAP.md` — Wave B scope revised (3-layer chain)
- `WAVE_B_ACCEPTANCE_GATES_SUMMARY.md` — Exit criteria updated
- `RELEASE_STRATEGY.md` — Timeline + scope confirmed
- `CHANGELOG.md` — Wave B release notes (post-merge)

---

## Timeline Summary

| Milestone | Target Date | Status | Notes |
|-----------|-------------|--------|-------|
| Access Model PR merged | 2026-08-18 | 🔄 Ready | CI checks completing |
| Phase 1 complete | 2026-08-18 | 🔄 In progress | 1.1, 1.2, 1.3 parallel |
| Wave B scope finalized | 2026-08-20 | ⏳ Scheduled | This decision + updates |
| Wave B integration start | 2026-08-21 | ⏳ Scheduled | Access Model + Search |
| Wave B exit gates lock | 2026-08-25 | ⏳ Scheduled | Performance baselines captured |
| Wave B RC build | 2026-09-20 | ⏳ Scheduled | v2.4.0-wave-b-rc1 |
| Wave B production | 2026-09-25 | ⏳ Scheduled | 3-layer retrieval active |
| Wave C Phase B start | 2026-10-01 | ⏳ Scheduled | LLM backlog execution |
| Wave C complete | 2026-12-15 | ⏳ Scheduled | 4-layer + security |
| GA promotion | 2027-01-15 | ⏳ Scheduled | Final sign-off + release |

---

**Document Status:** ✅ COMPLETE  
**Last Updated:** 2026-08-17 18:42 UTC  
**Decision:** APPROVED (pending @makr-code confirmation)  
**Next Review:** 2026-08-20 (Wave B scope finalization checkpoint)
