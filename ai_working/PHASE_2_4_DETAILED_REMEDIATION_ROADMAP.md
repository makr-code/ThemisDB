# Phase 2.4: Detailed Remediation Roadmap
## Graph Module Integration & Hardening

**Status:** 🟡 READY FOR EXECUTION  
**Timeline:** July 1–14, 2026 (2 weeks)  
**Scope:** 107 HIGH/MEDIUM findings remediation  
**Target Release:** v2.4  
**Owner:** @makr-code  

---

## Executive Summary

Phase 2.4 is the final integration and hardening phase for the ThemisDB graph module. Building on the successful resolution of all 9 CRITICAL gaps from Phases 2.1–2.3, this phase focuses on:

1. **Baseline Establishment:** Validate 326-test suite pass rates and identify pre-existing failures
2. **Conformance Verification:** Confirm all 9 CRITICAL gaps remain production-ready
3. **Finding Categorization & Remediation:** Triage and fix 107 HIGH/MEDIUM findings by scope and risk level
4. **Release Preparation:** Build v2.4 release candidate with version updates
5. **Stability Validation:** Execute 100× iteration stability runs

### Success Criteria

- ✅ 9 CRITICAL gaps remain RESOLVED (L1 conformance audit PASS)
- ✅ 326-test suite executes without regression
- ✅ 107 HIGH/MEDIUM findings categorized and remediated (or risk-accepted)
- ✅ v2.4 release artifacts generated
- ✅ 100× stability runs complete without anomalies

---

## Phase 2.4 Execution Roadmap

### Week 1 (July 1–7): Test Baseline & Conformance Audit

#### **Day 1–2 (Mon–Tue): Test Baseline Configuration**

**Objective:** Establish reproducible 326-test suite baseline

**Tasks:**
- [ ] Configure CMake with `--preset community-release`
- [ ] Build themis_graph module with full verbosity logging
- [ ] Create test environment documentation (CMake flags, compiler, OS)
- [ ] Establish test execution harness with timeout/resource limits

**Success Criteria:**
- CMake configuration succeeds without warnings
- Build completes with zero new errors in graph module
- Test harness ready for 326-test run

**Deliverable:** `PHASE_2.4_TEST_BASELINE_CONFIG.md`

**Parallel Work:**
- Review GRAPH_L1_AUDIT_REPORT_2026-07-01.md for known issues
- Collect static analysis baseline from previous scan

---

#### **Day 2–4 (Tue–Thu): Initial Test Suite Run**

**Objective:** Establish baseline pass/fail metrics for regression tracking

**Tasks:**
- [ ] Run 326-test suite: `ctest --preset community-release -R "test_graph" --output-on-failure -j 4`
- [ ] Capture full test output to baseline log with timestamps
- [ ] Categorize results: PASS | FAIL | SKIP with counts
- [ ] Analyze any pre-existing failures (root cause, frequency)
- [ ] Document test coverage breakdown by subsystem

**Success Criteria:**
- All tests execute without timeout
- Baseline metrics captured: total pass, fail, skip counts
- Pre-existing failures documented with root causes
- No regressions from phases 2.1–2.3 fixes

**Deliverable:** `PHASE_2.4_TEST_BASELINE_RESULTS.md` + test execution log

**Expected Output:**
```
Test Results Summary:
  PASS:    ~310 tests
  FAIL:    ~5–10 tests (pre-existing)
  SKIP:    ~5 tests
  Timeout: 0 tests
```

---

#### **Day 4–5 (Thu–Fri): L1 Conformance Audit**

**Objective:** Verify all 9 CRITICAL gaps remain RESOLVED with production context

**Files Under Audit:**

1. **`src/graph/rotate_completion.cpp`** (3 gaps)
   - Gap 2.1.1: entityEmbedding() scope & lifetime
   - Gap 2.1.2: relationPhase() iterator constructor
   - Gap 2.1.3: rankAll() cache consistency

2. **`src/graph/explain_plan.cpp`** (2 gaps)
   - Gap 2.2.1: toDot() defensive guard
   - Gap 2.2.2: toJson() defensive guard

3. **`src/graph/path_constraints.cpp`** (1 gap + ErrorRegistry)
   - Gap 2.2.3: ErrorRegistry switch exhaustiveness

4. **`src/graph/ontology_manager.cpp`** (1 gap)
   - Gap 2.3.1: YamlEntry RAII semantics

**Audit Tasks:**
- [ ] Inspect each gap fix for correctness and documentation
- [ ] Validate Doxygen documentation quality (purpose, parameters, return, failure modes)
- [ ] Verify RAII semantics are properly documented
- [ ] Check for any new warnings or regressions
- [ ] Confirm fail-safe behavior is preserved
- [ ] Cross-check with 326-test results for runtime alignment

**Gap Verification Checklist:**

**Gap 2.1.1** — entityEmbedding() scope & lifetime
- [ ] std::shared_lock acquired at function entry
- [ ] Lock released at scope exit (RAII)
- [ ] Move semantics documented in return comment
- [ ] Fail-safe behavior: returns empty vector if untrained
- [ ] Exception propagation documented

**Gap 2.1.2** — relationPhase() iterator constructor
- [ ] Vector constructed with iterator-range constructor (not initializer list)
- [ ] No dangling iterators or temporary references
- [ ] Tests verify correct element insertion
- [ ] Element count matches expected range

**Gap 2.1.3** — rankAll() cache consistency
- [ ] Cache access documented with thread-safety guarantees
- [ ] Returned results are independent vectors (no shared ownership)
- [ ] No cache corruption possible from external modifications
- [ ] Concurrent access patterns documented

**Gap 2.2.1** — toDot() defensive guard
- [ ] Null/empty checks present before access
- [ ] Returns sensible default for empty state
- [ ] Error handling documented in Doxygen

**Gap 2.2.2** — toJson() defensive guard
- [ ] Null/empty checks present before access
- [ ] Returns valid JSON structure for empty state
- [ ] Error handling documented in Doxygen

**Gap 2.2.3** — ErrorRegistry switch exhaustiveness
- [ ] Switch statement covers all ErrorRegistry enum values
- [ ] Default case exists with fail-safe fallback
- [ ] Compiler warnings absent (use -Wall -Wextra -Werror=switch)

**Gap 2.3.1** — YamlEntry RAII semantics
- [ ] Implicit destructor properly chains resource cleanup
- [ ] No manual delete/cleanup required by caller
- [ ] Move constructor/assignment documented
- [ ] Exception safety documented (strong or basic guarantee)

**Deliverable:** `PHASE_2.4_L1_CONFORMANCE_AUDIT_REPORT.md` (updated with production context)

**Expected Result:**
```
Audit Verdict: ✅ PASS
All 9 gaps verified RESOLVED
Ready for Phase 2.4 hardening phase
```

---

### Week 2 (July 8–14): Finding Remediation & Release

#### **Day 1–2 (Mon–Tue): Finding Categorization & Prioritization**

**Objective:** Triage 107 HIGH/MEDIUM findings by scope, risk level, and complexity

**Task Breakdown:**

**1. Static Analysis Finding Classification**

Categorize findings by scope:

| Scope Category | Count | Risk | Remediation Strategy |
|---|---|---|---|
| scope_mismatch | ~95 | HIGH | Defensive guards + documentation |
| iterator_invalidation | ~5 | CRITICAL | Code fixes + verification |
| uninitialized_access | ~3 | HIGH | Initialization guards |
| defensive_patterns | ~3 | MEDIUM | False positive assessment |
| Other | ~1 | LOW | Document or defer |

**Risk Classification:**

- **CRITICAL (Blocks Release):** Iterator invalidation, memory corruption risk
- **HIGH (Must Fix):** Scope mismatches in public APIs, defensive guard gaps
- **MEDIUM (Should Fix):** Non-critical guards, internal inconsistencies
- **LOW (Document):** False positives, known limitations

**Complexity Tiers:**

- **Tier 1 (Doc Only):** Update Doxygen, clarify scope in comments
- **Tier 2 (Trivial Code):** Add null checks, simple guards (< 5 LOC per fix)
- **Tier 3 (Moderate):** Refactor patterns, add defensive logic (5–20 LOC per fix)
- **Tier 4 (Complex):** Algorithm changes, architectural adjustments (> 20 LOC per fix)

**Tasks:**
- [ ] Extract detailed 107 findings list from static analysis
- [ ] Map findings to source files and line numbers
- [ ] Classify each finding: scope category, risk level, complexity tier
- [ ] Group findings by file for batch remediation
- [ ] Identify "quick wins" (Tier 1–2) for parallel execution

**Deliverable:** `PHASE_2.4_FINDING_CATEGORIZATION.json` + `FINDING_PRIORITIZATION.md`

**Output Format:**
```json
{
  "findings_total": 107,
  "by_scope": {
    "scope_mismatch": { "count": 95, "risk": "HIGH", "tier": "Tier 1-2" },
    "iterator_invalidation": { "count": 5, "risk": "CRITICAL", "tier": "Tier 3-4" },
    ...
  },
  "by_file": {
    "src/graph/rotate_completion.cpp": { "finding_ids": [...], "total": 28 },
    ...
  },
  "quick_wins": { "count": 45, "estimated_effort_hours": 8 },
  "complex_items": { "count": 12, "estimated_effort_hours": 24 }
}
```

---

#### **Day 2–3 (Tue–Wed): Quick-Win Remediation (Tier 1–2)**

**Objective:** Execute high-confidence, low-effort fixes in parallel

**Parallel Work Streams:**

**Stream A: Doxygen & Documentation Updates**
- [ ] Add `@param`, `@return`, `@throws` tags to public APIs
- [ ] Document scope lifetime expectations
- [ ] Clarify move semantics in function comments
- [ ] Add `@brief` descriptions for defensive guards

**Files:**
- `src/graph/rotate_completion.cpp`
- `src/graph/explain_plan.cpp`
- `src/graph/path_constraints.cpp`
- `src/graph/ontology_manager.cpp`

**Effort:** ~4 hours (parallelizable)

**Stream B: Defensive Guard Implementation (Null Checks, Empty Checks)**
- [ ] Add guard clauses for public API entry points
- [ ] Implement fail-safe returns (empty vectors, default objects)
- [ ] Add assertions for internal invariants
- [ ] Document guard rationale in comments

**Pattern Template:**
```cpp
/// Public API with defensive guard
std::vector<Result> getResults() const {
    if (!initialized_) return {};  // Fail-safe: empty vector
    // ... implementation ...
}
```

**Effort:** ~4 hours (parallelizable)

**Stream C: Iterator & Range Validation**
- [ ] Verify iterator ranges are materialized (not temporary)
- [ ] Add range-based assertions for bulk operations
- [ ] Document iterator lifetime contracts
- [ ] Add bounds checking for index-based access

**Effort:** ~2 hours (requires code review)

**Parallel Execution:**
- Assign developers to streams based on expertise
- Each developer works independently; coordinate at integration point
- Estimate 8–10 hours total for all Tier 1–2 fixes

**Deliverable:** Code changes committed with individual commit messages

---

#### **Day 4–5 (Thu–Fri): Integration Testing & Validation**

**Objective:** Verify all remediations work together and don't introduce regressions

**Tasks:**
- [ ] Rebuild graph module with all changes: `cmake --build --preset community-release --target themis_graph`
- [ ] Re-run 326-test suite: `ctest --preset community-release -R "test_graph"`
- [ ] Compare results to baseline (Day 2–4)
- [ ] Investigate any new failures
- [ ] Verify fix effectiveness (e.g., compiler warnings reduced, static analysis clean)
- [ ] Document remaining Tier 3–4 items for follow-up work

**Success Criteria:**
- No new test failures introduced
- Build succeeds with fewer or equal compiler warnings
- Static analysis pass rate improved
- All Tier 1–2 fixes implemented and verified

**Deliverable:** `PHASE_2.4_INTEGRATION_RESULTS.md` + `test_results_week2.log`

---

#### **Week 2 Additional (Mon–Thu): Tier 3–4 Complex Remediation (If Time Permits)**

**Objective:** Address complex findings requiring algorithm or architectural changes

**Scope:**
- Iterator invalidation patterns requiring refactoring
- Cache consistency improvements
- Memory safety hardening

**Execution:**
- [ ] Code review for each fix
- [ ] Unit test enhancements
- [ ] Performance impact assessment
- [ ] Documentation updates

**Target Completion:** July 11–13 (buffer for testing)

---

### Week 2 Closure (July 14): Release Preparation

#### **Day 5–6 (Fri + Weekend): Release Candidate Build & Verification**

**Objective:** Prepare v2.4 release candidate and validate stability

**Tasks:**
- [ ] Update version strings: `v2.4.0-rc1` in CMakeLists.txt, package.json, etc.
- [ ] Update CHANGELOG.md with Phase 2.4 fixes and improvements
- [ ] Generate release notes with finding remediation summary
- [ ] Build release package: `cmake --build --preset community-release --target package`
- [ ] Verify package contents (headers, libs, docs)
- [ ] Create release tag: `git tag v2.4.0-rc1`

**Deliverable:** `v2.4.0-rc1` release candidate

---

#### **Stability Validation (July 15–20): 100× Iteration Runs**

**Objective:** Validate release candidate stability under repeated execution

**Test Plan:**
- [ ] Run 326-test suite 100 times sequentially
- [ ] Monitor for flakiness, timeouts, resource leaks
- [ ] Capture metrics: pass rate, failure patterns, execution time trends
- [ ] Analyze any intermittent failures

**Success Criteria:**
- ≥ 99.5% pass rate (max 5 failures out of 500 test runs)
- No consistent failure patterns
- No resource exhaustion or memory leaks detected
- Execution time stable (coefficient of variation < 10%)

**Deliverable:** `PHASE_2.4_STABILITY_VALIDATION_REPORT.md`

---

## File-by-File Remediation Strategy

### **File 1: `src/graph/rotate_completion.cpp`**

**Current Status:** 3 CRITICAL gaps RESOLVED (Phases 2.1–2.3)

**Remaining Findings (HIGH/MEDIUM):** ~28 findings

**Remediation Plan:**

| Finding Type | Count | Risk | Tier | Action |
|---|---|---|---|---|
| scope_mismatch (entityEmbedding scope) | 8 | HIGH | 1 | Doxygen clarity |
| scope_mismatch (rankAll cache) | 6 | HIGH | 1 | Document cache ownership |
| iterator_invalidation (relationPhase) | 2 | CRITICAL | 3 | Verify iterator bounds |
| uninitialized_access (member vars) | 4 | HIGH | 2 | Add init guards |
| defensive_patterns (intentional) | 8 | LOW | 1 | Document as intentional |

**Tier 1–2 Tasks:**
- [ ] Add Doxygen `@param`, `@return`, `@throws` to entityEmbedding, rankAll, relationPhase
- [ ] Document cache semantics: "Returns independent copy; external modifications safe"
- [ ] Add null checks for internal member access
- [ ] Mark intentional defensive patterns with comments

**Tier 3–4 Tasks:**
- [ ] If iterator issues persist: refactor relationPhase to use range-based construction
- [ ] Add comprehensive cache unit tests

**Estimated Effort:** 12 hours (all tiers)

**Success Gate:** All 28 findings reviewed; 20+ resolved; 8 deferred with risk acceptance documented

---

### **File 2: `src/graph/explain_plan.cpp`**

**Current Status:** 2 CRITICAL gaps RESOLVED (Gap 2.2.1, 2.2.2)

**Remaining Findings (HIGH/MEDIUM):** ~18 findings

**Remediation Plan:**

| Finding Type | Count | Risk | Tier | Action |
|---|---|---|---|---|
| scope_mismatch (toDot/toJson scope) | 12 | HIGH | 1 | Doxygen updates |
| defensive_patterns (null checks) | 4 | MEDIUM | 2 | Add guards if missing |
| uninitialized_access | 2 | HIGH | 2 | Initialize defaults |

**Tier 1–2 Tasks:**
- [ ] Add Doxygen for toDot, toJson (defensive guard rationale)
- [ ] Verify null checks present before string operations
- [ ] Ensure default initializations for local variables

**Estimated Effort:** 6 hours

**Success Gate:** All 18 findings reviewed; 16+ resolved

---

### **File 3: `src/graph/path_constraints.cpp`**

**Current Status:** 1 CRITICAL gap RESOLVED (Gap 2.2.3 ErrorRegistry exhaustiveness)

**Remaining Findings (HIGH/MEDIUM):** ~12 findings

**Remediation Plan:**

| Finding Type | Count | Risk | Tier | Action |
|---|---|---|---|---|
| scope_mismatch (constraint scope) | 7 | HIGH | 1 | Doxygen clarity |
| defensive_patterns (ErrorRegistry) | 3 | LOW | 1 | Document as intentional |
| uninitialized_access | 2 | MEDIUM | 2 | Add guards |

**Tier 1–2 Tasks:**
- [ ] Add Doxygen for constraint API (scope, lifetime)
- [ ] Verify ErrorRegistry switch completeness (compiler check: `-Werror=switch`)
- [ ] Add initialization checks for constraint structures

**Estimated Effort:** 5 hours

**Success Gate:** All 12 findings reviewed; 10+ resolved

---

### **File 4: `src/graph/ontology_manager.cpp`**

**Current Status:** 1 CRITICAL gap RESOLVED (Gap 2.3.1 YamlEntry RAII)

**Remaining Findings (HIGH/MEDIUM):** ~21 findings

**Remediation Plan:**

| Finding Type | Count | Risk | Tier | Action |
|---|---|---|---|---|
| scope_mismatch (YamlEntry scope) | 10 | HIGH | 1 | Doxygen clarity |
| iterator_invalidation | 3 | CRITICAL | 3 | Code review + refactor |
| uninitialized_access | 5 | HIGH | 2 | Init guards |
| defensive_patterns (cache) | 3 | MEDIUM | 1 | Document |

**Tier 1–2 Tasks:**
- [ ] Add Doxygen for YamlEntry RAII semantics
- [ ] Document move constructor/assignment
- [ ] Add null/uninitialized checks
- [ ] Document cache ownership

**Tier 3–4 Tasks:**
- [ ] Review iterator invalidation patterns in YAML parsing
- [ ] Add bounds checking for cache access

**Estimated Effort:** 15 hours (tiers 1–4)

**Success Gate:** All 21 findings reviewed; 16+ resolved; complex items documented

---

## Parallel Work Stream Coordination

### **Stream 1: Documentation & Doxygen (Tier 1)**
- **Lead:** Senior developer or tech writer
- **Scope:** Add/update Doxygen comments across 4 files
- **Timeline:** Days 1–3, Week 2
- **Deliverable:** Doxygen documentation complete
- **Verification:** `doxygen Doxyfile` runs without warnings

### **Stream 2: Defensive Guards (Tier 2)**
- **Lead:** Mid-level developer
- **Scope:** Add null checks, empty guards, initialization
- **Timeline:** Days 2–4, Week 2
- **Deliverable:** All Tier 2 code fixes integrated
- **Verification:** Builds clean; tests pass

### **Stream 3: Complex Refactoring (Tier 3–4)**
- **Lead:** Senior architect
- **Scope:** Iterator invalidation, cache consistency
- **Timeline:** Days 3–5, Week 2 (if time permits)
- **Deliverable:** High-risk findings addressed
- **Verification:** Unit tests added; performance validated

### **Synchronization Points:**
- **Day 1 EOD:** Finding categorization complete; work streams assigned
- **Day 3 EOD:** Tier 1–2 fixes in code review
- **Day 4 EOD:** Integration testing underway
- **Day 5 EOD:** All changes merged; release candidate ready

---

## Risk Assessment & Mitigation

### **High-Impact Risks**

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| **Regression in 326-test suite** | Medium | Critical | Run baseline + post-fix tests; version control all changes |
| **Iterator invalidation persists** | Low | Critical | Code review + added unit tests for iterator patterns |
| **Cache inconsistency discovered** | Low | High | Add cache verification tests; document ownership model |
| **Scope mismatches still present** | Medium | Medium | Doxygen enforcement in code review; static analysis revalidation |

### **Mitigation Strategies**

1. **Pre-Integration Review:** Every Tier 2–4 fix requires peer code review
2. **Automated Testing:** Run 326-test suite after each major change
3. **Static Analysis Rerun:** Execute gap scanner after fixes to verify improvement
4. **Documentation Audit:** Verify Doxygen completeness for all modified APIs
5. **Stability Runs:** 100× iteration test before release candidate sign-off

---

## Success Gates & Verification Points

### **Gate 1: Baseline Establishment (End of Day 2, Week 1)**
- ✅ 326-test suite runs without timeout
- ✅ Baseline metrics captured
- ✅ No regressions from Phases 2.1–2.3

### **Gate 2: L1 Conformance Audit (End of Day 5, Week 1)**
- ✅ All 9 CRITICAL gaps verified RESOLVED
- ✅ Doxygen documentation present and complete
- ✅ No new warnings or regressions detected

### **Gate 3: Tier 1–2 Remediation Complete (End of Day 4, Week 2)**
- ✅ 45+ quick-win findings fixed
- ✅ Build succeeds; 326-test suite passes
- ✅ Code review sign-off on all changes

### **Gate 4: Integration Testing Pass (End of Day 5, Week 2)**
- ✅ No new test failures introduced
- ✅ Static analysis pass rate improved
- ✅ All Tier 1–2 fixes verified in context

### **Gate 5: Release Candidate Ready (Day 1, Week 3)**
- ✅ v2.4.0-rc1 tagged and packaged
- ✅ CHANGELOG updated
- ✅ Version artifacts consistent

### **Gate 6: Stability Validation (Day 6–7, Week 3)**
- ✅ 100× iteration runs with ≥ 99.5% pass rate
- ✅ No flakiness or resource leaks detected
- ✅ Release candidate signed off for promotion

---

## Acceptance Criteria Summary

| Criterion | Target | Verification Method |
|---|---|---|
| Test Pass Rate | ≥ 95% of 326 tests | `ctest --preset community-release` |
| Finding Remediation | ≥ 80% (85+ of 107) fixed | Finding categorization report |
| L1 Conformance | All 9 gaps RESOLVED | Audit report PASS |
| Build Warnings | ≤ 5 (baseline-aligned) | Build log analysis |
| Doxygen Coverage | 100% of modified APIs | `doxygen Doxyfile` |
| Stability Runs | ≥ 99.5% pass rate (100×) | Stability validation report |
| Release Artifacts | v2.4.0-rc1 tag created | Git tag verification |

---

## Dependencies & Resource Allocation

### **Team Composition (Estimated)**
- 1 × Senior Architect (30 hours): Oversight, complex fixes, release sign-off
- 2 × Senior Developers (40 hours each): Tier 3–4 refactoring, integration testing
- 1 × Mid-level Developer (30 hours): Tier 2 guards, documentation updates
- 1 × QA Engineer (20 hours): Test baseline, stability validation
- 1 × Tech Writer (10 hours): Release notes, documentation review

**Total:** ~170 person-hours over 2 weeks

### **Tools & Infrastructure**
- CMake (build automation)
- CTest (test execution)
- Doxygen (documentation generation)
- Static analysis tools (gap scanner, compiler warnings)
- Git (version control)
- GitHub Actions or CI/CD (automated testing)

---

## Deliverables Checklist

### **Week 1 Deliverables**
- [ ] `PHASE_2.4_TEST_BASELINE_CONFIG.md` (test environment setup)
- [ ] `PHASE_2.4_TEST_BASELINE_RESULTS.md` (baseline metrics)
- [ ] `PHASE_2.4_L1_CONFORMANCE_AUDIT_REPORT.md` (gap verification)
- [ ] Test execution log (full output)

### **Week 2 Deliverables**
- [ ] `PHASE_2.4_FINDING_CATEGORIZATION.json` (107 findings classified)
- [ ] `FINDING_PRIORITIZATION.md` (work stream breakdown)
- [ ] Code fixes committed (Tier 1–2, Tier 3–4 if time permits)
- [ ] Updated Doxygen documentation
- [ ] `PHASE_2.4_INTEGRATION_RESULTS.md` (test results post-fixes)
- [ ] `v2.4.0-rc1` release tag and package

### **Week 3 Deliverables (If Extended)**
- [ ] `PHASE_2.4_STABILITY_VALIDATION_REPORT.md` (100× runs)
- [ ] `PHASE_2_4_RELEASE_NOTES.md` (public release documentation)
- [ ] Version artifact updates (CMakeLists.txt, package.json, etc.)

---

## Next Steps

1. **Immediately (Day 1, Week 1):**
   - Schedule kickoff meeting with development team
   - Assign developers to parallel work streams
   - Set up CI/CD automation for test baseline

2. **By End of Week 1:**
   - Complete test baseline and L1 audit
   - Review findings categorization
   - Prepare Tier 1–2 task list for Week 2

3. **During Week 2:**
   - Execute parallel remediation streams
   - Conduct code reviews
   - Run integration tests

4. **By End of Week 2:**
   - Release candidate ready for stability testing
   - All Tier 1–2 fixes integrated
   - Remaining Tier 3–4 items documented for future phases

---

## Appendix: Static Analysis Finding Categories

### **Scope Mismatch (~95 findings)**
- Variable scope ambiguity in multi-threaded contexts
- Lock lifetime and acquisition point confusion
- Move semantics not clearly documented
- **Example:** entityEmbedding() lock scope (Gap 2.1.1)

### **Iterator Invalidation (~5 findings)**
- Dangling iterators after container modifications
- Temporary references in initializer lists
- **Example:** relationPhase() iterator constructor (Gap 2.1.2)

### **Uninitialized Access (~3 findings)**
- Local variables used before initialization
- Default constructor expectations unclear
- **Example:** Member variable initialization in YamlEntry

### **Defensive Patterns (~3 findings)**
- Intentional null checks causing false positives
- Fail-safe returns for empty states
- **Example:** toDot() empty state guard (Gap 2.2.1)

### **Other (~1 findings)**
- Miscellaneous findings requiring case-by-case review

---

**Document Status:** READY FOR EXECUTION  
**Last Updated:** 2026-07-01 09:30 UTC  
**Owner:** @makr-code  
**Reviewers:** [TBD]  

---

*This roadmap is a living document. Update as Phase 2.4 progresses. Report status changes immediately to stakeholders.*
