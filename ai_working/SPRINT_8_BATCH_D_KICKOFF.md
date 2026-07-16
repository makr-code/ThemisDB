# Sprint 8: Batch D Use-After-Move Semantics Remediation

**Status:** ✅ **READY FOR EXECUTION**  
**Date:** 2026-07-03  
**Target:** Weeks 31-32 (2026-07-23 to 2026-08-05)  
**Total Gaps:** 97 CRITICAL

---

## 🎯 Executive Summary

Sprint 8 focuses on remediating **Use-After-Move vulnerabilities (CWE-416)** across the ThemisDB codebase. This batch targets scenarios where objects are used after `std::move()` has been applied, leading to undefined behavior, memory corruption, or silent failures.

**Key Metrics:**
- **Total Gaps:** 97 CRITICAL severity
- **Modules:** transaction (45), distributed (32), llm (20)
- **Expected Duration:** 2 weeks (parallel execution)
- **Success Target:** 50+ gaps fixed in Sprint 8 phase 1-2

---

## 🔍 Pattern Definition & Risk Assessment

### CWE-416: Use After Free / Use After Move

**Common Manifestations:**
1. **Post-Move Object Access**
   ```cpp
   Object obj = createObject();
   Object moved = std::move(obj);
   obj.doSomething();  // ❌ Undefined behavior
   ```

2. **Moved Transaction State**
   ```cpp
   Transaction tx = buildTransaction();
   auto future = executeAsync(std::move(tx));
   tx.rollback();  // ❌ tx is now in moved state
   ```

3. **Moved Reference Reuse**
   ```cpp
   auto& ref = getObjectRef();
   Container container = std::move(*ref);
   ref->use();  // ❌ ref points to moved object
   ```

### Risk Classification

| Pattern | Severity | Count | Risk Level |
|---------|----------|-------|-----------|
| Transaction state use-after-move | CRITICAL | 45 | 🔴 HIGH |
| Distributed coordinator moves | CRITICAL | 32 | 🔴 HIGH |
| LLM model reference moves | CRITICAL | 20 | 🟡 MEDIUM |
| **TOTAL** | — | **97** | — |

---

## 📋 High-Risk Patterns (Top 30)

### Pattern 1: Transaction Executor Use-After-Move (15 gaps)
**Location:** `src/transaction/transaction_executor.cpp`, `src/sharding/cross_shard_transaction.cpp`

**Root Cause:**
- Transactions passed to async executors via `std::move()`
- Original transaction object accessed post-move for state queries
- No move-constructor validation

**Example:**
```cpp
Transaction tx = buildTx();
auto result = asyncExecutor->execute(std::move(tx));
if (tx.status() == PENDING) { /* ❌ UB: tx moved */ }
```

**Remediation:**
- Extract status/metadata before move
- Create state handle that survives move
- Use move semantics only at execution boundary

**Affected Files:**
- `src/transaction/transaction_executor.cpp` (8 gaps)
- `src/sharding/cross_shard_transaction.cpp` (7 gaps)

---

### Pattern 2: Distributed Coordinator Pipeline (12 gaps)
**Location:** `src/distributed/coordinator_pipeline.cpp`

**Root Cause:**
- Coordinator objects chained through pipeline stages
- Intermediate stages use `std::move()` but later stages re-access
- No coordinator state snapshots

**Remediation:**
- Snapshot critical state before move
- Create immutable state wrapper
- Ensure move semantics respect state invariants

**Affected Files:**
- `src/distributed/coordinator_pipeline.cpp` (12 gaps)

---

### Pattern 3: LLM Model Move Chains (8 gaps)
**Location:** `src/rag/llm_integration.cpp`, `src/rag/model_manager.cpp`

**Root Cause:**
- Model objects moved between pipeline stages
- Held references to moved models
- No clear ownership model

**Remediation:**
- Use shared_ptr for shared ownership
- Document move semantics in API
- Add move constructor/assignment validation

**Affected Files:**
- `src/rag/llm_integration.cpp` (5 gaps)
- `src/rag/model_manager.cpp` (3 gaps)

---

## 🛠️ Remediation Strategy

### Phase 1: Analysis & Preparation (Days 1-3)

**Deliverables:**
- [ ] Gap analysis report with context for top 30 patterns
- [ ] Risk matrix (exploitability vs likelihood)
- [ ] Module dependency map (to identify safe remediation order)
- [ ] Move semantics audit for affected classes

**Approach:**
1. Extract AST context for each gap (surrounding code, function signature)
2. Classify by remediation complexity (simple vs complex)
3. Identify move constructor/assignment definitions
4. Map dependency chains

**Output:** `SPRINT_8_BATCH_D_GAP_ANALYSIS.md`

---

### Phase 2: Top 20 Quick Fixes (Days 4-7)

**Approach:**
Target the 20 simplest gaps with surgical fixes:

**Fix Type A: Pre-Move State Snapshot (8 gaps)**
```cpp
// BEFORE
auto result = executor->execute(std::move(transaction));

// AFTER
TransactionState state = transaction.captureState();
auto result = executor->execute(std::move(transaction));
// Use state, not transaction
```

**Fix Type B: Index-Based Access (7 gaps)**
```cpp
// BEFORE
auto& obj = getObject();
Container moved = std::move(*obj);
obj->property();  // ❌ UB

// AFTER
size_t objId = obj->getId();
Container moved = std::move(*obj);
// Query by ID instead
```

**Fix Type C: Move Only at Scope Exit (5 gaps)**
```cpp
// BEFORE
T obj = create();
T moved = std::move(obj);
// other uses of obj
return moved;

// AFTER
T obj = create();
// ... all uses
return std::move(obj);  // only at end
```

**Output:**
- 20+ individual fix commits
- `SPRINT_8_BATCH_D_PHASE_2_FIXES.md` tracking

---

### Phase 3: Complex Refactors (Days 8-14)

**Approach:**
Address remaining 10-15 complex gaps requiring API changes:

**Pattern A: Move Constructor Documentation**
- Document what remains valid post-move
- Ensure move constructor leaves object in valid state
- Add assertions in destructors

**Pattern B: State Machine Guards**
- Add `is_moved_from` flag (debug mode)
- Assert access patterns match move semantics
- Use state machine to enforce valid transitions

**Pattern C: Ownership Restructuring**
- Use `unique_ptr` + `release()` instead of move
- Use `shared_ptr` for shared ownership
- Document explicit ownership boundaries

**Output:** `SPRINT_8_BATCH_D_REFACTORING_GUIDE.md`

---

## ✅ Acceptance Criteria

### Phase 1 Success (50% of gaps)
- [ ] 50+ gaps resolved with verified fixes
- [ ] All fixes backward compatible
- [ ] Zero new test failures
- [ ] Move semantics audit complete

### Phase 2 Success (80% of gaps)
- [ ] 77+ gaps resolved
- [ ] State machine guards in place
- [ ] Comprehensive move semantics tests added
- [ ] API documentation updated

### Sprint Success (100% of gaps)
- [ ] All 97 gaps resolved
- [ ] Undefined behavior tests pass
- [ ] Move constructor/assignment audit complete
- [ ] Zero regressions in transaction tests
- [ ] Zero regressions in distributed tests

---

## 🧪 Testing Strategy

### Test 1: Use-After-Move Detection
```cpp
// Should catch use after move
TEST(MoveSemantics, TransactionUseAfterMove) {
  Transaction tx = buildTransaction();
  Transaction moved = std::move(tx);
  
  // This should fail/be caught
  EXPECT_EQ(tx.status(), PENDING);  // Catch UB
}
```

### Test 2: Move Constructor Validity
```cpp
// Verify moved objects are valid (impl-defined but consistent)
TEST(MoveSemantics, TransactionMovedStateValid) {
  Transaction tx = buildTransaction();
  auto txId = tx.id();
  Transaction moved = std::move(tx);
  
  // tx should be in valid (but unspecified) state
  // OR explicitly documented as invalid
  EXPECT_TRUE(tx.isMovedFrom() || tx.isValid());
}
```

### Test 3: Coordinator Pipeline Moves
```cpp
TEST(Distributed, CoordinatorPipelineMoves) {
  Coordinator coord = createCoordinator();
  auto stage1 = stage1Pipeline(std::move(coord));
  // Verify stage1 has valid state
  EXPECT_TRUE(stage1.isValid());
}
```

---

## 📊 Metrics & Success Indicators

| Metric | Target | Success Criterion |
|--------|--------|-------------------|
| Gaps Resolved | 97 | ≥ 80 by sprint end |
| False Positive Rate | < 2% | No incorrect moves |
| Test Pass Rate | 100% | All existing tests pass |
| Regression Rate | 0% | Zero new failures |
| Code Review Cycle | < 3 days | Fast turnaround |

---

## 🗓️ Sprint Timeline

| Phase | Duration | Target | Status |
|-------|----------|--------|--------|
| **Phase 1: Analysis** | 3 days | Top 30 gaps analyzed | ⏳ READY |
| **Phase 2: Quick Fixes** | 4 days | Top 20 gaps fixed | ⏳ QUEUED |
| **Phase 3: Complex** | 7 days | Remaining 30-40 gaps | ⏳ QUEUED |
| **Integration** | 2 days | Full suite tests | ⏳ QUEUED |
| **Total** | **16 days** | **v1.5.0 target** | — |

---

## 🚀 Next Actions

1. **Day 1:** Begin gap analysis (extract context for top 30)
2. **Day 2:** Risk matrix + dependency mapping
3. **Day 3:** Prepare first batch of 10 simple fixes
4. **Day 4-7:** Implement and test phase 2 fixes
5. **Week 2:** Complex refactoring + integration testing

---

## 📚 References

- **Batch D Definition:** `PHASE_1_4_REMEDIATION_BATCHES.md` (lines 122-151)
- **Phase 1-4 Overview:** `PHASE_1_4_AND_6_EXECUTIVE_SUMMARY.md`
- **CWE-416 Details:** https://cwe.mitre.org/data/definitions/416.html
- **C++ Move Semantics:** Scott Meyers - "Effective Modern C++" Chapter 5

---

**Status:** Ready for Phase 1 execution  
**Owner:** Core Infrastructure Team  
**Sprint Duration:** 2026-07-23 to 2026-08-05
