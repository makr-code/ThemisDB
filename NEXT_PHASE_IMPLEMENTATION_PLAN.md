# ThemisDB Next Phase Implementation Plan

**Document:** Implementation roadmap and work-stream coordination  
**Status:** 🟡 ACTIVE — GATE-DRIVEN REBASELINE
**Last Updated:** 2026-07-28
**Target Completion:** Re-baselined to roadmap-driven GA closure gates (Phase 2/3/5/6 + Batch D)

---

## Source-of-Truth Rebaseline (2026-07-28)

- `ROADMAP.md` is authoritative for release readiness status and checkbox closure.
- This plan is now an execution companion only; if any statement conflicts with `ROADMAP.md`, treat the roadmap as canonical.
- Promotion remains blocked until:
  - open Phase 2/3/5 tasks in `ROADMAP.md` are closed with fresh evidence,
  - open Phase 6 governance/documentation checkboxes are closed,
  - Batch D promotion checklist is complete,
  - human sign-off in `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9 is completed.

### Current Open Scope (rebased to roadmap gates)

1. Phase 2: Graph/query optimisation closure under measurable latency/cache/pool/Wave-7 non-regression gates.
2. Phase 3: Release-critical + Wave 5/6 retention + Wave 8/chaos evidence re-confirmed on current `develop`.
3. Phase 5: Operational readiness closure (observability, backup/recovery proof, SLA evidence, runbook/manual checklist closure).
4. Phase 6: Root governance/documentation synchronization closure across canonical root files.
5. Batch D: D-1..D-10 evidence re-verified on current `develop`, then D-11 human approval.

---

## Phase 1-6 Subagent Execution Contract (2026-07-28)

### Branch and Promotion Guard

- Default implementation target branch is `develop` (hotfix exception path per `BRANCHING_STRATEGY.md` §6.4 remains valid).
- Promotion to `community` is blocked until the complete gate chain is PASS and human sign-off in `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9 is complete.

### Central Control Thread (mandatory before Phase 1)

- Maintain one central execution board with:
  - backlog items and owner subagent,
  - live gate status (`build`, `test`, `benchmark`, `security`, `docs`),
  - evidence links per completed gate,
  - current risk register and mitigation owner.
- Work proceeds strictly in sequence across Phase 1 → Phase 6.
- Parallel work is allowed only **inside** the active phase.
- Every phase closure must include:
  - Definition of Done (DoD),
  - explicit gate evidence,
  - risk update,
  - documentation/governance sync check.

### Subagent Role Matrix

| Subagent | Primary Responsibility | Required Output |
|---|---|---|
| `themisdb-implementer` | module implementation/fixes and test updates | merged change set + test/bench evidence |
| `themisdb-reviewer` | findings-first regression/security/correctness review | review findings + release recommendation |
| `gap-verifier` | validate scanner gaps and prioritize true positives | validated gap list with severity/order |
| `themisdb-doc-orchestrator` | roadmap/governance/doc synchronization (L1-L4 + SOT) | synchronized docs + drift report |
| `doc-orchestrator` (optional) | cross-module documentation cadence control | cadence report + unresolved doc blockers |

### Phase Execution Blocks (entry, parallelization, exit)

#### Phase 1 — Top-Risk Module Hardening (`server`, `llm`, `sharding`)
- Entry:
  - validated open critical gaps available from `gap-verifier`.
- Parallelization:
  - `themisdb-implementer` runs module workstreams in parallel (`server`/`llm`/`sharding`),
  - `themisdb-reviewer` performs rolling findings-first review.
- Exit:
  - no new CRITICAL findings,
  - no undocumented failure mode on release-critical paths,
  - evidence linked on control board.

#### Phase 2 — Performance/Scalability Readiness
- Entry:
  - Phase 1 exit criteria fully PASS.
- Parallelization:
  - optimization batches can run per subsystem (cache/pool/cost-model/load balancing),
  - review lane stays independent and mandatory.
- Exit:
  - Wave-7 non-regression evidence PASS,
  - repeatable load results and rollback path documented.

#### Phase 3 — Integration & Resilience Proof
- Entry:
  - Phase 2 performance and rollback evidence complete.
- Parallelization:
  - `release_critical` stability, Wave5/6 retention, and Wave8/chaos lanes may run concurrently.
- Exit:
  - continuous green `release_critical` chain,
  - resilience/recovery evidence complete and linked.

#### Phase 4 — Security & Compliance Hardening
- Entry:
  - integration/resilience evidence from Phase 3 complete.
- Parallelization:
  - `gap-verifier` security validation and `themisdb-implementer` remediation batches run in lockstep.
- Exit:
  - no new CRITICAL security findings,
  - sanitizer/pentest/residual-risk status consistent and documented.

#### Phase 5 — Operational Production Readiness
- Entry:
  - Phase 4 security/compliance closure complete.
- Parallelization:
  - observability/auditability and backup/recovery/SLA proof streams may run in parallel.
- Exit:
  - 99.99% SLA evidence complete,
  - runbook-backed operational readiness approved by reviewer lane.

#### Phase 6 — Documentation, Governance, Release Approval
- Entry:
  - Phases 1-5 technical gates all PASS.
- Parallelization:
  - `themisdb-doc-orchestrator` drives canonical file synchronization,
  - optional `doc-orchestrator` verifies cadence and unresolved drifts.
- Exit:
  - `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, `CHANGELOG.md`, and branch governance docs are mutually consistent,
  - human GA promotion sign-off complete.

### Global Transition Rule (hard gate)

- No phase transition is allowed without full gate package PASS:
  - build,
  - tests,
  - benchmarks,
  - security checks,
  - documentation/governance checks.

### Delivery Mode

- Execution remains batch-oriented: larger, complete remediation blocks are preferred over micro-fix iterations.

---

## Executive Summary

The project transitions from **foundational completion** (Graph Phase 2.4 ✅, Distributed Tensor Phase 4 ✅) to **optimization and hardening** across sequenced phases with in-phase parallelization:

1. **Graph Module Phase 3** — Query optimization, cache efficiency, resource pooling (6 weeks)
2. **Cross-Module Hardening** — Critical path fixes: server wire-protocol, LLM exception safety, sharding consistency (Phases 5-7)
3. **AQL 2.0.0 Language Expansion** — DDL, Geospatial, FTS parser integration (18-23 weeks, sequenced by gate readiness)

**Immediate Focus:** Graph Phase 3 + Wave 7/8 benchmark verification + LLM/Server Phase 5 hardening (first 4 weeks)

**Execution Automation:** use `/home/runner/work/ThemisDB/ThemisDB/scripts/next_phase_kickoff.py` to generate timestamped kickoff baseline/go-no-go reports in `/home/runner/work/ThemisDB/ThemisDB/ai_working`.

---

## Scope Freeze & Baseline Confirmation (2026-07-20)

### Fixed Done Baseline (not re-opened in this execution cycle)

- [x] Graph Phase 2.4 completed and accepted
- [x] Wave 5 completed
- [x] Wave 6 completed
- [x] Wave 7 hard-gate signoff completed
- [x] Block B completed (P3-03/P3-04)
- [x] Block C completed (P5-S01/P5-S02)
- [x] Block A completed (P3-01/P3-02, 2026-07-20)

### Remaining Open Scope (binding order)

1. **Block A** — P3-01 + P3-02 (Graph optimizer + cache)
2. **Block D** — P5-L01 + P5-L02 (LLM hardening)
3. **Block E** — P6-01 + P6-02 + P6-03 (Sharding consistency + fault injection)
4. **Parallel lane** — AQL 2.0 Phase 2 (DDL first; Geospatial/FTS after DDL gate)
5. **Evidence stream** — Wave 8 mandatory for distributed robustness signoff

### Acceptance Source Mapping (open scope)

- Block A criteria: Phase-3 gates in this document + root roadmap phase targets
- Block D criteria: Phase-5 LLM hardening gates in this document + module hardening targets
- Block E criteria: Phase-6 consistency/failover criteria + Wave-8 fault-injection evidence
- AQL lane criteria: DDL-first non-breaking sequencing with tracked compatibility notes
- Cross-cutting execution model: six-phase delivery contract from `FUTURE_ENHANCEMENTS.md` (Design → Core → Error/Edge → Tests → Hardening → Documentation/Acceptance)

---

## Phase Hierarchy & Timeline

### 🎯 **Phase 3: Graph Module Optimization & Hardening** (PRIMARY)

**Timeline:** 2026-07-08 to 2026-08-19 (6 weeks)  
**Status:** 🟡 Active (Block A complete; Block B complete; Block D complete)  
**Scope:** Query optimization, cache, resource pooling, load balancing, distributed consistency  
**Target Release:** v1.9.0-beta (Q4 2026)

#### Deliverables

| ID | Component | Effort | Owner | Gate | Status |
|----|-----------|--------|-------|------|--------|
| P3-01 | Query optimizer: plan cache + cost model hardening | 2 weeks | Team A | 46 tests | ✅ Complete (Block A, 2026-07-20) |
| P3-02 | Cache efficiency: LRU + multi-tier eviction | 1.5 weeks | Team A | 32 tests | ✅ Complete (Block A, 2026-07-20) |
| P3-03 | Resource pooling: connection + thread + buffer managers | 2 weeks | Team B | 28 tests | ✅ Complete (Block B) |
| P3-04 | Load balancing: query distribution + scheduling | 1.5 weeks | Team B | 24 tests | ✅ Complete (Block B) |
| P3-05 | Integration + performance benchmarks + sign-off | 1 week | Team A+B | Wave 7 gates | 🔵 Planned |

**Success Criteria:**
- ✅ All 130 new tests passing (cumulative with existing 326 graph tests = 456 total)
- ✅ Query latency p99 ≤ 50ms (vs. current baseline)
- ✅ Cache hit ratio ≥ 85% on synthetic workloads
- ✅ Zero new CRITICAL findings (code review + scanner)

**Kickoff Checklist:**
- [ ] Architecture review: Query optimizer plan
- [ ] Resource allocation: Assign Team A (2 engineers) + Team B (2 engineers)
- [ ] Workspace setup: `feature/graph-phase3-optimization` branch
- [ ] Test infrastructure: Add P3-specific benchmarks to Wave 7 suite

---

### 🎯 **Phase 5: Server & LLM Hardening** (SEQUENCED PHASE; MODULE LANES MAY RUN IN PARALLEL)

**Timeline:** 2026-07-22 to 2026-08-09 (3 weeks)  
**Status:** ✅ Implemented (Block D complete; transition to GA sign-off evidence)  
**Scope:** Wire-protocol retry logic, LLM exception safety, memory leak fixes  
**Target Release:** v1.9.0-beta patch

#### Deliverables

| ID | Component | Effort | Owner | Tests | Status |
|----|-----------|--------|-------|-------|--------|
| P5-S01 | Server: wire-protocol retry (2-3 retries + exponential backoff) | 1 week | Team C | 16 tests | ✅ Delivered (Block C) |
| P5-S02 | Server: HTTP timeout patterns + graceful shutdown | 1 week | Team C | 12 tests | ✅ Delivered (Block C) |
| P5-L01 | LLM: exception safety audit + RAII wrapper refactoring | 2 weeks | Team D | 28 tests | ✅ Delivered (Block D) |
| P5-L02 | LLM: memory leak fixes (model loading, cache cleanup) | 1.5 weeks | Team D | 24 tests | ✅ Delivered (Block D) |

**Success Criteria:**
- ✅ Server: 99.9% recovery from transient faults (verified with fault injection)
- ✅ LLM: Zero memory leaks on model load/unload cycles (valgrind clean)
- ✅ LLM: Exception safety score ≥ 4.5/5 (vs. current 3.5)

---

### 🎯 **Phase 6: Sharding Consistency & Fault Injection** (SEQUENCED PHASE)

**Timeline:** 2026-07-22 (delivered ahead of schedule)  
**Status:** ✅ Complete  
**Scope:** Transaction consistency, failover guarantees, distributed fault injection tests  
**Target Release:** v2.0.0-rc1 (Oct 2026)

#### Deliverables

| ID | Component | Effort | Owner | Tests | Status |
|----|-----------|--------|-------|-------|--------|
| P6-01 | Sharding: 2PC/3PC consistency verification + edge case testing | 2 weeks | Team E | 32 tests | ✅ Delivered (TXC-01..TXC-32, 2026-07-22) |
| P6-02 | Sharding: failover logic + recovery path hardening | 1.5 weeks | Team E | 20 tests | ✅ Delivered (FLR-01..FLR-20, 2026-07-22) |
| P6-03 | Wave 8: distributed fault injection suite (network partition, coordinator failure, cascade) | 2 weeks | Team F | 40 scenarios | ✅ Delivered (FI-01..FI-40, 2026-07-22) |

---

### 🎯 **AQL 2.0.0 Language Expansion** (SEPARATE LANE; ADMITTED ONLY AFTER ACTIVE PHASE GATES PASS)

**Timeline:** 2026-07-15 onwards  
**Status:** 🟡 Phase 2 Complete, Phase 3 (Geospatial) Queued

| Phase | Component | Duration | Owner | Status |
|-------|-----------|----------|-------|--------|
| Phase 1 | Parser + AST + Tokenizer + Safety | 5 weeks | ✅ Complete | ✅ DONE 2026-07-15 |
| Phase 2 | DDL (CREATE/DROP/ALTER COLLECTION/INDEX/VIEW) | 4-6 weeks | Team G | ✅ DONE 2026-07-22 |
| Phase 3 | Geospatial (ST_* parser integration) | 2-3 weeks | Team H | 🔵 Queued (after Phase 2) |
| Phase 4 | FTS enhancement | 2-3 weeks | Team H | 🔵 Queued (parallel with Phase 3) |
| Phase 5 | Integration & testing | 3-4 weeks | Team G+H | 🔵 Queued |

**Phase 2 Delivery (DDL):**
- [x] `SchemaDDLType`, `FieldDef`, `IndexDef`, `SchemaDDL` AST types added to `include/query/aql_parser.h`
- [x] `AQLParser::parseSchemaDDL()` implemented in `src/query/aql_parser.cpp`
- [x] `DDLExecutor` + `SchemaRegistry` in `include/query/ddl_executor.h` + `src/query/ddl_executor.cpp`
- [x] 32 GTest cases (DDL-01..DDL-32) in `tests/query/test_aql_ddl_phase2.cpp`, registered in CMakeLists

---

## Critical Gap Remediation Strategy

### Top 3 Modules (60% of CRITICAL+HIGH gaps)

**Focus Order:** LLM → Server → Sharding

| Module | CRITICAL | HIGH | Total | Batches | Timeline |
|--------|----------|------|-------|---------|----------|
| **LLM** | 1,245 | 22,755 | 24,000 | Batch 1-4 (2 weeks each) | Aug-Sep |
| **Server** | 1,245 | 17,814 | 19,059 | Batch 1-4 (10 days each) | Jul-Aug |
| **Sharding** | 696 | 10,316 | 11,012 | Batch 1-3 (10 days each) | Aug-Sep |

**Batch Strategy:** 2-week remediation cycles per module with scanner validation between batches

**Target:** 50% CRITICAL gap reduction by 2026-09-30 (EOQ3)

---

## Wave 7/8 Benchmark Verification

### Wave 7 Release Critical Signoff (Current)

**Status:** ✅ COMPLETE (2026-07-16)  
**6 Hard Gates:**
- ✅ Gate 1: Read p99 ≤ 200µs
- ✅ Gate 2: Write ≥ 80k ops/s
- ✅ Gate 3: Range scan p99 ≤ 500µs
- ✅ Gate 4: Batch insert p99 ≤ 5ms
- ✅ Gate 5-6: Self-check counters (consistency verified)

**Evidence:**
- `benchmarks/wave7/bench_w7a_release_critical_signoff.cpp` — RCS-01..RCS-08 ✅
- `benchmarks/wave7/release_gate_manifest_w7.json` — All gates PASS
- `benchmarks/wave7/RUNBOOK_W7.md` — Re-run procedure documented

### Wave 8 Endurance & Fault Injection (Planned)

**Timeline:** 2026-08-15 to 2026-09-15  
**Scope:** Soak tests, degradation scenarios, cascade failures

| Scenario | Duration | Objective | Target | Owner |
|----------|----------|-----------|--------|-------|
| SOK-01..08 | 24h + 72h soak | Memory stability, GC pressure | No memory leaks | Team F |
| DFR-01..08 | Network partition + recovery | Failover + replication lag | < 5s recovery | Team E |
| GVO-01..08 | Guardrails & variance | p99 tail latency stability | p99 within ±10% | Team A |

**Baseline Comparison:** Wave 7 gates repeated; must stay within thresholds

---

## Resource Allocation & Team Structure

### Recommended Team Assignments

| Team | Members | Primary Phase | Secondary |
|------|---------|---------------|-----------|
| **Team A** | 2 eng | Phase 3: Query optimizer + cache | Wave 8: GVO scenarios |
| **Team B** | 2 eng | Phase 3: Resource pooling + load balancing | Phase 5-S02: HTTP patterns |
| **Team C** | 2 eng | Phase 5: Server hardening | Wave 7 re-runs |
| **Team D** | 2 eng | Phase 5: LLM exception safety | Gap remediation Batch 1 |
| **Team E** | 2 eng | Phase 6: Sharding consistency + failover | Wave 8: DFR scenarios |
| **Team F** | 2 eng | Wave 8: Fault injection suite + scenarios | Gap remediation Batch 2 |
| **Team G** | 2 eng | AQL Phase 2: DDL implementation | — |
| **Team H** | 1 eng | AQL Phase 3-4: Geospatial + FTS | — |

**Total:** 15 engineers (flexible, overlap-friendly)

---

## Immediate Next Actions (Week 1: 2026-07-22 to 2026-07-26)

### Priority 1: Kickoff Phase 3 & Phase 5

- [ ] **Monday 2026-07-22:** Architecture review meetings (Teams A, B, C, D present roadmaps)
- [ ] **Monday-Tuesday:** Create GitHub issues + milestones for Phase 3, Phase 5-S, Phase 5-L
- [ ] **Tuesday:** Feature branch creation: `feature/graph-phase3-optimization`, `feature/server-phase5-hardening`, `feature/llm-phase5-hardening`
- [ ] **Wednesday:** Update ROADMAP.md section headings to reflect "Phase 3 ACTIVE"
- [ ] **Wednesday-Thursday:** Setup test scaffolding (P3 test files, Phase 5 test fixtures)
- [ ] **Friday:** First commit cycle: empty test files + stubs + CMakeLists.txt changes (no logic yet)

### Priority 2: Establish Monitoring & Tracking

- [ ] Create GitHub Project for "Next Phase Implementation" with columns: TODO / In Progress / Review / Blocked / Done
- [ ] Create issue templates for Phase 3, Phase 5, Phase 6, Wave 8
- [ ] Set up daily standup tracking doc in ai_working/NEXT_PHASE_STATUS.md
- [ ] Establish weekly review gates (every Friday)

### Priority 3: Baseline Measurements

- [ ] Re-run Wave 7 benchmark to confirm baseline (all 6 gates must pass)
- [ ] Profile current query optimizer performance (p50, p95, p99)
- [ ] LLM memory usage baseline (Valgrind + massif)
- [ ] Server connection pool saturation test
- [ ] Scanner re-run on develop: current baseline for CRITICAL/HIGH gap counts

### Priority 4: AQL Phase 2 Kickoff

- [ ] Team G: Review `src/query/sql_parser.cpp` structure
- [ ] Create `src/query/aql_ddl_parser.cpp` skeleton + test fixture
- [ ] Design DDL AST node hierarchy (CREATE TABLE / CREATE INDEX / CREATE VIEW)
- [ ] Document breaking changes (if any) vs. v1.x

---

## Definition of Done (Per Phase)

### Code Completeness
- [x] All methods/functions have production logic (no TODO stubs)
- [x] RAII patterns applied (no raw pointers in public APIs)
- [x] Exception safety >= 3/5 (basic exception-safe; cleanup on failure)
- [x] Memory safety: No leaks (valgrind clean) or data races (TSan pass)

### Testing
- [x] Unit tests: >= 80% code coverage (line + branch)
- [x] Integration tests: >= 3 critical paths per component
- [x] Performance benchmarks: Baseline measured + worst-case documented
- [x] Fault injection: >= 1 failure scenario tested per module

### Documentation
- [x] API docs: All public functions have Doxygen comments
- [x] Architecture: Decision rationale for optimization choices (comments + ADR)
- [x] Troubleshooting: Known limitations + workarounds documented

### Review & Sign-Off
- [x] Code review: >= 1 peer review from different team (not author's team)
- [x] Scanner audit: Zero new CRITICAL findings (HIGH findings documented)
- [x] Security review: No new vulnerabilities introduced (CodeQL pass)
- [x] Phase gate: All acceptance criteria met (see per-phase deliverables)

---

## Success Metrics & Acceptance Gates

### Phase 3 Success (Graph Module)
- ✅ All 130 new tests PASS (must be deterministic, no flakes)
- ✅ Query latency p99 ≤ 50ms (vs. baseline: measure first)
- ✅ Cache hit ratio ≥ 85% on production-like workloads
- ✅ Zero regressions in existing 326 graph tests
- ✅ Scanner: Zero new CRITICAL findings

### Phase 5 Success (Server + LLM)
- ✅ Server: 99.9% fault recovery rate (fault injection verified)
- ✅ LLM: Valgrind clean (no memory leaks); exception safety >= 4/5
- ✅ 80 new tests PASS (Phase 5-S + Phase 5-L target set)
- ✅ Zero regressions in server/llm existing tests

### Wave 8 Success (Soak + Fault Injection)
- ✅ 72h soak: Memory stable (no OOM, no GC hangs)
- ✅ Failover: Recovery time < 5 seconds
- ✅ Guardrails: p99 variance within ±10% of Wave 7 baseline
- ✅ No cascading failures across multi-shard topology

### Gap Remediation Success
- ✅ 50% CRITICAL gap reduction (LLM: 1,245 → 620; Server: 1,245 → 620; Sharding: 696 → 350)
- ✅ 30% HIGH gap reduction across top 3 modules
- ✅ All Batch N remediation PRs merged and tested

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Phase 3 optimizer regression | Medium | High | Intensive perf regression testing; Wave 7 gates must stay pass |
| LLM memory leak not reproducible | Medium | High | Dedicated memory profiler; daily Valgrind runs on CI |
| Sharding consensus bug discovered | Low | Critical | 2PC/3PC formal verification test; chaos engineering |
| AQL Phase 2 parser complexity | Medium | Medium | Reference sql_parser.cpp; weekly design reviews |
| Resource contention (all teams active) | Low | Low | Staggered kickoffs (Phase 3 + Phase 5 week 1; Phase 6 week 4) |

---

## Evidence & Tracking

### Deliverables Repository
- **ai_working/NEXT_PHASE_PHASE3_OPTIMIZATION.md** — Phase 3 detailed plan + test cases
- **ai_working/NEXT_PHASE_PHASE5_HARDENING.md** — Phase 5 detailed plan + acceptance criteria
- **ai_working/NEXT_PHASE_STATUS.md** — Daily standup tracking (updated Friday each week)

### GitHub Milestones & Issues
- Milestone: `Phase 3: Graph Optimization (2026-07-22 → 2026-08-19)`
- Milestone: `Phase 5: Server+LLM Hardening (2026-07-22 → 2026-08-09)`
- Milestone: `Wave 8: Fault Injection Suite (2026-08-15 → 2026-09-15)`
- Issues: One per deliverable (P3-01..05, P5-S01..02, P5-L01..02, etc.)

### Verification Evidence
- Build logs: `cmake --build --preset community-release` (must succeed)
- Test results: `ctest --preset community-release -L phase3` (all PASS)
- Benchmark results: `benchmarks/wave7/report_variance_w7.py` (baseline comparison)
- Memory analysis: Valgrind logs archived in CI (zero leaks)
- Coverage reports: Doxygen audit + gcov coverage reports (>= 80% per module)

---

## Decision Tree: Which Phase to Start?

```
START: You have 2 weeks of capacity

Are Wave 7 gates confirmed PASS? ──YES──> Start Phase 3 + Phase 5 in parallel
                                          (Primary: Graph Phase 3 focus)
                                    NO──> BLOCKER: Re-run Wave 7 until gates pass
                                          (Cannot advance without baseline)

Is LLM exception-safety audit complete? ──YES──> Start Phase 5-L immediately
                                           NO──> Schedule LLM audit as Day 1 task
                                                 (3 days, max)

Is server wire-protocol retry merge-ready? ──YES──> Start Phase 5-S immediately
                                           NO──> Complete code review
                                                 (1-2 days, then merge)

Do you have >= 12 engineers? ──YES──> Full parallel execution (all phases)
                               NO──> Stagger: Phase 3 week 1-2, Phase 5 week 2-4
```

---

## Rollout & Promotion Path

### Internal Milestones (for v1.9.0-beta)
- 2026-07-22: Phase 3 + Phase 5 kickoff
- 2026-08-05: Phase 3 mid-gate (50% tests passing)
- 2026-08-09: Phase 5 sign-off (all hardening complete)
- 2026-08-19: Phase 3 complete + Wave 7 re-pass
- 2026-08-31: Wave 8 complete (soak + fault injection)

### Release Promotion
- v1.9.0-beta: 2026-09-15 (Phase 3 + Phase 5 complete, Wave 7/8 verified)
- v2.0.0-rc1: 2026-10-15 (Phase 6 + AQL Phase 2-3 complete)
- v2.0.0 GA: 2026-11-15 (Production hardening + SLA validation)

---

**Next Action:** Review this plan with team leads. Run the decision tree above. Begin Phase 3 architecture review on 2026-07-22.
