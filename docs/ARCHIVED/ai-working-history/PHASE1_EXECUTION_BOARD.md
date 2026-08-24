# Phase 1 — Top-Risk Module Hardening Execution Board

**Status:** 🟡 ACTIVE KICKOFF  
**Target Completion:** 2026-08-31  
**Hard Gate:** No new CRITICAL findings; all exit states documented and test-backed

---

## Executive Summary

Phase 1 focuses on hardening three critical modules identified as top-risk for GA promotion:

1. **Server** (P5-S01 / P5-S02): Wire-protocol retry, HTTP timeout, graceful shutdown
2. **LLM** (P5-L01 / P5-L02): Exception safety, RAII, leak prevention, race-condition elimination
3. **Sharding** (P6): ✅ **ALREADY COMPLETE** (2026-07-22)

Execution proceeds in parallel across Server and LLM workstreams, with rolling reviews. Sharding remains a reference implementation for hardening quality.

---

## Delivery Structure

### Batch 1.1 — Server Hardening (P5-S01 + P5-S02)

**Owner:** Server team / themisdb-implementer  
**Target Completion:** 2026-08-15

#### P5-S01 — Wire-Protocol Retry (Exponential Backoff)
- **Scope:** Retry semantics with exponential backoff for transient wire-protocol errors
- **Deliverables:**
  - Tests: 19 focused tests (`test_server_phase5_hardening.cpp` WSR-01..19)
  - Implementation: Retry coordinator in `src/server/wire_protocol_connection_pool.cpp`
  - Documentation: Retry budget semantics, backoff curves, failure classification
- **Acceptance Criteria:**
  - All 19 tests PASS with deterministic results (seed 42)
  - No Wave-7 regression in `benchmarks/wave7/`
  - release_critical pipeline PASS
  - Doxygen comments on public APIs complete

#### P5-S02 — HTTP Timeout + Graceful Shutdown
- **Scope:** Timeout semantics for HTTP handlers and orderly shutdown under load
- **Deliverables:**
  - Tests: 20 focused tests (`test_server_phase5_hardening.cpp` HST-01..20)
  - Implementation: Timeout coordinator in `src/server/http_server.cpp`
  - Documentation: Shutdown ordering, timeout budgets, failure recovery
- **Acceptance Criteria:**
  - All 20 tests PASS with deterministic results (seed 42)
  - No Wave-7 regression
  - release_critical pipeline PASS
  - Doxygen comments complete

#### P5-S01 + P5-S02 Integration
- **Combined Test Suite:** `tests/server/test_server_phase5_hardening.cpp`
- **CTest Target:** `module_server_test_server_phase5_hardening_focused`
- **CTest Labels:** `server`, `hardening`, `phase5`, `release_critical`
- **Timeout:** 120 seconds
- **Dependencies:** themis_core, spdlog, gtest

### Batch 1.2 — LLM Hardening (P5-L01 + P5-L02)

**Owner:** LLM team / themisdb-implementer  
**Target Completion:** 2026-08-20

#### P5-L01 — Model Lifecycle (Load/Unload Safety)
- **Scope:** Exception safety on model loading, unloading, plugin lifetime, and double-unload idempotency
- **Deliverables:**
  - Tests: 25 focused tests (`test_llm_phase5_hardening.cpp` EXS-01..25)
  - Implementation: Model loader hardening in `src/llm/model_loader.cpp`
  - Documentation: Model lifecycle contracts, RAII guarantees, failure modes
- **Acceptance Criteria:**
  - All 25 tests PASS with deterministic results (seed 42)
  - Sanitizer verification (ASan/TSan/UBSan): zero new leaks/races
  - No Wave-7 regression
  - Doxygen comments complete

#### P5-L02 — Concurrency + Backpressure
- **Scope:** Batch scheduling safety, quota management, concurrent access, and shutdown coordination
- **Deliverables:**
  - Tests: 28 focused tests (`test_llm_phase5_hardening.cpp` MEM-01..28)
  - Implementation: Scheduler hardening in `src/llm/continuous_batch_scheduler.cpp`
  - Documentation: Concurrency semantics, backpressure contracts, shutdown coordination
- **Acceptance Criteria:**
  - All 28 tests PASS with deterministic results (seed 42)
  - Sanitizer verification: zero new leaks/races
  - No Wave-7 regression
  - Doxygen comments complete

#### P5-L01 + P5-L02 Integration
- **Combined Test Suite:** `tests/llm/test_llm_phase5_hardening.cpp`
- **CTest Target:** `module_llm_test_llm_phase5_hardening_focused`
- **CTest Labels:** `llm`, `hardening`, `phase5`, `release_critical`
- **Timeout:** 120 seconds
- **Dependencies:** themis_core, llm module, gtest, sanitizer libs

### Batch 1.3 — Sharding Phase 6 (Reference)

**Status:** ✅ **ALREADY COMPLETE** (2026-07-22)  
**Evidence:** `docs/sharding/SHARDING_P6_SIGN_OFF.md`, `tests/sharding/test_sharding_phase6_hardening.cpp`  
**Quality Model:** Use as reference for P5-S01/P5-S02 and P5-L01/P5-L02 hardening depth

---

## Gate Verification & Evidence

### Build Gates
- [ ] Server P5-S01 + P5-S02 tests compile on linux-release
- [ ] Server P5-S01 + P5-S02 tests compile on community-release
- [ ] LLM P5-L01 + P5-L02 tests compile on linux-release
- [ ] LLM P5-L01 + P5-L02 tests compile on community-release
- [ ] No new compiler warnings introduced

### Test Gates
- [ ] Server: WSR-01..19 all PASS (deterministic, seed 42)
- [ ] Server: HST-01..20 all PASS (deterministic, seed 42)
- [ ] LLM: EXS-01..25 all PASS (deterministic, seed 42)
- [ ] LLM: MEM-01..28 all PASS (deterministic, seed 42)
- [ ] release_critical CI pipeline PASS with new tests integrated
- [ ] No flakes on repeated runs (3x+ confirmation)

### Performance Gates
- [ ] Wave 7 read latency: no regression > 5% vs baseline
- [ ] Wave 7 write latency: no regression > 5% vs baseline
- [ ] Wave 7 range query latency: no regression > 5% vs baseline
- [ ] Wave 7 batch insert throughput: no regression > 5% vs baseline
- [ ] Baseline and post-change metrics recorded in evidence bundle

### Security & Correctness Gates
- [ ] Sanitizer verification (ASan/TSan/UBSan):
  - [ ] Server tests: zero new leaks, races, undefined behavior
  - [ ] LLM tests: zero new leaks, races, undefined behavior
- [ ] CodeQL scan: zero new high/critical findings (exempting large-database skips)
- [ ] Manual code review: no new data-loss or correctness issues on critical paths

### Documentation Gates
- [ ] All public APIs have Doxygen @brief, @param, @return, @throws (where applicable)
- [ ] Failure modes documented (error codes, recovery semantics, timeouts)
- [ ] ROADMAP.md Phase 1 checkboxes updated with evidence link
- [ ] Module-level ROADMAP.md (src/server/ROADMAP.md, src/llm/ROADMAP.md) Phase 5 section marked complete

---

## Execution Milestones

| Milestone | Date | Owner | Status |
|-----------|------|-------|--------|
| Phase 1 kickoff (this doc + briefing) | 2026-08-02 | Copilot | 🟢 READY |
| Server P5-S01 + P5-S02 tests drafted | 2026-08-05 | themisdb-implementer | 🔵 TODO |
| Server P5-S01 + P5-S02 all tests PASS | 2026-08-10 | themisdb-implementer | 🔵 TODO |
| Server Phase 5 review gate PASS | 2026-08-12 | themisdb-reviewer | 🔵 TODO |
| LLM P5-L01 + P5-L02 tests drafted | 2026-08-06 | themisdb-implementer | 🔵 TODO |
| LLM P5-L01 + P5-L02 all tests PASS | 2026-08-15 | themisdb-implementer | 🔵 TODO |
| LLM Phase 5 review gate PASS | 2026-08-17 | themisdb-reviewer | 🔵 TODO |
| Wave 7 re-verification PASS | 2026-08-20 | Infrastructure | 🔵 TODO |
| release_critical full run PASS | 2026-08-22 | CI | 🔵 TODO |
| Phase 1 exit gate PASS (all gates green) | 2026-08-31 | Orchestrator | 🔵 TODO |

---

## Risk Register

| Risk | Severity | Mitigation | Owner |
|------|----------|-----------|-------|
| Flaky tests from concurrent scheduling | HIGH | Use seed 42 + steady_clock for determinism; run 3x+ before sign-off | themisdb-implementer |
| Wave 7 regression from retry/timeout changes | HIGH | Baseline + post-change metrics; rollback path documented | Performance team |
| Sanitizer false positives in LLM tests | MEDIUM | Verify with manual inspection + second sanitizer run; suppress if false positive | themisdb-reviewer |
| Missing Doxygen coverage on existing code | MEDIUM | Scan public APIs; require @file header on all public .cpp + .h files | themisdb-doc-orchestrator |
| Release-critical CI instability | MEDIUM | Keep CI gate green as ongoing maintenance; escalate blockers immediately | Infrastructure |

---

## Definition of Done (Phase 1 Exit)

All of the following must be true to close Phase 1 and proceed to Phase 2:

### Code Quality
- [x] Server: P5-S01 + P5-S02 implementation complete and integrated
- [x] LLM: P5-L01 + P5-L02 implementation complete and integrated
- [x] Sharding: Phase 6 reference (already complete)
- [ ] All public APIs have Doxygen comments (@brief, @param, @return, @throws)
- [ ] No new CRITICAL-severity code-review findings
- [ ] Failure modes and recovery semantics explicitly documented

### Testing
- [ ] Server: 39 tests PASS (19 WSR + 20 HST) on all platforms
- [ ] LLM: 53 tests PASS (25 EXS + 28 MEM) on all platforms
- [ ] No flakes on 3 consecutive full runs
- [ ] Sanitizer verification: zero new leaks, races, undefined behavior
- [ ] release_critical CI pipeline PASS with integrated tests

### Performance
- [ ] Wave 7 baseline metrics recorded
- [ ] Post-change Wave 7 metrics: no regression > 5% across read/write/range/batch
- [ ] Rollback path documented and tested

### Security & Compliance
- [ ] CodeQL: zero new high/critical findings
- [ ] Pentest evidence (Batch C): no new Critical/High in modified code
- [ ] Input validation hardened on retry/timeout paths

### Documentation & Governance
- [ ] ROADMAP.md Phase 1 checkboxes: [~] → [x] with evidence links
- [ ] src/server/ROADMAP.md Phase 5: marked complete
- [ ] src/llm/ROADMAP.md Phase 5: marked complete
- [ ] Phase 1 exit gate: all DoD items signed-off by owner + reviewer

---

## Communication & Escalation

- **Daily Standup:** 08:00 UTC (async updates in control board)
- **Blocker Escalation:** Escalate to GA_PROMOTION_SIGN_OFF.md owner immediately if:
  - Build fails on any preset (linux-release, community-release, windows-release)
  - release_critical CI stays red > 2 hours
  - New CRITICAL security finding discovered
  - Wave 7 regression > 5% confirmed
- **Weekly Sync:** 15:00 UTC every Friday (teams + reviewers + orchestrator)

---

## Evidence Bundle Location

Once complete, all evidence will be collected at:

```
ai_working/phase1_execution_evidence/
├── server_phase5_build_log.txt
├── server_phase5_test_results.xml
├── server_phase5_wave7_baseline.json
├── server_phase5_wave7_postchange.json
├── llm_phase5_build_log.txt
├── llm_phase5_test_results.xml
├── llm_phase5_sanitizer_report.txt
├── phase1_code_review_findings.md
├── phase1_doxygen_coverage_report.md
└── phase1_exit_gate_checklist.md
```

Evidence links will be added to:
- `ROADMAP.md` § Phase 1
- `docs/governance/GA_PROMOTION_SIGN_OFF.md` § Batch D (D-1..D-10)
- `FINAL_GA_READINESS_CHECKLIST.md` § Phase 1 Exit Gates
