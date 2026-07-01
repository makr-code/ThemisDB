# Block 2: Graph Module Phase 2.1-2.4 Implementation Plan

**Status:** KICKOFF — Phase 2.1 (rotate_completion.cpp)  
**Timeline:** 6 weeks (2026-07-01 to 2026-08-06)  
**Target:** v2.4 release with Graph Module completion  
**Owner Assignment:** REQUIRED before implementation

---

## Executive Overview

### Block 2 Scope (6 Weeks)
| Phase | File(s) | Gaps | Duration | Tests | Gate |
|-------|---------|------|----------|-------|------|
| **2.1** | rotate_completion.cpp | 3 CRITICAL | 1 week | 9 tests | Phase 2.1 PASS |
| **2.2** | explain_plan.cpp, path_constraints.cpp | 4 CRITICAL | 1 week | 14+8 tests | Phase 2.2 PASS |
| **2.3** | ontology_manager.cpp | 2 CRITICAL | 1 week | 12+13 tests | Phase 2.3 PASS |
| **2.3-2.4** | Full Integration & Hardening | 107 findings (19 CRITICAL, 88 HIGH) | 2 weeks | 326 full suite | Phase 2.4 PASS |
| **Parallel** | Distributed TX Recovery Refactoring | Design/planning | Ongoing | Per roadmap | Async |

### Success Criteria
- ✅ All 9 CRITICAL gaps RESOLVED (documented in MODULE_GAPS.md)
- ✅ All 326 graph tests PASS (ctest)
- ✅ 9 rotating_completion tests PASS (Phase 2.1 gate)
- ✅ 14 path_constraints + 8 explain_plan tests PASS (Phase 2.2 gate)
- ✅ 12 ontology + 13 entity_type_constraints tests PASS (Phase 2.3 gate)
- ✅ L1 conformance audit re-run: 4/4 PASS
- ✅ No new static analysis CRITICAL findings

---

## Phase 2.1: rotate_completion.cpp (Week 1)

### Gap Overview

**File:** `src/graph/rotate_completion.cpp` (529 lines)  
**Gaps:** 3 CRITICAL (scope_mismatch @ lines 95, 109; iterator_invalidation related)  
**Impact:** Multi-plane queries cannot complete; all complex query plans fail

### CRITICAL Gaps Identified

#### Gap 2.1.1: Scope Mismatch @ Line 95
**Type:** scope_mismatch (HIGH severity)  
**Pattern:** Variable declared in inner scope; used outside  
**Location:** `rotate_completion.cpp:95`  
**Current Behavior:** Rotation logic incomplete; predicates lost after scope exit  
**Fix Required:** Lift variable to outer scope or return via value

#### Gap 2.1.2: Scope Mismatch @ Line 109
**Type:** scope_mismatch (HIGH severity)  
**Pattern:** Loop iterator outlives loop scope  
**Location:** `rotate_completion.cpp:109`  
**Current Behavior:** Rotation result references invalid iterator  
**Fix Required:** Materialize results before loop end

#### Gap 2.1.3: Iterator Invalidation (related to graph_query_optimizer.cpp)
**Type:** iterator_invalidation (CRITICAL)  
**Pattern:** Cache invalidation during rotation traversal  
**Location:** References from graph_query_optimizer.cpp:1367, 2081  
**Current Behavior:** Query cache corrupted after rotation completion  
**Fix Required:** Ensure cache consistency during multi-plane completion

### Implementation Tasks

#### Task 2.1.1: Fix Scope Mismatch @ Line 95
**Subtask A:** Identify the predicate/rotation structure being declared  
**Subtask B:** Lift variable to function scope or return from loop  
**Subtask C:** Ensure semantic validity (type safety for rotated predicates)  
**Acceptance:** Gap 2.1.1 resolved; code compiles; test KGC-12 PASS  

#### Task 2.1.2: Fix Scope Mismatch @ Line 109
**Subtask A:** Identify the result container requiring lifetime extension  
**Subtask B:** Materialize or copy results before loop end  
**Subtask C:** Add bounds checking for rotated plane indices  
**Acceptance:** Gap 2.1.2 resolved; test KGC-13 PASS

#### Task 2.1.3: Ensure Iterator Cache Consistency
**Subtask A:** Add cache invalidation guard during rotation completion  
**Subtask B:** Verify cache lookups don't reference stale plane indices  
**Subtask C:** Add integration test for multi-plane query with cache  
**Acceptance:** Gap 2.1.3 resolved; integration test PASS

### Test Coverage (9 Tests)

| Test ID | Name | Purpose | Expected Outcome |
|---------|------|---------|------------------|
| KGC-12 | PredictTailSorted | Verify tail completion via rotation | ✅ PASS |
| KGC-13 | PredictHeadSorted | Verify head completion via rotation | ✅ PASS |
| KGC-14 | ReasonerInjection | KGCompletionEngine injects predictions | ✅ PASS |
| KGC-15 | CompleteHeadDelegates | Delegation to LinkPredictionHead | ✅ PASS |
| KGC-16 | EpochCountInfluencesScore | Training convergence validation | ✅ PASS |
| MULTI-01 | Multi-PlaneRotation | Complex 3-plane query completion | ✅ PASS (new) |
| MULTI-02 | CacheConsistency | Cache survives multi-plane rotation | ✅ PASS (new) |
| PERF-01 | RotationThroughput | Completion speed >= 1000 triples/sec | ✅ PASS (new) |
| PERF-02 | ScalabilityN | Completion time O(N·dim) not O(N²) | ✅ PASS (new) |

### Implementation Pattern

```cpp
// Before (Gap 2.1.1 & 2.1.2)
for (const auto& plane : query_planes) {
    std::vector<Predicate> rotated_preds;  // <- Gap: declared here
    for (const auto& pred : plane) {
        rotated_preds.push_back(rotate(pred, plane.rotation));
    }  // <- rotated_preds scope ends here
    results.push_back(rotated_preds);  // <- Uses out-of-scope var
}

// After (Fixed)
std::vector<std::vector<Predicate>> all_rotated;  // <- Lifted to function scope
for (const auto& plane : query_planes) {
    std::vector<Predicate> rotated_preds;
    for (const auto& pred : plane) {
        rotated_preds.push_back(rotate(pred, plane.rotation));
    }
    all_rotated.push_back(std::move(rotated_preds));  // <- Materialize before exit
}
```

### Verification Checklist

- [ ] Identify exact location and context of Gap 2.1.1 (line 95)
- [ ] Identify exact location and context of Gap 2.1.2 (line 109)
- [ ] Implement fix for scope mismatch @ line 95
- [ ] Implement fix for scope mismatch @ line 109
- [ ] Add cache invalidation guard for iterator safety
- [ ] CMake build passes: `cmake --preset community-release`
- [ ] All 9 rotate_completion tests PASS: `ctest -R test_rotate_completion --output-on-failure`
- [ ] Integration test MULTI-01 PASS
- [ ] Integration test MULTI-02 PASS
- [ ] Performance test PERF-01 PASS (threshold: 1000 triples/sec)
- [ ] No new static analysis warnings
- [ ] Code review: Semantic validation coverage 100%

---

## Phase 2.2: Explain Plan & Path Constraints (Weeks 2-3)

**Files:** explain_plan.cpp (2 CRITICAL gaps), path_constraints.cpp (2 gaps)  
**Tests:** 8 explain_plan + 14 path_constraints  
**Gate:** Phase 2.2 PASS (cost estimates within 10% of actual)

### Tentative Gaps
- explain_plan.cpp:68, 92 (scope_mismatch)
- path_constraints.cpp (uninitialized_access + iterator_invalidation)

---

## Phase 2.3: Ontology Manager (Week 3)

**File:** ontology_manager.cpp (2 CRITICAL gaps)  
**Tests:** 12 ontology + 13 entity_type_constraints  
**Gate:** Phase 2.3 PASS

### Tentative Gaps
- ontology_manager.cpp:192 (missing_dtor CRITICAL)

---

## Phase 2.3-2.4: Integration & Hardening (Weeks 4-6)

**Scope:** 107 actionable findings (19 CRITICAL + 88 HIGH)  
**Tests:** 326 full graph test suite  
**Verification:** 100x stability runs

---

## Execution Workflow

### Per-Phase Workflow
1. **Planning:** Review gap specifications in MODULE_GAPS.md
2. **Implementation:** Fix gaps in order (CRITICAL first)
3. **Local Testing:** Run focused test suite for that file
4. **Integration Test:** Run full graph suite
5. **Acceptance:** All tests PASS + no new warnings
6. **Sign-Off:** Update ROADMAP.md phase completion marker

### Build & Test Commands

```bash
# Phase 2.1 build & test
cmake --preset community-release
cmake --build --preset community-release --target themis_graph --parallel 16
ctest --preset community-release -R test_rotate_completion --output-on-failure

# Phase 2.1 integration
ctest --preset community-release -R "test_graph" --output-on-failure

# Full suite (Phase 2.4 final gate)
ctest --preset community-release --output-on-failure --timeout 300
```

---

## Owner Assignment (REQUIRED)

### Phase Owners (assign before kickoff)

| Phase | Primary Owner | Secondary | Contacts |
|-------|---------------|-----------|----------|
| 2.1 (rotate_completion) | [ASSIGN] | [ASSIGN] | @makr-code |
| 2.2 (explain/constraints) | [ASSIGN] | [ASSIGN] | @makr-code |
| 2.3 (ontology) | [ASSIGN] | [ASSIGN] | @makr-code |
| 2.4 (integration) | [ASSIGN] | [ASSIGN] | @makr-code |

### Prerequisite Knowledge
- C++20 (modern templates, auto, RAII)
- Graph algorithms (traversal, query optimization)
- ThemisDB logging (THEMIS_* macros)
- GTest framework

---

## Progress Tracking

### Phase 2.1 Milestone Targets
- **Day 1:** Gap analysis complete; implementation plan finalized
- **Day 2-3:** Implement fixes for Gap 2.1.1 & 2.1.2
- **Day 4-5:** Implement cache consistency guard (Gap 2.1.3)
- **Day 6:** Testing & verification
- **Day 7:** Final sign-off & merge to develop

### Sign-Off Gate
```
✅ Phase 2.1 COMPLETE
├─ 3/3 CRITICAL gaps RESOLVED
├─ 9/9 tests PASS
├─ No new warnings
├─ ROADMAP.md updated
└─ Ready for Phase 2.2 kickoff
```

---

## Risk Mitigation

### Known Risks
1. **Scope Mismatch Complexity:** Multiple nested loops; ensure all lifetimes correct
2. **Cache Invalidation:** Multi-plane queries may corrupt query optimizer cache
3. **Iterator Safety:** STL iterators invalidated by vector reallocation

### Mitigation Strategies
1. Add lifetime diagram in comments (scope diagram)
2. Use RAII-based cache guards (scope_guard pattern)
3. Materialize results before container mutation

---

## Success Definition

**Block 2 is COMPLETE when:**
- All 9 CRITICAL gaps are RESOLVED
- All 326 graph tests PASS
- ROADMAP.md Phase 2.4 marked [x] COMPLETE
- L1 conformance audit 4/4 PASS
- Ready for v2.4 release candidate

---

**Generated:** 2026-07-01  
**Updated:** [Ongoing as phases complete]  
**Target Release:** Q3 2026 (v2.4)  
**Next Step:** Phase 2.1 kickoff - rotate_completion.cpp implementation
