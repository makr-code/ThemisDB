# ThemisDB Master Execution Plan — Phases 1A–5 (2026 Q3–Q4)

**Document:** Consolidated roadmap for GA promotion and post-GA hardening  
**Created:** 2026-08-05  
**Status:** 🟢 **ACTIVE** — Ready for immediate execution  
**Target Completion:** 2026-10-31 (Phase 5 close)  
**Owner:** AI Delivery Agent + Release Governance Board

---

## Executive Summary

This master plan orchestrates the delivery of **ThemisDB v2.4.0 GA** promotion and five consecutive hardening phases across the remainder of Q3 2026 and Q4 2026. The execution model is:

1. **Phase 1A (2026-08-05 → 2026-08-11):** Sequential human governance gate for GA sign-off closure
2. **Phases 2–5 (2026-08-12 → 2026-10-31):** Parallel execution against `develop` with phase-entry gates
3. **Release Promotion:** After Phase 1A completion, staged promotion through `community`, `enterprise`, `hyperscaler`, `military` lanes per RELEASE_STRATEGY.md

---

## Timeline Overview

```
┌─ PHASE 1A (Sequential Gate) ─────────────────┐
│ GA Sign-Off Closure (1–2 weeks)              │
│ Target: 2026-08-11                          │
│ ✅ Human sign-off                            │
│ ✅ Wave 7/8/9 gates PASS                     │
│ ✅ Release v2.4.0 GA created                 │
└────────────────────────────────────────────┘
                        │
                        ↓
     ┌──────────────────┼──────────────────┐
     │                  │                  │
 [PHASE 2]         [PHASE 3]          [PHASE 4]        [PHASE 5]
 Perf &            Plugin            Observability     LLM Wiki
 Scalability       External.         & Operations      Hardening
 (6–8 weeks)       (8–10 weeks)       (6–8 weeks)      (8–10 weeks)
 Target:           Target:            Target:          Target:
 2026-09-30        2026-10-31         2026-09-30       2026-10-31
 ✅ Query opt      ✅ Submodule pins  ✅ Error tax     ✅ Phase B active
 ✅ Cost model     ✅ Manifest v2     ✅ Audit logs    ✅ Wikipedia
 ✅ Resource       ✅ Edition gates    ✅ SLO signals   ✅ RocksDB
   pooling         ✅ CI policy       ✅ Runbooks      ✅ RBAC
 130+ tests        40+ tests          32 tests         32 tests
```

---

## Phase Descriptions & Dependencies

### Phase 1A: GA Promotion Sign-Off Closure

**Duration:** 1–2 weeks  
**Owner:** Release Approver + Release Engineer  
**Status:** 🔴 **READY** — Awaiting human start signal  
**Detailed Plan:** `ai_working/PHASE_1A_GA_SIGN_OFF_DETAILED_PLAN.md`

**Deliverables:**
- ✅ Section 9 human sign-off in `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- ✅ All Batch D gates re-verified on current `develop` HEAD
- ✅ Wave 7, 8, 9 hard gate evidence confirmed PASS
- ✅ Release tag `v2.4.0` created on `community` branch
- ✅ CHANGELOG.md entry published for v2.4.0 GA
- ✅ research/implementation_influence/by_module.md Soll-Ist matrix verified

**Acceptance Criteria:**
- [ ] `develop` HEAD passes `release_critical` CTest suite (zero failures)
- [ ] All Wave 7 hard gates (GATE-W7-01..06) confirmed PASS
- [ ] All Wave 8 hard gates (GATE-W8-01..04) confirmed PASS
- [ ] All Wave 9 hard gates (GATE-W9-01..06) confirmed PASS
- [ ] Security Lead sign-off on sanitizer + pentest evidence bundles
- [ ] Module gap audit (server/llm/sharding): zero new CRITICAL findings
- [ ] CHANGELOG.md moved from `[Unreleased]` to `[2.4.0]`
- [ ] VERSIONING.md confirms v2.4.0 GA stable status
- [ ] ROADMAP.md Phase 0–6 completion markers documented
- [ ] Doxygen coverage audit: >99% header coverage confirmed
- [ ] `develop` → `community` merge approved + tag created

**Blocking Status:** ⏸️ Awaiting human release approver authorization

---

### Phase 2: Performance & Scalability Readiness

**Duration:** 6–8 weeks (after Phase 1A)  
**Owner:** Query Optimization Team (Team B)  
**Status:** 🟤 **PENDING** — Ready to start 2026-08-12  
**Detailed Plan:** `ai_working/PHASE_2_PERFORMANCE_SCALABILITY_DETAILED_PLAN.md`

**Deliverables:**
1. Plan-cache efficiency: ≥95% hit-rate, <100 MB footprint
2. Cost-model refinement: p99 latency <200 ms (complex multi-shard)
3. Resource pooling: ≥2× throughput improvement vs. baseline
4. Benchmark infrastructure: measurement harness + Wave 7 regression gates
5. 130+ new test cases covering all three optimization vectors

**Milestones:**
- **M1 (Weeks 1–2):** Baseline & measurement infrastructure
- **M2 (Weeks 3–4):** Plan-cache optimization
- **M3 (Weeks 4–5):** Cost-model refinement
- **M4 (Weeks 5–6):** Resource pooling tuning
- **M5 (Weeks 6–8):** Wave 7 regression validation + hardening

**Entry Gate:**
- [ ] Phase 1A complete + release tag v2.4.0 created
- [ ] Baseline performance captured (benchmarks/wave7/release_gate_manifest_w7.json)

**Exit Gate:**
- [ ] All 130+ tests PASS with no Wave 7 regressions
- [ ] Throughput improvement ≥2× vs. baseline verified
- [ ] Plan-cache metrics archived

---

### Phase 3: Plugin Externalization & Monetization

**Duration:** 8–10 weeks (parallel with Phase 2, after Phase 1A)  
**Owner:** Infrastructure Team (Team C)  
**Status:** 🟤 **PENDING** — Ready to start 2026-08-12  
**Detailed Plan:** `ai_working/PHASE_3_PLUGIN_EXTERNALIZATION_DETAILED_PLAN.md`

**Deliverables:**
1. Wave-1 private plugins as commit-pinned submodules:
   - `plugins/private/themisdb_ethic_ai`
   - `plugins/private/themisdb_storage`
   - `plugins/private/themisdb_importer`
   - `plugins/private/themisdb_llm_wiki`
2. Manifest schema v2 with visibility, edition-allowance, license-feature metadata
3. Edition gating: Community/Minimal fail-closed for private sources
4. CI policy checks: prevent private credential leakage
5. 40+ new tests covering private/public boundaries

**Milestones:**
- **M1 (Weeks 1–2):** Submodule pinning & commit freeze
- **M2 (Weeks 2–3):** Manifest schema v2 implementation
- **M3 (Weeks 3–4):** Edition gating logic
- **M4 (Weeks 4–5):** CI policy integration
- **M5 (Weeks 5–7):** Public externalizations (geo, timeseries)
- **M6 (Weeks 7–10):** Testing & validation

**Entry Gate:**
- [ ] Phase 1A complete + release tag v2.4.0 created
- [ ] All Wave-1 private repos provisioned and initialized

**Exit Gate:**
- [ ] All 40+ tests PASS (private/public boundary validation)
- [ ] Community-only builds verified for zero private credential leakage
- [ ] All submodule commit hashes pinned and locked

---

### Phase 4: Observability & Operations Hardening

**Duration:** 6–8 weeks (parallel with Phase 2/3, after Phase 1A)  
**Owner:** Operations & SRE Team (Team D)  
**Status:** 🟤 **PENDING** — Ready to start 2026-08-12  
**Detailed Plan:** `ai_working/PHASE_4_OBSERVABILITY_OPERATIONS_DETAILED_PLAN.md`

**Deliverables:**
1. Error taxonomy expansion: 20+ new error codes for operational semantics
2. Audit logging framework: GDPR-/HIPAA-compliant audit trail
3. SLO signal extraction: latency, error rate, availability, resource metrics
4. Runbook suite: installation, failover, recovery, upgrade, troubleshooting
5. 99.99% SLA validation: Wave 9b test confirms 5-nines uptime under chaos
6. 32 new operational test cases

**Milestones:**
- **M1 (Weeks 1–2):** Error classification & taxonomy
- **M2 (Weeks 2–3):** Audit logging & compliance instrumentation
- **M3 (Weeks 3–4):** SLO signal extraction
- **M4 (Weeks 4–5):** Runbook suite creation
- **M5 (Weeks 5–8):** SLA validation & hardening

**Entry Gate:**
- [ ] Phase 1A complete + release tag v2.4.0 created

**Exit Gate:**
- [ ] All 32 tests PASS
- [ ] Wave 9b SLA compliance test confirms 99.99% uptime
- [ ] All runbooks archived in docs/

---

### Phase 5: LLM Wiki Enterprise Plugin Hardening

**Duration:** 8–10 weeks (parallel with Phase 2/3/4, after Phase 1A)  
**Owner:** LLM Wiki Plugin Development Team (Team E)  
**Status:** 🟤 **PENDING** — Ready to start 2026-08-12  
**Detailed Plan:** `ai_working/PHASE_5_LLM_WIKI_HARDENING_DETAILED_PLAN.md`

**Deliverables:**
1. Phase B index architecture: RocksDB BM25 + HNSW + RRF
2. Persistent embedding cache: RocksDB column family, ≥99% hit rate
3. C++ workspace orchestrator: lifecycle, concurrency, recovery
4. Wikipedia ingestion: ≥5k articles/s throughput validation
5. Phase B→A degradation mode: fallback when Phase B unavailable
6. Enterprise RBAC: multi-tenant namespaces
7. 32 new focused tests (LWP-09..LWP-40)

**Milestones:**
- **M1 (Weeks 1–2):** Phase B design & RocksDB schema
- **M2 (Weeks 2–3):** RocksDB BM25 + HNSW kernel implementation
- **M3 (Weeks 3–4):** Persistent cache layer
- **M4 (Weeks 4–5):** Workspace orchestrator hardening
- **M5 (Weeks 5–7):** Wikipedia ingestion integration
- **M6 (Weeks 7–10):** RBAC + testing & validation

**Entry Gate:**
- [ ] Phase 1A complete + release tag v2.4.0 created
- [ ] Phase A baseline (Python MVP + JSON index) production-stable

**Exit Gate:**
- [ ] All 32 tests PASS
- [ ] Wikipedia ingestion: ≥5k articles/s throughput verified
- [ ] Embedding cache: ≥99% hit rate confirmed
- [ ] RBAC multi-tenant namespaces operational

---

## Execution Sequencing & Dependencies

### Sequential Blocker (Phase 1A)

Phase 1A **must complete before any Phase 2–5 work is promoted** to release lanes. However, Phases 2–5 **can execute in parallel** against `develop` once Phase 1A gates pass and each phase's entry conditions are met.

```
MUST COMPLETE FIRST:          CAN EXECUTE IN PARALLEL:
┌──────────────────┐          ┌─────────────┬─────────────┬─────────────┬──────────────┐
│  PHASE 1A        │ ─────→   │ PHASE 2     │ PHASE 3     │ PHASE 4     │ PHASE 5      │
│ GA Sign-Off      │          │ Perf &      │ Plugin      │ Observ. &   │ LLM Wiki     │
│ (1–2 weeks)      │          │ Scalability │ External.   │ Operations  │ Hardening    │
│                  │          │ (6–8 w)     │ (8–10 w)    │ (6–8 w)     │ (8–10 w)     │
└──────────────────┘          └─────────────┴─────────────┴─────────────┴──────────────┘
```

### Phase Interdependencies

- **Phase 1A → Phase 2:** Query optimizer work requires stable GA baseline
- **Phase 1A → Phase 3:** Plugin externalization requires public/private boundary definitions
- **Phase 1A → Phase 4:** Observability hardening requires operational semantics established in GA
- **Phase 1A → Phase 5:** Wiki Phase B requires stable LLM embedding infrastructure (Phase A)

- **Phase 2 ↔ Phase 3:** Independent (no direct blocking)
- **Phase 2 ↔ Phase 4:** Independent (no direct blocking)
- **Phase 2 ↔ Phase 5:** Independent (no direct blocking)
- **Phase 3 ↔ Phase 4:** Independent (no direct blocking)
- **Phase 3 ↔ Phase 5:** Plugin externalization (Phase 3 M6) should complete before LLM Wiki release (Phase 5)
- **Phase 4 ↔ Phase 5:** Independent (no direct blocking)

---

## Resource & Team Allocation

| Phase | Team | Lead | Key Roles | Effort |
|-------|------|------|-----------|--------|
| 1A | Release Eng | Release Approver | Engineer, QA, Security Lead | 40–60 hours |
| 2 | Query Opt | Team B Lead | DB Optimizer, Benchmark Eng, QA | 240–320 hours |
| 3 | Infrastructure | Team C Lead | Release Eng, Build Maintainer, QA | 300–400 hours |
| 4 | Ops/SRE | Team D Lead | SRE, Ops Eng, Support Eng, QA | 200–280 hours |
| 5 | LLM Wiki | Team E Lead | C++ Dev, ML Eng, QA, Benchmark Eng | 280–360 hours |

**Total:** ~1,260–1,800 engineering hours across 5 months

---

## Release Lane Sequencing

After Phase 1A completion, promotion follows the edition lanes:

```
DEVELOP BRANCH                    RELEASE LANES
┌─ v2.4.0-rc1 (current)           ┌─ Minimal
│                          ─────→  ├─ Community ← (2026-08-11)
│ + Phase 2 work                   ├─ Enterprise
│ + Phase 3 work                   └─ Hyperscaler
│ + Phase 4 work                   + Military
│ + Phase 5 work
│
└─ v2.4.1-alpha (post-GA work)
  (Phases 2–5 staging)
```

**Promotion Path:**
1. **2026-08-11:** Phase 1A complete → v2.4.0 GA tag created on `community` (from develop)
2. **2026-09-30:** Phase 2 & 4 complete → v2.4.1-beta candidate branch
3. **2026-10-31:** Phases 3 & 5 complete → v2.4.1 GA candidate for all lanes

---

## Critical Path & Risk Register

### Critical Path

```
Phase 1A (8 days)
 └─→ Phase 3 Milestone 2 (11 days: M1 + M2)
  └─→ Phase 3 Milestone 4 (7 days: M3 + M4)
   └─→ Phase 3 Milestone 6 (21 days: M5 + M6)
    └─→ Phase 5 Completion (21 days from M1 start)
     └─→ TOTAL CRITICAL PATH: ~68 days (ends 2026-10-12)
```

### Top Risks & Mitigation

| Risk | Impact | Likelihood | Mitigation |
|------|--------|-------------|-----------|
| Phase 1A human sign-off delayed | Blocks all Phase 2–5 work | Medium | Schedule sign-off meeting 1 week in advance |
| Private submodule integration issues | Phase 3 blocker | Medium | Test all submodules locally before phase start |
| Performance regression in Phase 2 | Fails Wave 7 gates | Low | Continuous regression testing during development |
| CI policy edge cases in Phase 3 | Community builds fail | Medium | Comprehensive CI testing pre-Phase-3-end |
| Observability framework incompleteness (Phase 4) | Incomplete runbooks | Low | Parallel runbook creation with framework |
| LLM Wiki Phase B scale issues (Phase 5) | <5k articles/s throughput | Medium | Early Wikipedia dump validation in M2 |

---

## Success Criteria & Acceptance Gates

### Phase 1A Success Criteria
- ✅ Human sign-off obtained and archived
- ✅ All release_critical tests PASS (zero failures)
- ✅ Wave 7/8/9 gates all PASS
- ✅ v2.4.0 tag created and reachable
- ✅ Release build succeeds from tag

### Phases 2–5 Success Criteria (per phase)
- ✅ All phase-specific tests PASS (zero failures)
- ✅ No Wave 7 regressions (performance gates hold)
- ✅ Phase exit gate acceptance criteria met
- ✅ Documentation updated (Doxygen, API docs, runbooks)
- ✅ Code review approval obtained

---

## Key Artifacts & Evidence Trail

### Phase 1A
- `docs/governance/GA_PROMOTION_SIGN_OFF.md` (Section 9 sign-off)
- `benchmarks/wave{7,8,9}/FINAL_GATE_EVIDENCE_v2.4.0.json`
- `artifacts/MODULE_GAP_AUDIT_2026-08-XX.txt`
- `CHANGELOG.md` (v2.4.0 entry)
- `VERSIONING.md` (GA status)

### Phases 2–5
- Phase-specific test reports (CMake/CTest XML output)
- Benchmark reports (throughput, latency, resource utilization)
- Module documentation updates (Doxygen headers)
- Performance regression baseline comparisons
- Runbook suite (for Phase 4)
- Plugin manifest validation reports (for Phase 3)
- Wikipedia ingestion metrics (for Phase 5)

---

## Rollout Checkpoints

| Date | Checkpoint | Gate | Owner |
|------|-----------|------|-------|
| 2026-08-05 | Phase 1A kickoff (THIS DAY) | Ready | Release Eng |
| 2026-08-11 | Phase 1A complete + v2.4.0 tag | PASS | Release Approver |
| 2026-08-12 | Phases 2–5 parallel execution start | Ready | Team Leads (B, C, D, E) |
| 2026-09-06 | Phase 2 Milestone 3 checkpoint | Progress | Team B |
| 2026-09-13 | Phase 3 Milestone 2 checkpoint | Progress | Team C |
| 2026-09-20 | Phase 4 Milestone 3 checkpoint | Progress | Team D |
| 2026-09-27 | Phase 5 Milestone 3 checkpoint | Progress | Team E |
| 2026-09-30 | Phase 2 & 4 complete | PASS | Teams B & D |
| 2026-10-15 | Phase 3 mid-point review | Progress | Team C |
| 2026-10-31 | Phases 3 & 5 complete | PASS | Teams C & E |
| 2026-11-07 | v2.4.1-GA promotion candidate | GATE | Release Approver |

---

## Governance & Sign-Off

**Current Status:** 🟢 **READY FOR EXECUTION**

**Next Action:** Release Approver authorizes Phase 1A start

**Approval Chain:**
1. ⏳ Release Approver signs Phase 1A start authorization
2. ⏳ Security Lead reviews GA evidence bundles (Phase 1A)
3. ⏳ All phases' entry gates cleared by respective team leads

---

## References

- Orchestration Master: `ai_working/PHASE_1A_5_IMPLEMENTATION_ORCHESTRATION.md`
- Phase 1A Detailed: `ai_working/PHASE_1A_GA_SIGN_OFF_DETAILED_PLAN.md`
- Phase 2 Detailed: `ai_working/PHASE_2_PERFORMANCE_SCALABILITY_DETAILED_PLAN.md`
- Phase 3 Detailed: `ai_working/PHASE_3_PLUGIN_EXTERNALIZATION_DETAILED_PLAN.md`
- Phase 4 Detailed: `ai_working/PHASE_4_OBSERVABILITY_OPERATIONS_DETAILED_PLAN.md`
- Phase 5 Detailed: `ai_working/PHASE_5_LLM_WIKI_HARDENING_DETAILED_PLAN.md`
- Release Strategy: `RELEASE_STRATEGY.md`
- Branching Strategy: `BRANCHING_STRATEGY.md`
- GA Sign-Off: `docs/governance/GA_PROMOTION_SIGN_OFF.md`

---

**Last Updated:** 2026-08-05 08:30 UTC  
**Next Review:** After Phase 1A completion (2026-08-12)
