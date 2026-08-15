# Content Module Batch 5 GA Finalization — Parallel Execution Coordination

**Status:** 🚀 **EXECUTION LAUNCH (2026-08-15 13:58 UTC)**  
**Release Target:** v2.4.0 GA (Week of Sept 22, 2026)  
**Timeline:** 2–3 weeks (aggressive parallel execution)  
**Scope:** CMT-7500 through CMT-7506 (6 parallel + sequential phases)  
**Model:** 4-stream parallel execution (Streams A–D)

---

## Execution Model (4-Stream Parallel)

```
TIMELINE: [Week 1]          [Week 2]          [Week 3]
          ───────────────────────────────────────────

Stream A: Phase 1 TRIAGE    Phase 2 CRITICAL  Phase 3 HIGH (Batch 1-3)
          ─────────────────────────────────────────────
          CMT-7500 hdrs     CMT-7501 scores   CMT-7502 TODOs
          (verif ready)     (batch by type)   (classify all)

Stream B: Phase 3 HIGH      Phase 4 CRITICAL  Phase 5 SCOPE+DOCS
          ─────────────────────────────────────────────
          CMT-7502 classify CMT-7503 adapters CMT-7504 linksets
          (3 parallel)      (3 files fixed)   (cross-sync)

Stream C: Phase 4 MEDIUM    Phase 5 SCOPE     Phase 6 TESTS+GATE
          ─────────────────────────────────────────────
          CMT-7503 rework   CMT-7504 docs     CMT-7505 coverage
          (2 adapters)      (2 sync items)    (integration tests)

Stream D: Phase 5 REVIEW    Checkpoint 1-4    GA SIGN-OFF
          ─────────────────────────────────────────────
          Continuous        (Weekly 2024)     CMT-7506 final gate
          code review       consolidation     (v2.4.0 ready)
```

---

## Agent Dispatch (Parallel Sub-Agents)

### Stream A: Doxygen + Metadata Baseline (gap-verifier lead)

**Agent:** `content-batch5-phase1-doxygen`  
**Type:** `themisdb-reviewer` (verification focus)  
**Timeline:** Week 1 (Days 1–3)  
**Scope:** CMT-7500 & CMT-7501  

**Input Context:**
```json
{
  "module": "content",
  "batch": 5,
  "phases": ["CMT-7500", "CMT-7501"],
  "files_target": 44,
  "findings_count": {"doxygen_headers": 47, "metadata_issues": 44},
  "template_standard": "See src/content/MODULE_GAPS_BATCH5.md §CMT-7500",
  "acceptance": {
    "doxygen_all_headers_match_template": true,
    "maturity_scores_verified": true,
    "no_doxygen_warnings": true
  }
}
```

**Deliverables:**
- [ ] Verified Doxygen template for all 44 files
- [ ] Maturity score calculation spreadsheet (CMT-FIN-01..20)
- [ ] Gap summary metadata synchronized (TODO/Stub/Unimpl/Mock/Sim/Debt counts)
- [ ] Test suite CMT-FIN-01..20 ready for Phase 2

**Success Criteria:**
- [ ] `doxygen Doxyfile.audit` runs clean (0 warnings in content section)
- [ ] All 44 files have Maturity score >= 50 (minimum for consideration)
- [ ] Score >= 85 for PRODUCTION-READY set (abuse_detector, audio_processor, etc.)

---

### Stream B: Production TODO Classification (themisdb-implementer lead)

**Agent:** `content-batch5-phase3-todos`  
**Type:** `themisdb-implementer`  
**Timeline:** Week 1–2 (Days 2–7, parallel with Stream A)  
**Scope:** CMT-7502 (73 TODO instances)  

**Input Context:**
```json
{
  "module": "content",
  "batch": 5,
  "phase": "CMT-7502",
  "files_affected": "all content module .cpp files",
  "todo_count": 73,
  "classification_types": ["Optimization", "Feature", "Vendor", "Other"],
  "requirement": "Each TODO must have GitHub issue ref OR documented rationale",
  "output": {
    "classification_report": "CONTENT_DEFERRED_FEATURES.md",
    "issue_correlation": "track all findings"
  }
}
```

**Deliverables:**
- [ ] Classified all 73 TODOs by type (Optimization/Feature/Vendor/Other)
- [ ] Added GitHub issue references to all legitimate production TODOs
- [ ] Created CONTENT_DEFERRED_FEATURES.md documenting deferred work
- [ ] Test suite CMT-FIN-21..35 ready

**Success Criteria:**
- [ ] 100% of production TODOs have issue reference or documented rationale
- [ ] CONTENT_DEFERRED_FEATURES.md updated in FUTURE_ENHANCEMENTS.md cross-refs
- [ ] No orphaned TODOs without tracking

---

### Stream C: Scope Mismatch + Documentation Sync (themisdb-implementer lead)

**Agent:** `content-batch5-phase4-scope-docs`  
**Type:** `themisdb-implementer`  
**Timeline:** Week 2–3 (Days 6–14)  
**Scope:** CMT-7503 (3 critical adapters) + CMT-7504 (2 doc sync items)  

**Input Context:**
```json
{
  "module": "content",
  "batch": 5,
  "phases": ["CMT-7503", "CMT-7504"],
  "critical_files": {
    "image_extractor_adapter.cpp": "lines 25-26 (dangling pointer)",
    "pdf_extractor_adapter.cpp": "line 26 (dangling pointer)"
  },
  "doc_sync": {
    "ROADMAP.md": "update processor inventory count",
    "FUTURE_ENHANCEMENTS.md": "add deferred features from CMT-7502",
    "README.md": "cross-reference current state"
  },
  "test_requirement": "CMT-FIN-36..40 scope validation, CMT-FIN-41..50 doc link validation"
}
```

**Deliverables:**
- [ ] Fixed image_extractor_adapter.cpp (ownership transfer via unique_ptr)
- [ ] Fixed pdf_extractor_adapter.cpp (ownership transfer via unique_ptr)
- [ ] Updated ROADMAP.md with current processor inventory
- [ ] Updated FUTURE_ENHANCEMENTS.md with deferred features
- [ ] Cross-checked all doc linksets (no broken anchors)
- [ ] Test suite CMT-FIN-36..50 ready

**Success Criteria:**
- [ ] All adapters use RAII ownership patterns (unique_ptr/shared_ptr)
- [ ] No dangling pointers (verified by static analysis)
- [ ] All doc links validate (`markdown-link-check` passes)
- [ ] ROADMAP/FUTURE_ENHANCEMENTS/README all in sync

---

### Stream D: Continuous Code Review + 4 Checkpoints (themisdb-reviewer lead)

**Agent:** `content-batch5-phase5-review`  
**Type:** `themisdb-reviewer` (continuous integration)  
**Timeline:** Weeks 1–3 (parallel with all other streams)  
**Scope:** Continuous review + 4 checkpoint consolidations  

**Input Context:**
```json
{
  "module": "content",
  "batch": 5,
  "role": "continuous code review + checkpoint consolidation",
  "checkpoints": {
    "CP-1": "Day 3 (2026-08-18) — Stream A baseline verification",
    "CP-2": "Day 7 (2026-08-22) — Stream B TODO classification + Stream C scope fixes",
    "CP-3": "Day 11 (2026-08-26) — Stream A/B/C deliverables integrated",
    "CP-4": "Day 14 (2026-08-29) — Final GA sign-off gate (CMT-7506)"
  },
  "review_focus": [
    "code quality (clang-format, clang-tidy)",
    "test coverage (95%+ for fixed gaps)",
    "security (CodeQL, no new vulnerabilities)",
    "performance (benchmark neutral or improved)"
  ]
}
```

**Deliverables (Per Checkpoint):**

**CP-1 (Day 3):** Stream A Baseline Verification
- [ ] All 44 Doxygen headers verified and rendered
- [ ] Maturity scores calculated and validated
- [ ] No doxygen warnings in content section
- [ ] Blocker review: 0 CRITICAL findings in header format

**CP-2 (Day 7):** Mid-Stream Integration
- [ ] Stream B TODOs classified (73/73 items processed)
- [ ] Stream C scope fixes for 2 adapters (CMT-7503-01, CMT-7503-02 merged)
- [ ] Blocker review: 0 dangling pointer issues
- [ ] Cross-module dependency check (no Stream A/B/C conflicts)

**CP-3 (Day 11):** Full Integration
- [ ] All Streams A/B/C deliverables merged to integration branch
- [ ] Test suite CMT-FIN-01..50 passing (>95% coverage)
- [ ] Documentation linksets validated (CMT-7504)
- [ ] Security scanning passed (CodeQL, SAST, no new alerts)

**CP-4 (Day 14):** GA Sign-Off Gate
- [ ] All 6 phases complete and integrated
- [ ] Content module maturity score >= 85/100
- [ ] CI/CD all green (build, test, security, performance)
- [ ] GA_PROMOTION_SIGN_OFF.md § Content Module (Batch 5) signed

**Success Criteria (All Checkpoints):**
- [ ] 0 CRITICAL findings blocking merge
- [ ] CI/CD green on all presets (community-release, linux-release, etc.)
- [ ] Benchmark performance neutral or improved
- [ ] Two-reviewer sign-off ready for final merge

---

## Phase Specifications

### Phase 1: Doxygen Header Standardization (CMT-7500)

**Files:** 44 content processor .cpp files  
**Template (from MODULE_GAPS_BATCH5.md):**

```cpp
/**
 * @file <filename>
 * @brief <One-line description of class/component functionality>.
 * @version <SEMVER matching module version>
 * @note Maturity: 🟢 PRODUCTION-READY | 🟡 BETA | 🔴 ALPHA
 * @note Score: <N>/100 (implementation completeness + test coverage)
 * @note Gap Summary: total=<N>; TODO=<N>, Stub=<N>, Unimpl=<N>, Mock=<N>, Sim=<N>, Debt=<N>, C=<N>, H=<N>, M=<N>, L=<N>
 * @note Status: <Production Ready | Beta | Alpha>
 * @note This block is auto-generated and will be overwritten.
 */
```

**Batching Strategy (User Preference: Larger Batches):**

- **Batch A (Days 1–2):** Core processor files (12 files, highest coverage)
  - abuse_detector.cpp, audio_processor.cpp, content_manager.cpp
  - ...and 9 more high-maturity files
  
- **Batch B (Day 3):** Medium-maturity processors (16 files)
  - mime_detector.cpp, office_processor.cpp, geo_processor.cpp
  - ...and 13 more medium-maturity files
  
- **Batch C (Days 4–5):** Specialized/edge processors (16 files)
  - mock_clip_processor.cpp, content_policy.cpp, specialized extractors
  - ...and 14 more lower-maturity/alpha files

**Verification:** `doxygen Doxyfile.audit` must pass with 0 warnings.

---

### Phase 2: Maturity Metadata Verification (CMT-7501)

**Formula:**
```
Score = (100 * implementations_complete / total_implementations) 
       + (test_lines / code_lines * 20)
```

**Bands:**
- PRODUCTION-READY: Score >= 85
- BETA: Score 70–84
- ALPHA: Score < 70

**Files Requiring Verification:**

| File | Current Score | Target | Action |
|---|---:|---:|---|
| abuse_detector.cpp | 85 | 85 (maintain) | Verify; document rationale |
| audio_processor.cpp | 80 | 88+ | Minimal test coverage improvement |
| content_manager.cpp | 78 | 92+ | Test coverage correlation |
| ... (41 more files) | ... | ... | ... |

---

### Phase 3: Production TODO Validation (CMT-7502)

**73 TODO instances across module**

**Classification by Type:**
1. **Optimization** (deferred perf improvements)
2. **Feature** (conditional feature flags)
3. **Vendor** (integration deferred pending contracts/availability)
4. **Other** (documentation, cleanup)

**Requirement:** Each TODO must have either:
- GitHub issue reference (e.g., `//TODO(#5678): feature-name`)
- Documented rationale in FUTURE_ENHANCEMENTS.md

**Output Artifact:** `CONTENT_DEFERRED_FEATURES.md` with:
- Complete inventory of all deferred work
- Classification by type
- Target release for each deferred item
- Dependency graph (if any item blocks another)

---

### Phase 4: Critical Scope Mismatch Fixes (CMT-7503)

**3 Critical Findings:**

**CMT-7503-01: image_extractor_adapter.cpp:25–26**

```cpp
// BEFORE (DANGLING POINTER)
std::string* image_extractor_adapter::extractImage(...) {
    std::string result = processImage(data);
    return &result;  // Invalid after function return
}

// AFTER (FIXED)
std::unique_ptr<std::string> image_extractor_adapter::extractImage(...) {
    auto result = std::make_unique<std::string>();
    *result = processImage(data);
    return result;  // Safe ownership transfer
}
```

**CMT-7503-02: pdf_extractor_adapter.cpp:26**
- Similar fix: replace raw pointer return with unique_ptr ownership

**CMT-7503-03: Add scope validation tests**
- Test suite CMT-FIN-36..40
- Verify stack/heap boundary checks
- RAII pattern validation

**Success Criteria:**
- [ ] No dangling pointers (verified by static analysis + clang-tidy)
- [ ] All adapters use smart pointers (unique_ptr/shared_ptr)
- [ ] Tests CMT-FIN-36..40 passing

---

### Phase 5: Documentation Sync (CMT-7504)

**2 Doc Sync Items:**

1. **ROADMAP.md update:** Update content section with current processor inventory (44 files confirmed production-ready for GA)

2. **FUTURE_ENHANCEMENTS.md sync:** Integrate deferred features from CMT-7502 TODO classification

3. **README.md cross-reference:** Update module overview with current state

4. **Automated link validation:** Add CI step to check for broken anchors (`markdown-link-check`)

**Success Criteria:**
- [ ] All ROADMAP/FUTURE_ENHANCEMENTS/README sections in sync
- [ ] No broken anchor references
- [ ] Phase status consistent across all docs
- [ ] CI includes markdown link validation

---

### Phase 6: Test Coverage + GA Promotion (CMT-7505 + CMT-7506)

**CMT-7505: Test Coverage Correlation**

**Strategy:**
- [ ] Aggregate all Batch 1–4 remediation items (CRITICAL 48 + HIGH 402)
- [ ] For each fix: verify corresponding test in tests/content/ exists
- [ ] Run: `ctest --preset community-release -L content`
- [ ] Generate test coverage report (gap-to-test mapping)

**Target:** 95%+ of fixed gaps have test validation

**CMT-7506: GA Promotion Sign-Off**

**Final Checklist:**
- [ ] All Batch 1–4 deliverables merged and passing CI
- [ ] Doxygen headers complete (CMT-7500) ✅
- [ ] Maturity metadata verified (CMT-7501, score >= 85 for GA processors) ✅
- [ ] Production TODOs validated (CMT-7502) ✅
- [ ] Scope mismatches fixed (CMT-7503) ✅
- [ ] Documentation linksets synchronized (CMT-7504) ✅
- [ ] Test coverage >= 95% (CMT-7505) ✅
- [ ] Security scanning passed (CodeQL, SAST) ✅
- [ ] Performance benchmarks neutral or improved ✅
- [ ] Two reviewer sign-off (architecture + security) ✅

**Sign-Off Record:** docs/governance/GA_PROMOTION_SIGN_OFF.md § Content Module (Batch 5)

---

## Acceptance Criteria (All Phases Complete)

**Batch 5 is complete when:**

1. ✅ All Doxygen headers match standard template (CMT-7500)
2. ✅ All 44 files have verified maturity scores >= 50 (metadata)
3. ✅ All 73 production TODOs classified and tracked (CMT-7502)
4. ✅ All 3 scope_mismatch findings fixed (CMT-7503)
5. ✅ All documentation linksets pass validation (CMT-7504)
6. ✅ Test coverage >= 95% for all batch deliverables (CMT-7505)
7. ✅ GA sign-off checklist complete (CMT-7506)
8. ✅ CI/CD all green (build, test, security, performance)
9. ✅ Content module maturity score >= 85/100
10. ✅ Ready for production release v2.4.0 GA

---

## Checkpoint Schedule

| Checkpoint | Date | Focus | Blocker Review |
|---|---|---|---|
| **CP-1** | 2026-08-18 (Day 3) | Stream A (Doxygen + Metadata) | 0 CRITICAL findings |
| **CP-2** | 2026-08-22 (Day 7) | Streams B+C (TODOs + Scope fixes) | 0 dangling pointers |
| **CP-3** | 2026-08-26 (Day 11) | Full integration (all 3 streams) | 95%+ coverage, CI green |
| **CP-4** | 2026-08-29 (Day 14) | Final GA sign-off gate | All criteria met, ready for merge |

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Doxygen formatting conflicts | Medium | Low | Template validation in Stream A early |
| Maturity score recalculation reveals new gaps | Low | Medium | Gap-verifier review in CP-1 blocker check |
| Scope mismatch fixes break ABI | Low | High | Full test suite validation (CMT-FIN-36..40) |
| Doc linkset sync cascades | Medium | Low | Automated CI step added early (CMT-7504) |
| Test coverage plateau at 92% | Medium | Medium | Prioritize gap-to-test coverage by severity |

---

## Next Actions

1. **Today (2026-08-15 13:58 UTC):** Launch 4 parallel sub-agents (Streams A–D)
2. **Day 1 (2026-08-16):** Stream A Doxygen baseline + Stream B TODO scan starts
3. **Day 3 (2026-08-18):** CP-1 blocker review — verify Stream A completeness
4. **Day 7 (2026-08-22):** CP-2 — Stream B/C integration checkpoint
5. **Day 11 (2026-08-26):** CP-3 — Full integration + security/perf validation
6. **Day 14 (2026-08-29):** CP-4 — Final GA sign-off + v2.4.0 merge ready

---

**Owner:** Content Module Team  
**Coordination:** AI Sub-Agent Orchestration (4-stream parallel model)  
**Authority:** src/content/MODULE_GAPS_BATCH5.md  
**Sign-Off:** docs/governance/GA_PROMOTION_SIGN_OFF.md § Content Module (Batch 5)
