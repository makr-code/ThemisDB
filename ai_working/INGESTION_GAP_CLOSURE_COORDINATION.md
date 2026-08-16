# Ingestion Module Gap Closure Coordination — Master Plan & Status

**Status:** Implementation In Progress  
**Start Date:** 2026-08-15  
**Target Completion:** 2026-09-19  
**Release Target:** v2.4.0 GA (Week of Sept 22, 2026)  

---

## Executive Summary

The ingestion module has **324 total findings** with **144 actionable items** (41 CRITICAL, 103 HIGH) across 34 affected files. Execution follows a 6-phase coordinated closure model with parallel sub-agent streams to compress timeline from linear to wave-based closure.

**Key Metrics:**
- CRITICAL findings: 41 (must fix)
- HIGH findings: 103 (must fix)
- MEDIUM findings: 173 (resolve or defer with justification)
- LOW findings: 7 (quality polish)
- Total: 324 findings across 34 files

**Wave A Priority:** This work supports v2.4.0 GA release hardening path (ROADMAP.md Wave A → B → C → D model).

---

## Phase Breakdown & Status

### Phase 1: Triage & Verification (Week 1: Aug 15–22)
**Status:** [~] IN PROGRESS  
**Agent:** `gap-verifier`  
**Scope:** 324 findings across 34 files  

**Objectives:**
- [ ] Run gap-scanner verification against src/ingestion/MODULE_GAPS.md
- [ ] Classify findings into 5 severity tiers:
  - CRITICAL-BLOCKING: requires architecture review
  - CRITICAL-HIGH: direct data-race/memory-safety issues
  - HIGH: performance/reliability concerns
  - MEDIUM: code quality / standard compliance
  - LOW: documentation polish
- [ ] Identify and isolate false positives
- [ ] Generate PHASE_1_VERIFICATION_REPORT.json with tier assignments
- [ ] Flag timeout findings (15 instances) for coordinator state-machine review
- [ ] Flag data-race findings (4 instances) for ThreadSanitizer validation
- [ ] Flag resource-leak findings (28 instances) for exception-path test priority

**Entry Criteria:**
- [ ] src/ingestion/MODULE_GAPS.md available (✅ exists)
- [ ] gap-verifier sub-agent available

**Exit Criteria:**
- [ ] PHASE_1_VERIFICATION_REPORT.json generated
- [ ] All 144 actionable findings classified
- [ ] False positives isolated and documented
- [ ] High-risk categories (timeout, data-race, resource-leak) flagged

**Deliverable:** INGESTION_PHASE_1_VERIFICATION_REPORT.json (ready for Phase 2 dispatch)

---

### Phase 2: CRITICAL Fixes (Week 2–3: Aug 22–Sep 5)
**Status:** [ ] READY FOR DISPATCH  
**Agent:** `themisdb-implementer`  
**Scope:** 41 CRITICAL findings  

**Target Files (by priority):**
- ingestion_manager.cpp (5 CRITICAL): no_timeout (3), data_race (1), resource leak (1)
- ingestion_coordinator.cpp (5 CRITICAL): no_timeout (2), uninitialized_access (2), exception leak (1)
- filesystem_ingester.cpp (3 CRITICAL): resource leak (2), pointer arithmetic (1)
- database_connector.cpp (3 CRITICAL): resource leak (2), data_race (1)
- tensor_core_bridge_step.cpp (2 CRITICAL): resource leak (1), uninitialized (1)
- Others: (23 CRITICAL across remaining files)

**Focused Categories:**
- no_timeout (5): Add timeout parameters to file I/O operations
- data_race (4): Add mutex guards to shared data access
- resource leak in exception (8): Wrap resource acquisition in RAII
- uninitialized_access (3): Initialize all members in constructors
- pointer arithmetic (2): Bounds-check array access

**Deliverables per Fix:**
- [ ] Code fix (C++ with RAII safety)
- [ ] Focused unit test (INGESTION-xxx pattern, 1 test per fix)
- [ ] GitHub issue reference or documented rationale
- [ ] Clang-format validation

**Quality Gates:**
- [ ] All fixes pass clang-format
- [ ] All fixes have corresponding test cases
- [ ] No new compiler warnings
- [ ] `release_critical` CI gate green

**Exit Criteria:**
- [ ] All 41 CRITICAL findings resolved or explicitly deferred with justification
- [ ] Test coverage: 100% of fixes validated
- [ ] CI/CD green on develop

**Deliverable:** INGESTION_PHASE_2_COMPLETION_SUMMARY.md (status of all 41 fixes + test evidence)

---

### Phase 3: HIGH Fixes Batch A (Week 3–4: Aug 29–Sep 5)
**Status:** [ ] READY FOR DISPATCH  
**Agent:** `themisdb-implementer` (parallel with Phase 2)  
**Scope:** 103 HIGH findings  

**Top-Risk Target Files (Batch A1):**
- ingestion_manager.cpp (12 HIGH): copy_overhead (3), missing_latency_metric (2), hardcoded_path (2), others (5)
- ingestion_coordinator.cpp (15 HIGH): string_concat_loop (5), unordered_iter (3), generic_catch (3), others (4)
- cdc_connector.cpp (5 HIGH): resource leak (2), copy_overhead (2), iterator_invalidation (1)
- kafka_connector.cpp (5 HIGH): string_concat_loop (2), hardcoded_path (2), missing_metric (1)
- web_crawler_connector.cpp (4 HIGH): resource leak (2), missing_timeout (1), copy_overhead (1)
- Others: (62 HIGH across remaining 29 files)

**Focused Categories:**
- string_concat_loop (47): Replace with std::string or std::vector::insert()
- resource_leaked_in_exception (28): Convert to std::unique_ptr / std::shared_ptr
- copy_overhead (19): Use std::string_view, std::span, move semantics
- unordered_container_iter (13): Validate iterator validity after insertion/deletion
- missing_latency_metric (12): Add timing instrumentation

**Per-File Strategy:**
- Batch A1 (top 5 files): 44 HIGH findings → execute first
- Batch A2 (next 10 files): 38 HIGH findings → parallel execution
- Batch A3 (remaining 19 files): 21 HIGH findings → async task agents

**Deliverables per Fix:**
- [ ] Code refactoring (modern C++ best practices)
- [ ] Integration test (validates real-world scenario, not unit mock)
- [ ] Performance validation (benchmark neutral or improved)
- [ ] GitHub issue reference or FUTURE_ENHANCEMENTS.md update

**Quality Gates:**
- [ ] All fixes pass static analysis
- [ ] Integration tests pass on `community-release` preset
- [ ] Performance benchmarks show no regression

**Exit Criteria:**
- [ ] All 103 HIGH findings resolved or deferred
- [ ] Test coverage: ≥90% of fixes validated
- [ ] Benchmarks stable or improved

**Deliverable:** INGESTION_PHASE_3_COMPLETION_SUMMARY.md (per-batch status + performance report)

---

### Phase 4: MEDIUM/LOW Fixes (Week 4–5: Sep 5–12)
**Status:** [ ] READY FOR DISPATCH  
**Agent:** `task` agents (lightweight, high throughput)  
**Scope:** 173 MEDIUM + 7 LOW findings  

**Distribution Strategy:**
- Launch 3-4 parallel `task` agents (one per batch of ~50 findings)
- Each agent handles independent file groups or categories
- Focus on code quality gates (linting, style, documentation)

**Focused Categories:**
- iterator_invalidation (9): Add iterator-safety validation
- expensive_inner_op (3): Reduce algorithmic complexity
- allocation_loop (2): Move allocations outside loops
- missing_dtor (2): Add explicit destructors where needed
- module_doc_linkset_drift (2): Cross-reference validation
- Others: (152 misc MEDIUM/LOW quality improvements)

**Per-Fix Pattern:**
- [ ] Linting fix (clang-format, clang-tidy)
- [ ] Style validation (repository conventions)
- [ ] Test case if behavioral change
- [ ] Documentation update if doc-related

**Quality Gates:**
- [ ] Linter green (clang-format + clang-tidy)
- [ ] No test regressions
- [ ] Documentation consistency

**Exit Criteria:**
- [ ] All 173 MEDIUM findings resolved or deferred
- [ ] All 7 LOW findings resolved
- [ ] Linting gates pass for all files

**Deliverable:** INGESTION_PHASE_4_COMPLETION_SUMMARY.md (per-file linting status)

---

### Phase 5: Review & CI Integration (Week 5–6: Sep 5–12)
**Status:** [ ] READY FOR DISPATCH  
**Agent:** `themisdb-reviewer`  
**Scope:** Continuous review during Phases 2–4; final aggregation  

**Objectives:**
- [ ] Continuous code review of Phases 2–4 deliverables
- [ ] Aggregate gap closure metrics (findings closed vs. deferred)
- [ ] Validate all CRITICAL/HIGH fixes have test coverage ≥95%
- [ ] Cross-check for regressions (benchmark, functional, security)
- [ ] Generate compliance report (before/after gap counts, closure rate)
- [ ] Prepare GA sign-off checklist

**Review Checkpoints:**
- **Checkpoint 1:** Phase 2 (41 CRITICAL) — week of Aug 29
- **Checkpoint 2:** Phase 3 Batch A1 (44 HIGH from top 5 files) — week of Sep 5
- **Checkpoint 3:** Phase 3 Batch A2/A3 + Phase 4 (59 HIGH + 180 MEDIUM/LOW) — week of Sep 12
- **Checkpoint 4:** Final compliance aggregation — week of Sep 19

**Compliance Report Contents:**
- Closure rate: X/144 CRITICAL+HIGH resolved
- False positive rate (from Phase 1 triage)
- Test coverage metrics (lines, coverage %, test case count)
- Regression risk assessment (benchmark data, functionality)
- Deferred findings justification + tracking issues

**Quality Gates:**
- [ ] All Phase 2 fixes reviewed and approved
- [ ] All Phase 3 fixes reviewed and approved
- [ ] All Phase 4 fixes reviewed and approved
- [ ] Compliance report complete and accurate

**Exit Criteria:**
- [ ] 100% of CRITICAL findings resolved or deferred with justification
- [ ] 100% of HIGH findings resolved or deferred with justification
- [ ] Compliance report shows ≥90% closure rate
- [ ] `release_critical` CI gate green on develop

**Deliverable:** INGESTION_PHASE_5_COMPLIANCE_REPORT.md (final gap closure metrics + GA sign-off readiness)

---

### Phase 6: Documentation & Release (Week 6–7: Sep 12–19)
**Status:** [ ] READY FOR DISPATCH  
**Agent:** `doc-orchestrator` (if governance-relevant) or manual sync  
**Scope:** Final documentation and release preparation  

**Objectives:**
- [ ] Finalize Doxygen headers if doc gaps exist
- [ ] Update src/ingestion/ROADMAP.md with Phase 1–5 closure summary
- [ ] Sync MODULE_GAPS.md with final closure status
- [ ] Update FUTURE_ENHANCEMENTS.md with deferred items
- [ ] Generate INGESTION_PHASE_6_RELEASE_NOTES.md
- [ ] Prepare GA sign-off at docs/governance/GA_PROMOTION_SIGN_OFF.md

**Documentation Tasks:**
- [ ] Review all Doxygen headers in 34 affected files
- [ ] Update maturity scores (if CMT-7501 pattern applies)
- [ ] Cross-check ROADMAP.md, FUTURE_ENHANCEMENTS.md, README.md consistency
- [ ] Archive MODULE_GAPS.md closure snapshot (append to CHANGELOG.md)

**GA Sign-Off Checklist:**
- [ ] All 41 CRITICAL findings fixed or justified
- [ ] All 103 HIGH findings fixed or justified
- [ ] All 173 MEDIUM findings resolved or tracked
- [ ] Test coverage ≥95% for all fixes
- [ ] `release_critical` CI green on develop
- [ ] Security scanning complete (CodeQL, SAST)
- [ ] Performance benchmarks neutral or improved
- [ ] Two-reviewer sign-off (architecture + security)

**Release Preparation:**
- [ ] Version bump (if applicable) in VERSION file
- [ ] CHANGELOG.md entry for v2.4.0 GA ingestion closure
- [ ] Merge to appropriate release branch (community, enterprise, etc.)

**Exit Criteria:**
- [ ] All documentation synchronized
- [ ] GA sign-off checklist complete
- [ ] Ready for v2.4.0 GA release (Week of Sept 22)

**Deliverable:** INGESTION_PHASE_6_RELEASE_NOTES.md + GA sign-off section in docs/governance/GA_PROMOTION_SIGN_OFF.md

---

## Sub-Agent Dispatch Strategy

### Parallel Execution Streams

**Stream A — CRITICAL Triage & Fix**
```
Phase 1: gap-verifier (Aug 15–22)
  → Outputs: INGESTION_PHASE_1_VERIFICATION_REPORT.json
  ↓
Phase 2: themisdb-implementer (Aug 22–Sep 5)
  → Inputs: Tier-1 (CRITICAL-BLOCKING) + Tier-2 (CRITICAL-HIGH) findings
  → Outputs: INGESTION_PHASE_2_COMPLETION_SUMMARY.md + test suite
```

**Stream B — HIGH Fixes (Parallel with Phase 2)**
```
Phase 3: themisdb-implementer Batch A1 (Aug 29–Sep 5)
  → Inputs: Top 5 files with HIGH findings (44 findings)
  → Outputs: INGESTION_PHASE_3_BATCH_A1_COMPLETION.md
  ↓
Phase 3: themisdb-implementer Batch A2/A3 (Sep 5–12)
  → Inputs: Remaining HIGH findings (59 findings)
  → Outputs: INGESTION_PHASE_3_BATCH_A2_COMPLETION.md + BATCH_A3_COMPLETION.md
```

**Stream C — MEDIUM/LOW Fixes (Parallel with Phase 2–3)**
```
Phase 4: task agents × 3 (Sep 5–12)
  → Inputs: 180 MEDIUM/LOW findings distributed across agents
  → Outputs: Per-agent batch completion reports
```

**Stream D — Continuous Review**
```
Phase 5: themisdb-reviewer (continuous Aug 29–Sep 12, final Sep 12–19)
  → Inputs: Phase 2–4 deliverables as they complete
  → Checkpoints: 4 review gates (see Phase 5 section)
  → Outputs: INGESTION_PHASE_5_COMPLIANCE_REPORT.md
```

**Stream E — Documentation & Release (Final)**
```
Phase 6: doc-orchestrator or manual (Sep 12–19)
  → Inputs: Phase 5 compliance report
  → Outputs: INGESTION_PHASE_6_RELEASE_NOTES.md + GA sign-off
```

### Timeline Visualization

```
Aug 15  Aug 22  Aug 29  Sep 5   Sep 12  Sep 19
|-------|-------|-------|-------|-------|
Phase 1 [========]
Phase 2           [===============]
Phase 3A          [========]
Phase 3B+4                 [===============]
Phase 5           [====== continuous review ======]
Phase 6                              [====]
```

---

## Risk Mitigation Table

| Risk Category | Impact | Instances | Mitigation Strategy | Owner |
|---|---|---|---|---|
| **Timeout Findings** | May require state-machine refactoring | 15 | Flag for architecture review; defer to Phase 2 if complex | themisdb-implementer |
| **Data Race Findings** | Memory safety blocker | 4 | Require ThreadSanitizer validation before merge | themisdb-reviewer |
| **Resource Leak in Exception** | Crash under error paths | 28 | Prioritize exception-path testing; verify RAII | themisdb-implementer |
| **False Positives** | Wasted effort, noise | ~5–10% (est.) | Capture in Phase 1 triage; document with rationale | gap-verifier |
| **CI/CD Regression** | Release blocker | Unknown | Baseline benchmarks before Phase 2; monitor after each fix | themisdb-reviewer |
| **Deferred Findings Overload** | Timeline slippage | If >20% defer rate | Escalate to Wave A owner; add Phase 3.5 if needed | project lead |

---

## Acceptance Criteria for Full Closure

**Phase 1 (Triage) Complete When:**
1. All 324 findings classified into 5 tiers
2. False positives isolated (≤10% allowed)
3. High-risk categories flagged for priority
4. INGESTION_PHASE_1_VERIFICATION_REPORT.json generated

**Phase 2 (CRITICAL Fixes) Complete When:**
1. All 41 CRITICAL findings fixed or justified
2. 100% have test coverage
3. No new compiler warnings or clang-tidy issues
4. `release_critical` CI gate green

**Phase 3 (HIGH Fixes) Complete When:**
1. All 103 HIGH findings fixed or deferred
2. ≥90% have integration test coverage
3. Performance benchmarks neutral or improved
4. Cross-file refactoring validated

**Phase 4 (MEDIUM/LOW) Complete When:**
1. All 180 MEDIUM/LOW findings resolved
2. Linting gates pass (clang-format, clang-tidy)
3. No test regressions
4. Documentation consistency verified

**Phase 5 (Review) Complete When:**
1. 4 review checkpoints all approved
2. Compliance report shows ≥90% closure rate
3. Regressions assessed (zero critical, <5% performance variance)
4. GA sign-off checklist prepared

**Phase 6 (Release) Complete When:**
1. All documentation synchronized
2. GA sign-off checklist complete with 2 reviewer approvals
3. Ready for v2.4.0 GA release (target: Week of Sept 22, 2026)

---

## File Inventory (34 Files in Scope)

### High-Priority Files (26 CRITICAL + 44 HIGH = 70 actionable)
1. ingestion_manager.cpp — 5 C, 12 H (mutex guards, timeouts, RAII)
2. ingestion_coordinator.cpp — 5 C, 15 H (thread-safety, exception handling)
3. cdc_connector.cpp — 2 C, 5 H (resource management)
4. ingestion_quality_judge.cpp — 3 C, 3 H (error handling)
5. filesystem_ingester.cpp — 3 C, 1 H (I/O safety)
6. database_connector.cpp — 3 C, 0 H (connection pooling)
7. kafka_connector.cpp — 2 C, 5 H (stream handling)
8. web_crawler_connector.cpp — 2 C, 4 H (network timeouts)
9. ingestion_sinks.cpp — 3 C, 4 H (write-path safety)
10. parse_text_step.cpp — 5 C, 3 H (text processing)
11–34. Others (remaining 15 C, 25 H distributed)

### Standard Files (103 MEDIUM + 7 LOW)
- Legal domain extraction, steps (ner, chunk, embed, etc.)
- Adapters and validators
- Helper utilities

---

## Coordination Workflow

Each phase agent receives:
1. **Input:** Verified findings list for their phase (from previous phase or triage)
2. **Execution:** Per-fix code changes + tests
3. **Validation:** Linting, testing, benchmarking as appropriate
4. **Output:** Phase completion summary (JSON + Markdown)
5. **Handoff:** Push changes; report_progress update

Each phase completes with:
- [ ] Commit to develop with descriptive message (Phase X: scope summary)
- [ ] engine-tools-report_progress call with updated checklist
- [ ] Hand-off summary for next phase agent

---

## Key References

- **Gap Source:** src/ingestion/MODULE_GAPS.md (324 findings, auto-generated from scanner)
- **Module Roadmap:** src/ingestion/ROADMAP.md (Phase 1–6 Wave A support)
- **Release Gate:** ROADMAP.md § Wave A Exit Criteria
- **GA Sign-Off:** docs/governance/GA_PROMOTION_SIGN_OFF.md
- **Test Infrastructure:** tests/ingestion/ (existing test suite)
- **Build Preset:** cmake --preset community-release (validation target)

---

**Last Updated:** 2026-08-15  
**Next Checkpoint:** Phase 1 triage completion → 2026-08-22  
**Status:** Ready for Phase 1 Agent Dispatch

