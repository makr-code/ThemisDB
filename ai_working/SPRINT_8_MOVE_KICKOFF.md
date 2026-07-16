# Sprint 8: Move Semantics & Moved-From State Remediation (97 gaps)

**Date:** 2026-07-05  
**Status:** 🟡 PLANNING & KICKOFF  
**Target:** v1.5.0 (2026-08-31)  
**Duration:** 2 weeks (estimated)  
**Gap Count:** 97 gaps (CWE-457, use-of-moved-from)  
**Completion Target:** 90+ gaps (92.8% completion)

---

## Executive Summary

Sprint 8 addresses **moved-from state violations** and **use-after-move** patterns in the codebase. These gaps involve:

1. **Use of moved-from objects** (CWE-457)
2. **Invalid state after std::move()** 
3. **Moved-from object member access**
4. **Double-move and nested-move patterns**

Key differences from Sprint 7 (Iterator):
- **Scope:** Object lifetime after move semantics (C++11+)
- **Pattern:** `T t; T u = std::move(t); use(t);` or `obj.member_access()` after move
- **Severity:** HIGH/CRITICAL (use-after-free class vulnerabilities)
- **Mitigation:** Add moved-from state checks, use SafeMoved wrapper, documentation

---

## Gap Categories (97 total)

### Category A: Direct Use-After-Move (35 gaps)
```cpp
// Pattern A1: Basic use after move
MyClass obj = SomeFunc();
MyClass obj2 = std::move(obj);
obj.method();  // MOVED-FROM USE
obj.member;    // MOVED-FROM ACCESS
```

- Location pattern: Same scope, consecutive operations
- Typical fix: Replace with `obj2` or restructure logic
- Risk: Logic error, undefined behavior
- Estimate: 35 gaps across core modules

### Category B: Moved-From in Containers (28 gaps)
```cpp
// Pattern B1: Move element, then use it
std::vector<T> v = {...};
T temp = std::move(v[0]);
use(v[0]);  // MOVED-FROM
v[0].reset();  // Should be: temp.reset() or omit
```

- Location pattern: Vector/container element access post-move
- Typical fix: Track which element was moved, or re-initialize
- Risk: Container invariant violation
- Estimate: 28 gaps

### Category C: Moved-From in Function Returns (18 gaps)
```cpp
// Pattern C1: Return moved object, use original
MyClass obj;
std::optional<MyClass> opt = GetOptional(std::move(obj));
obj.validate();  // MOVED-FROM
```

- Location pattern: Post-move function call with local var
- Typical fix: Restructure or use temporary
- Risk: Null/invalid reference
- Estimate: 18 gaps

### Category D: Moved-From in Error Paths (16 gaps)
```cpp
// Pattern D1: Conditional move, then use in error handler
if (condition) {
    T t2 = std::move(t);
}
t.cleanup();  // MOVED-FROM if condition true
```

- Location pattern: Try/catch, if/else with move branches
- Typical fix: Safe state check or restructure control flow
- Risk: Resource leak, UAF in error handlers
- Estimate: 16 gaps

---

## Top Affected Modules

| Module | Estimated Gaps | Hotspot Files | Priority |
|--------|----------------|---------------|----------|
| server | 18 | http_server.cpp, query_api_handler.cpp | P0 |
| llm | 16 | multi_lora_manager.cpp, inference_engine.cpp | P0 |
| query | 14 | query_engine.cpp, aql_translator.cpp | P1 |
| sharding | 12 | cross_shard_transaction.cpp, shard_router.cpp | P1 |
| index | 10 | secondary_index.cpp, vector_index.cpp | P1 |
| graph | 7 | path_constraints.cpp, explain_plan.cpp | P2 |
| core | 4 | Various utility patterns | P2 |

---

## Remediation Strategy

### Phase 1: Gap Identification & Categorization (2 days)

**Goal:** Scan, categorize, and document all 97 gaps

**Tasks:**
- [ ] Run gap scanner with moved-from pattern detection (tools/gap_scanner_v3.py)
- [ ] Generate detailed gap report with:
  - Gap ID, file, line number, pattern category
  - Code context (3-5 lines before/after)
  - Severity assessment (HIGH vs CRITICAL)
  - Suggested fix strategy
- [ ] Group gaps by module and category
- [ ] Create GitHub issues for tracking (if needed)

**Output:** ai_working/SPRINT_8_GAP_REPORT.md

### Phase 2: Remediation Strategy Design (2 days)

**Goal:** Define fix approaches and create remediation guide

**Tasks:**
- [ ] Design SafeMove wrapper class (if applicable)
  ```cpp
  template <typename T>
  class SafeMove {
      T value_;
      bool moved_ = false;
  public:
      T& operator*() { 
          if (moved_) throw std::runtime_error("moved-from access");
          return value_;
      }
  };
  ```
- [ ] Document fix patterns for each category (A-D)
- [ ] Create remediation guide with examples
- [ ] Prioritize gaps by difficulty (easy/medium/hard)

**Output:** ai_working/SPRINT_8_REMEDIATION_GUIDE.md

### Phase 3: Implementation (8 days)

**Goal:** Implement fixes across all modules

**Strategy:**
1. **Batch A (3 days):** Easy fixes (restructure, use moved var)
   - Target: 35-40 gaps
   - Modules: server, query (hotspots first)

2. **Batch B (3 days):** Medium fixes (container handling, error paths)
   - Target: 35-40 gaps
   - Modules: llm, sharding, index

3. **Batch C (2 days):** Hard fixes (complex control flow, refactoring)
   - Target: 17-22 gaps
   - Modules: graph, core

**Verification:** Each batch commits with test updates

### Phase 4: Testing & Verification (2 days)

**Goal:** Ensure fixes don't introduce regressions

**Tasks:**
- [ ] Run existing test suite (CTest full)
- [ ] Add move-semantics specific tests
- [ ] Verify no new moved-from violations
- [ ] Performance regression check (if applicable)

**Output:** ai_working/SPRINT_8_TEST_VERIFICATION.md

### Phase 5: Documentation & Finalization (1 day)

**Goal:** Document fixes and close Sprint 8

**Tasks:**
- [ ] Create Sprint 8 Completion Report
- [ ] Update relevant module documentation
- [ ] Document lessons learned
- [ ] Prepare transition to Sprint 9 (Concurrency)

**Output:** ai_working/SPRINT_8_COMPLETION_SUMMARY.md

---

## Risk Mitigation

### Risk 1: False Positives (moved-from detection)
- **Mitigation:** Manual review of categorized gaps before fixing
- **Review Level:** Spot-check 20% of Category A, 100% of Category D

### Risk 2: Regression from Fix
- **Mitigation:** Comprehensive test execution, spot-check fix sites
- **Test Coverage:** Existing unit + integration tests

### Risk 3: Incomplete Understanding of Move State
- **Mitigation:** Consult C++ standard, reference compiler move semantics docs
- **Expert Input:** C++ Language Service Tools (semantic symbol navigation)

---

## Success Criteria

### Quantitative
- ✓ 90+ of 97 gaps remediated (92.8%)
- ✓ 0 regressions in existing tests
- ✓ All fixes build successfully
- ✓ New moved-from violations not introduced

### Qualitative
- ✓ Fixes follow modern C++ best practices
- ✓ Code comments explain moved-from state handling
- ✓ Remediation guide is clear and reusable
- ✓ No breaking API changes

---

## Timeline

```
Week 1:
├─ Day 1-2: Phase 1 (Gap ID & categorization)
├─ Day 3-4: Phase 2 (Strategy & guide design)
└─ Day 5: Review & adjust plan

Week 2:
├─ Day 1-3: Phase 3A (Easy fixes, Batch A)
├─ Day 2-4: Phase 3B (Medium fixes, Batch B) [overlap]
├─ Day 5-6: Phase 3C (Hard fixes, Batch C)
├─ Day 7: Phase 4 (Testing & verification)
└─ Day 8: Phase 5 (Documentation & finalization)

Target Completion: 2026-07-19
```

---

## Deliverables

1. **SPRINT_8_GAP_REPORT.md** — Categorized gap list with contexts
2. **SPRINT_8_REMEDIATION_GUIDE.md** — Fix patterns and examples
3. **Code Changes** — 50-100 modified files across modules
4. **SPRINT_8_TEST_VERIFICATION.md** — Test results and validation
5. **SPRINT_8_COMPLETION_SUMMARY.md** — Final status and metrics

---

## Transition to Sprint 9

Sprint 9 will focus on **Concurrency** (20 gaps) related to:
- Data races (race conditions in shared access)
- Lost wakeup (condition variable issues)
- Double-checked locking patterns
- Unsynchronized container access

**Estimated Sprint 9 Duration:** 1 week (smaller scope: 20 gaps)

---

## Related Documents

- [PHASE_1_4_REMEDIATION_BATCHES.md](./PHASE_1_4_REMEDIATION_BATCHES.md)
- [PHASE_1_4_AND_6_EXECUTIVE_SUMMARY.md](./PHASE_1_4_AND_6_EXECUTIVE_SUMMARY.md)
- [SPRINT_7_FINAL_COMPLETION_REPORT.md](./SPRINT_7_FINAL_COMPLETION_REPORT.md)

