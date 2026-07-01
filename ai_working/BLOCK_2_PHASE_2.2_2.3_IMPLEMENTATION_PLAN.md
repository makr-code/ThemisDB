# Block 2: Phase 2.2-2.3 Combined Implementation Plan

**Status:** READY FOR EXECUTION  
**Timeline:** 2 weeks (2026-07-01 to 2026-07-15)  
**Target Completion:** Phase 2.2 + 2.3 (4 CRITICAL gaps + 5 integration tests)  
**Release Target:** v2.4 (2026-08-06)

---

## Scope Summary

### Phase 2.2: explain_plan.cpp + path_constraints.cpp
- **Gaps:** 2 CRITICAL (scope_mismatch @ lines 68, 92 in explain_plan.cpp)
- **Duration:** 1 week
- **Tests:** 8 explain_plan + 14 path_constraints = 22 tests
- **Success Criteria:** All 22 tests PASS; no new warnings

### Phase 2.3: ontology_manager.cpp  
- **Gaps:** 2 CRITICAL (missing_dtor @ line 192; uninitialized_access in path_constraints line 16)
- **Duration:** 1 week
- **Tests:** 12 ontology + 13 entity_type_constraints = 25 tests
- **Success Criteria:** All 25 tests PASS; no new warnings

### Combined Success Criteria
- ✅ All 4 CRITICAL gaps analyzed & documented
- ✅ All 47 tests PASS (22 phase 2.2 + 25 phase 2.3)
- ✅ CMake build succeeds
- ✅ No new static analysis warnings
- ✅ ROADMAP.md updated with Phase 2.2-2.3 completion

---

## Phase 2.2 Implementation

### File 1: explain_plan.cpp (146 lines)

#### Gap 2.2.1: scope_mismatch @ Line 68
**Type:** scope_mismatch (HIGH)  
**Location:** explain_plan.cpp:68 (toDot() method)  
**Current Pattern:** Empty plan detection and early return

```cpp
std::string GraphExplainPlan::toDot() const {
    if (nodes.empty()) {
        return {};  // <- Gap: empty return; semantics verified (defensive pattern)
    }
    // ... rest of implementation
}
```

**Analysis:**
- Gap marked as HIGH in scanner (scope_mismatch classification)
- Pattern verified in ROADMAP.md L0 analysis: "Empty plan → empty DOT (correct semantics)"
- This is a GUARDED_STUB pattern (defensive guard against invalid state)
- **Fix Strategy:** Add documentation comment explaining defensive guard

**Implementation:**
1. Add clear Doxygen comment explaining the guard
2. Verify return semantics (empty DOT vs NULL DOT)
3. Test: Verify KGC-EXPLAIN-01 handles empty plan correctly

#### Gap 2.2.2: scope_mismatch @ Line 92
**Type:** scope_mismatch (HIGH)  
**Location:** explain_plan.cpp:92 (toJson() method)  
**Current Pattern:** Empty plan detection and early return

```cpp
std::string GraphExplainPlan::toJson() const {
    if (nodes.empty()) {
        return {};  // <- Gap: empty return; semantics verified (defensive pattern)
    }
    // ... rest of implementation
}
```

**Analysis:**
- Gap marked as HIGH in scanner (scope_mismatch classification)
- Pattern verified in ROADMAP.md L0 analysis: "Empty plan → empty JSON (correct semantics)"
- This is a GUARDED_STUB pattern (defensive guard against invalid state)
- **Fix Strategy:** Add documentation comment explaining defensive guard

**Implementation:**
1. Add clear Doxygen comment explaining the guard
2. Verify return semantics (empty JSON vs NULL JSON)
3. Test: Verify KGC-EXPLAIN-02 handles empty plan correctly

### File 2: path_constraints.cpp (753 lines)

#### Gap 2.2.3: uninitialized_access @ Line 16
**Type:** uninitialized_access (HIGH)  
**Location:** path_constraints.cpp:16 (in ErrorRegistry namespace)  
**Current Pattern:** Static error code mapping

```cpp
namespace {
struct ErrorRegistry {
    enum class ErrorCode { VALIDATION_FAILED, INVALID_STATE, NOT_FOUND };
};

inline errors::ErrorCode mapErrorCode(ErrorRegistry::ErrorCode code) {
    switch (code) {
        case ErrorRegistry::ErrorCode::VALIDATION_FAILED:
            return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
        // ... more cases
    }
    return errors::ErrorCode::ERR_UNKNOWN;
}
}
```

**Analysis:**
- Gap flagged as uninitialized_access at line 16 context
- This refers to the static ErrorRegistry struct initialization
- The switch statement ensures all code paths return a valid ErrorCode
- **Fix Strategy:** Add explicit initialization check and document contract

**Implementation:**
1. Verify all ErrorRegistry::ErrorCode cases are handled in switch
2. Add static_assert to verify enum completeness
3. Test: Verify error handling for all constraint violation scenarios

---

## Phase 2.3 Implementation

### File 1: ontology_manager.cpp (703 lines)

#### Gap 2.3.1: missing_dtor @ Line 192 (CRITICAL)
**Type:** missing_dtor (CRITICAL)  
**Location:** ontology_manager.cpp:192  
**Current Pattern:** YAML parser YamlEntry struct

```cpp
struct YamlEntry {
    std::unordered_map<std::string, std::string> scalar;
    std::unordered_map<std::string, std::vector<std::string>> list;
};
```

**Analysis:**
- Gap flagged as CRITICAL missing destructor
- YamlEntry contains STL containers (unordered_map) with automatic destructors
- Scanner may be flagging potential resource cleanup in parent context
- **Fix Strategy:** Verify resource cleanup semantics in context (likely false positive or defensive hardening)

**Implementation:**
1. Check caller context for YamlEntry usage patterns
2. Add explicit destructor if cleanup needed (likely not required for STL containers)
3. Add documentation comment about RAII semantics
4. Test: Verify ontology parsing and cleanup for large YAML inputs

#### Gap 2.3.2: uninitialized_access in path_constraints @ Line 16 (SECONDARY)
This gap overlaps with Phase 2.2.3 (above).
- Consolidated with Phase 2.2 fixes
- Test: Path constraint initialization across both files

---

## Test Coverage & Validation

### Phase 2.2 Tests (22 total)
**explain_plan.cpp Tests (8):**
- KGC-EXPLAIN-01: toDot() with empty plan
- KGC-EXPLAIN-02: toJson() with empty plan
- KGC-EXPLAIN-03: toDot() with single node
- KGC-EXPLAIN-04: toJson() with single node
- KGC-EXPLAIN-05: toDot() with multiple nodes and edges
- KGC-EXPLAIN-06: toJson() with multiple nodes and edges
- KGC-EXPLAIN-07: escapeJson() special characters
- KGC-EXPLAIN-08: nodeTypeToString() all node types

**path_constraints.cpp Tests (14):**
- KGC-PATH-01: PathConstraints initialization
- KGC-PATH-02: addMinLength() constraint
- KGC-PATH-03: addMaxLength() constraint
- KGC-PATH-04: addForbiddenNode() constraint
- KGC-PATH-05: Error handling (invalid identifier)
- KGC-PATH-06: Error handling (invalid field name)
- KGC-PATH-07: Multiple constraints combined
- KGC-PATH-08: setGraphManager() state update
- KGC-PATH-09: Validation with empty constraints
- KGC-PATH-10: Validation with full constraint set
- KGC-PATH-11: Error registry code mapping
- KGC-PATH-12: Constraint serialization
- KGC-PATH-13: Constraint deserialization
- KGC-PATH-14: Performance: 1000+ constraints

### Phase 2.3 Tests (25 total)
**ontology_manager.cpp Tests (12):**
- KGC-ONT-01: OntologyManager initialization
- KGC-ONT-02: parseString() with valid JSON
- KGC-ONT-03: parseString() with parse error
- KGC-ONT-04: parseString() with empty string
- KGC-ONT-05: addConcept() semantics
- KGC-ONT-06: addRelation() semantics
- KGC-ONT-07: YamlEntry construction and cleanup
- KGC-ONT-08: Resource cleanup (RAII verification)
- KGC-ONT-09: Large ontology (1000+ concepts)
- KGC-ONT-10: Nested concept hierarchy
- KGC-ONT-11: Axiom validation
- KGC-ONT-12: Error handling edge cases

**entity_type_constraints.cpp Tests (13):**
- KGC-ETC-01: EntityTypeConstraints initialization
- KGC-ETC-02: Type constraint enforcement
- KGC-ETC-03: Inheritance hierarchy
- KGC-ETC-04: Constraint validation
- KGC-ETC-05: Type mismatch detection
- KGC-ETC-06: Multi-inheritance scenarios
- KGC-ETC-07: Constraint combination
- KGC-ETC-08: Error propagation
- KGC-ETC-09: Resource cleanup (RAII verification)
- KGC-ETC-10: Performance: type lookup O(1)
- KGC-ETC-11: Serialization/deserialization
- KGC-ETC-12: Large type hierarchy (1000+ types)
- KGC-ETC-13: Edge case: circular inheritance detection

---

## Build & Test Commands

```bash
# Configure
cmake --preset community-release

# Build graph module
cmake --build --preset community-release --target themis_graph --parallel 16

# Phase 2.2 Tests
ctest --preset community-release -R "test_explain_plan" --output-on-failure
ctest --preset community-release -R "test_path_constraints" --output-on-failure

# Phase 2.3 Tests
ctest --preset community-release -R "test_ontology" --output-on-failure
ctest --preset community-release -R "test_entity_type_constraints" --output-on-failure

# All Phase 2.2-2.3 Tests
ctest --preset community-release -R "(explain_plan|path_constraints|ontology|entity_type)" --output-on-failure

# Full integration sanity check
ctest --preset community-release -R "test_graph" --output-on-failure
```

---

## Implementation Workflow

### Per-Phase Execution
1. **Gap Analysis:** Verify gap locations and semantics
2. **Fix Implementation:** Apply minimal focused fixes
3. **Local Testing:** Run phase-specific test suite
4. **Integration Test:** Run full graph suite
5. **Code Review:** Semantic validation
6. **Commit & Sign-Off:** Update ROADMAP.md phase markers

### Commits (Expected)
- `Phase 2.2.1 fix: Add defensive guard documentation to explain_plan::toDot()`
- `Phase 2.2.2 fix: Add defensive guard documentation to explain_plan::toJson()`
- `Phase 2.2.3 fix: Add ErrorRegistry initialization verification and static_assert`
- `Phase 2.3.1 fix: Add RAII documentation and resource cleanup verification to ontology_manager::YamlEntry`
- `Update ROADMAP.md: Phase 2.2-2.3 completion markers`

---

## Success Criteria Checklist

### Phase 2.2
- [ ] Gap 2.2.1 analyzed & documented
- [ ] Gap 2.2.2 analyzed & documented
- [ ] Gap 2.2.3 analyzed & documented
- [ ] explain_plan.cpp toDot() has defensive guard documentation
- [ ] explain_plan.cpp toJson() has defensive guard documentation
- [ ] path_constraints.cpp ErrorRegistry has initialization verification
- [ ] All 8 explain_plan tests PASS
- [ ] All 14 path_constraints tests PASS
- [ ] CMake build succeeds
- [ ] No new static analysis warnings

### Phase 2.3
- [ ] Gap 2.3.1 analyzed & documented
- [ ] ontology_manager.cpp YamlEntry has RAII documentation
- [ ] All 12 ontology tests PASS
- [ ] All 13 entity_type_constraints tests PASS
- [ ] Resource cleanup verified (RAII patterns)
- [ ] CMake build succeeds
- [ ] No new static analysis warnings

### Overall
- [ ] All 47 tests PASS (22 + 25)
- [ ] Full graph module integration tests PASS
- [ ] ROADMAP.md updated: Phase 2.2-2.3 [x] COMPLETE
- [ ] Ready for Phase 2.4 (integration & hardening)

---

## Risk Mitigation

### Known Risks
1. **Gap Classification:** Defensive patterns may be incorrectly classified as bugs
2. **RAII Semantics:** STL container destructors are implicit; documentation may be needed
3. **Error Handling:** ErrorRegistry must maintain exhaustive switch coverage

### Mitigation Strategies
1. Verify each gap against L0 analysis (source-verifiable semantics)
2. Add explicit RAII documentation to complex resource handling
3. Use static_assert for enum/switch completeness checks
4. Run full integration tests to ensure no regressions

---

## Sign-Off Gate

```
✅ Phase 2.2-2.3 COMPLETE
├─ 4/4 CRITICAL gaps RESOLVED
├─ 47/47 tests PASS
├─ No new warnings
├─ ROADMAP.md updated
└─ Ready for Phase 2.4 kickoff
```

---

**Generated:** 2026-07-01  
**Updated:** [During execution]  
**Target Release:** Q3 2026 (v2.4)  
**Next Step:** Phase 2.2-2.3 implementation and validation
