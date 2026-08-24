# Sprint 8: Move Semantics Remediation - Kickoff Plan

**Status:** 🚀 In Progress  
**Date:** 2026-07-05  
**Target Completion:** 2026-07-26 (3 weeks)  
**Scope:** 97 move semantics gaps remediation  
**Release:** v1.5.0 (2026-08-31)

---

## 1. Gap Analysis Summary

### Overview
- **Total Gaps:** 97 move semantics-related violations
- **Severity:** CRITICAL (Undefined behavior, use-after-move, double-free)
- **Categories:**
  - Type A: Implicit move semantics missing (45-50 gaps)
  - Type B: Move constructor/assignment issues (30-35 gaps)
  - Type C: Complex move scenarios & move ordering (15-20 gaps)

### Scanner Source
- **From:** Phase 1-4 Gap Remediation Initiative (Sprint 8 batch)
- **Detection:** CWE-415 (Double Free), CWE-416 (Use After Free), CWE-763 (Object Lifecycle)
- **Baseline:** 1,236 → 1,119 gaps (Sprints 5-7 complete), 97 remaining

### Affected Modules (Priority Order)
1. **llm** - Move-intensive model loading/caching
2. **sharding** - Cross-shard transaction coordinator move operations
3. **storage** - RocksDB wrapper move semantics
4. **query** - Query plan node move chains
5. **replication** - WAL entry move semantics
6. **tensor** - Tensor data move operations
7. **acceleration** - GPU kernel wrapper moves
8. **network** - Message buffer move chains
9. Others (smaller counts, distributed across 20+ modules)

---

## 2. Approach & Deliverables

### 2.1 SafeMove Library Design

**Location:** `include/security/safe_move.h` + `src/security/safe_move.cpp`

**Components:**

```cpp
namespace themis::security {

// Core safety wrapper
template<typename T>
class SafeMove {
  // Ensures move source is cleared after move
  // Validates move construction/assignment
  // Detects use-after-move violations
};

// Move operation validator
struct MoveValidator {
  // Pre-move validation: object state check
  // Post-move validation: source cleanup verification
};

// Move source guard (RAII)
template<typename T>
class MoveGuard {
  // Wraps T* source pointer
  // Verifies source is not used after move
  // Automatic cleanup on destruction
};

// Move chain tracker
struct MoveChainTracker {
  // Tracks multi-hop move operations
  // Detects circular moves, premature destruction
};

} // namespace themis::security
```

**Reference:** `include/security/safe_iterator.h` (510 lines, 44 tests)

---

### 2.2 Remediation Pattern Templates

#### Pattern A: Missing Move Constructor/Assignment

**Before:**
```cpp
class DataBuffer {
  std::vector<uint8_t> data;
  // No move semantics defined - implicit fallback to copy
};
```

**After:**
```cpp
class DataBuffer {
  std::vector<uint8_t> data;

  DataBuffer(DataBuffer&& other) noexcept 
    : data(std::move(other.data)) {
    // SafeMove validation
  }

  DataBuffer& operator=(DataBuffer&& other) noexcept {
    if (this != &other) {
      data = std::move(other.data);
      // Validation that other is cleared
    }
    return *this;
  }

  // Delete copy to enforce move-only semantics where appropriate
};
```

#### Pattern B: Move Source State Not Cleared

**Before:**
```cpp
QueryPlan extractPlan(QueryPlan&& plan) {
  // plan is moved but may still be in partial state
  return std::move(plan); 
}
```

**After:**
```cpp
QueryPlan extractPlan(QueryPlan&& plan) {
  auto result = std::move(plan);
  plan = QueryPlan(); // Ensure source is in valid state
  return result;
}
// Or use SafeMove<QueryPlan> for automatic validation
```

#### Pattern C: Complex Move Chains

**Before:**
```cpp
struct TransactionContext {
  std::vector<WriteOperation> ops;
  // Moved in multiple places without clear ownership
};
```

**After:**
```cpp
struct TransactionContext {
  std::vector<WriteOperation> ops;

  // Explicit move with validation
  std::vector<WriteOperation> extractOperations() && {
    auto result = std::move(ops);
    THEMIS_MOVE_GUARD(result); // Ensure valid state
    return result;
  }
};
```

---

### 2.3 Deliverables Timeline

| Phase | Deliverable | Timeline | Status |
|-------|-------------|----------|--------|
| **Phase 1** | SafeMove library header + implementation | Week 1 (2026-07-05 - 2026-07-11) | 🚀 Start |
| **Phase 1** | 40+ SafeMove unit tests | Week 1 | 🚀 Start |
| **Phase 2** | Type A batch remediation (45-50 gaps) | Week 1-2 (2026-07-12 - 2026-07-18) | ⏳ Pending |
| **Phase 2** | Type B batch remediation (30-35 gaps) | Week 2 | ⏳ Pending |
| **Phase 3** | Type C batch remediation (15-20 gaps) | Week 2-3 (2026-07-19 - 2026-07-25) | ⏳ Pending |
| **Phase 4** | Integration + regression test suite | Week 3 | ⏳ Pending |
| **Phase 5** | Documentation + CHANGELOG + merge | 2026-07-26 | ⏳ Pending |

---

## 3. Quality Acceptance Criteria

✅ **Code Quality:**
- [ ] All 97 gaps remediated
- [ ] SafeMove library: 40+ tests, 100% pass rate
- [ ] Zero new compilation warnings
- [ ] Zero new security issues

✅ **Testing:**
- [ ] Unit tests for each remediated gap
- [ ] Integration tests for move chains
- [ ] Backward compatibility regression tests
- [ ] Move semantics correctness validation

✅ **Documentation:**
- [ ] SafeMove API documentation (Doxygen)
- [ ] Move remediation guide (10+ patterns)
- [ ] Integration examples (3-5 real-world cases)
- [ ] CHANGELOG entry

✅ **Performance:**
- [ ] Move operations >= 5% faster than copy baseline
- [ ] No additional memory overhead (SafeMove is zero-cost)
- [ ] No performance regression in existing code

---

## 4. Implementation Strategy

### Phase 1: Library Development (Week 1)

1. **Design & Header:**
   - Create `include/security/safe_move.h` (300-400 lines)
   - Document move safety patterns and contracts
   - Define MoveValidator, MoveGuard, SafeMove wrapper

2. **Implementation:**
   - Implement `src/security/safe_move.cpp` (200-250 lines)
   - Move chain tracking and validation logic
   - Integration with existing security infrastructure

3. **Testing:**
   - Create `tests/security/test_safe_move.cpp` (600-800 lines)
   - 40+ tests covering all move patterns
   - Correctness and safety validation

### Phase 2-3: Remediation (Weeks 2-3)

**Batch A (Type A - Implicit Moves):**
- Affected modules: llm, tensor, acceleration
- Pattern: Add move constructors/assignments
- Effort: ~2-3 gaps per file, 15-20 files

**Batch B (Type B - Constructor Issues):**
- Affected modules: sharding, storage, query
- Pattern: Fix move constructor implementation
- Effort: ~1-2 gaps per file, 15-20 files

**Batch C (Type C - Complex Scenarios):**
- Affected modules: replication, network, others
- Pattern: Move chain tracking, state validation
- Effort: ~1-2 gaps per file, 10-15 files

### Phase 4: Integration & Testing (Week 3)

- Unit test suite for each remediated location
- Integration tests for cross-module move chains
- Regression test suite (existing functionality preserved)
- Performance validation

### Phase 5: Documentation & Release (End of Week 3)

- SafeMove usage guide
- Pattern reference documentation
- CHANGELOG update for v1.5.0
- Create PR and prepare for merge to develop

---

## 5. Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| **Move semantics complexity** | Reference SafeIterator pattern; start with simple cases |
| **Binary size increase** | SafeMove library designed as zero-cost abstraction (templates/inlining) |
| **Performance regression** | Comprehensive benchmarking; move operations must be faster than copy |
| **Integration issues** | Phase gap with existing code; test backward compatibility |
| **Test coverage gaps** | 100+ tests covering all patterns; use property-based testing where applicable |

---

## 6. Success Metrics

- ✅ All 97 move semantics gaps remediated
- ✅ SafeMove library: 40+ tests, 100% pass rate
- ✅ Zero regression in existing tests
- ✅ Move operations 5-10% faster than copy baseline
- ✅ Production-ready code for v1.5.0 release
- ✅ Complete documentation and usage guide

---

## References

- **SafeIterator Pattern:** `include/security/safe_iterator.h` (510 lines, 44 tests)
- **Sprint 7 Completion:** `ai_working/SPRINT_7_FINAL_COMPLETION_REPORT.md`
- **Phase 1-4 Summary:** `ai_working/PHASE_1_4_REMEDIATION_BATCHES.md`
- **Related Issues:** #5195-#5201 (Module gap remediation tracking)
- **Release Target:** v1.5.0 (2026-08-31)
