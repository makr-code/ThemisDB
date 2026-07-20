# Sprint 8 Move Semantics Remediation Guide

**Date:** 2026-07-05  
**Status:** Ready for Phase 2-4 Implementation  
**Target:** 97 move semantics gaps across 50+ modules

---

## 1. Remediation Patterns & Implementation Guide

### Pattern A: Implicit Move Semantics

**Problem:** Classes with moveable members but no explicit move semantics defined.

**Symptoms:**
- Compiler-generated move operations are copy-like or deleted
- Use-after-move bugs from implicit shallow copies
- Performance issues from unnecessary copies

**Implementation Template:**

```cpp
// BEFORE:
class DataBuffer {
  std::vector<uint8_t> data_;
  size_t capacity_ = 0;
  // No move semantics - defaults to implicitly deleted or copy
};

// AFTER:
class DataBuffer {
  std::vector<uint8_t> data_;
  size_t capacity_ = 0;

 public:
  // Explicit move constructor
  DataBuffer(DataBuffer&& other) noexcept 
      : data_(std::move(other.data_)), capacity_(other.capacity_) {
    other.capacity_ = 0; // Ensure source in valid state
  }

  // Explicit move assignment
  DataBuffer& operator=(DataBuffer&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      capacity_ = other.capacity_;
      other.capacity_ = 0; // Ensure source in valid state
    }
    return *this;
  }

  // Delete copy semantics (optional, use with care)
  DataBuffer(const DataBuffer&) = delete;
  DataBuffer& operator=(const DataBuffer&) = delete;
};
```

**SafeMove Integration:**
```cpp
DataBuffer& operator=(DataBuffer&& other) noexcept {
  if (this != &other) {
    data_ = std::move(other.data_);
    capacity_ = other.capacity_;
    other.capacity_ = 0;
    
    // Optional: Validate post-move state
    THEMIS_VALIDATE_MOVE(other);
  }
  return *this;
}
```

**Affected Modules (Type A - ~40-50 gaps):**
- llm (10-12 gaps) - Model cache, adapter moves
- tensor (8-10 gaps) - Tensor data, metadata structures
- query (8-10 gaps) - Query plan nodes, iterators
- storage (5-8 gaps) - RocksDB wrappers, indices
- acceleration (5-7 gaps) - GPU kernel wrappers

---

### Pattern B: Move Constructor/Assignment Issues

**Problem:** Incomplete or incorrect move constructor/assignment implementations.

**Symptoms:**
- Member not moved in constructor
- Source not cleared after move
- Exception safety violations in move operations
- Use-after-free from shallow copies

**Implementation Template:**

```cpp
// BEFORE:
class TransactionContext {
  std::vector<WriteOperation> operations_;
  std::string transaction_id_;

  TransactionContext(TransactionContext&& other) {
    // BUG 1: transaction_id_ not moved
    operations_ = std::move(other.operations_);
    // BUG 2: other.operations_ may still be valid (not std::moved)
  }
};

// AFTER:
class TransactionContext {
  std::vector<WriteOperation> operations_;
  std::string transaction_id_;

 public:
  TransactionContext(TransactionContext&& other) noexcept
      : operations_(std::move(other.operations_)),
        transaction_id_(std::move(other.transaction_id_)) {
    // SafeMove validation
    THEMIS_VALIDATE_MOVE(other);
  }

  TransactionContext& operator=(TransactionContext&& other) noexcept {
    if (this != &other) {
      operations_ = std::move(other.operations_);
      transaction_id_ = std::move(other.transaction_id_);
      THEMIS_VALIDATE_MOVE(other);
    }
    return *this;
  }
};
```

**Affected Modules (Type B - ~30-35 gaps):**
- sharding (8-10 gaps) - Transaction coordinators, write operations
- replication (6-8 gaps) - WAL entries, replication context
- network (5-7 gaps) - Message buffers, protocol structures
- cache (4-6 gaps) - Cache entries, expiry tracking
- distributed_knowledge (3-5 gaps) - Knowledge graph structures

---

### Pattern C: Complex Move Scenarios

**Problem:** Nested moves, move chains, or moves in template/polymorphic contexts.

**Symptoms:**
- Moves inside containers or smart pointers
- Move-only types in complex ownership scenarios
- Move operations on polymorphic types
- Long move chains with state leakage

**Implementation Template:**

```cpp
// BEFORE:
template<typename T>
class ObjectPool {
  std::vector<std::unique_ptr<T>> items_;
  
  std::unique_ptr<T> retrieve() {
    if (items_.empty()) return nullptr;
    auto item = std::move(items_.back());
    items_.pop_back();
    // BUG: item might be invalid if T's move is broken
    return item;
  }
};

// AFTER:
template<typename T>
class ObjectPool {
  std::vector<std::unique_ptr<T>> items_;
  MoveChainTracker chain_tracker_;
  
 public:
  std::unique_ptr<T> retrieve() {
    if (items_.empty()) return nullptr;
    
    chain_tracker_.onMoveBegin();
    try {
      // Validate T's move semantics
      THEMIS_VALIDATE_MOVE(items_.back());
      auto item = std::move(items_.back());
      items_.pop_back();
      chain_tracker_.onMoveEnd();
      return item;
    } catch (...) {
      chain_tracker_.onMoveEnd();
      throw;
    }
  }
  
  // Safe extraction with guard
  std::unique_ptr<T> safeRetrieve() {
    if (items_.empty()) return nullptr;
    
    auto guard = THEMIS_MOVE_GUARD(items_.back());
    auto item = std::move(items_.back());
    items_.pop_back();
    
    guard.markMovedFrom();
    return item;
  }
};
```

**Affected Modules (Type C - ~15-20 gaps):**
- llm (3-4 gaps) - Polymorphic model adapters
- graph (3-4 gaps) - Graph node/edge moves
- server (3-4 gaps) - Connection/session context moves
- temporal (2-3 gaps) - Time-series data moves
- others (4-5 gaps) - Miscellaneous complex scenarios

---

## 2. Gap Categorization & Priority

### Module Priority (by gap count + impact):

| Rank | Module | Gaps | Type | Priority | Est. Hours |
|------|--------|------|------|----------|-----------|
| 1 | llm | 12 | A/C | P0 | 8-10 |
| 2 | sharding | 10 | B | P0 | 6-8 |
| 3 | query | 10 | A | P0 | 6-8 |
| 4 | tensor | 8 | A/B | P1 | 5-6 |
| 5 | storage | 7 | A/B | P1 | 4-5 |
| 6 | replication | 7 | B/C | P1 | 5-6 |
| 7 | acceleration | 6 | A | P1 | 4-5 |
| 8 | network | 6 | B | P1 | 4-5 |
| 9 | cache | 5 | A/B | P2 | 3-4 |
| 10 | distributed_knowledge | 4 | B | P2 | 2-3 |
| 11-50 | Others (40 modules) | 37 | Mixed | P2 | 20-25 |
| **Total** | | **97** | | | **65-80 hrs** |

---

## 3. Implementation Phases

### Phase 1: SafeMove Library (Week 1) ✅
- [x] SafeMove header + implementation (completed)
- [x] 40+ unit tests (completed)
- [x] Exception hierarchy (completed)
- [x] Macros for common patterns (completed)

### Phase 2: Type A Remediation (Week 1-2)
- [ ] llm module (10-12 gaps)
- [ ] query module (10 gaps)
- [ ] tensor module (8 gaps)
- [ ] storage module (7 gaps)
- [ ] acceleration module (6 gaps)
- [ ] cache module (5 gaps)

**Subtotal:** 46-50 gaps

### Phase 3: Type B & C Remediation (Week 2-3)
- [ ] sharding module (10 gaps, Type B)
- [ ] replication module (7 gaps, Type B/C)
- [ ] network module (6 gaps, Type B)
- [ ] distributed_knowledge module (4 gaps, Type B)
- [ ] llm polymorphic moves (2 gaps, Type C)
- [ ] graph module (3 gaps, Type C)
- [ ] server module (3 gaps, Type C)
- [ ] temporal module (2 gaps, Type C)
- [ ] misc modules (14 gaps)

**Subtotal:** 47-50 gaps

### Phase 4: Testing & Validation (Week 3)
- [ ] Unit tests for each remediated file
- [ ] Integration tests for move chains
- [ ] Regression test suite (backward compatibility)
- [ ] Performance benchmarking

### Phase 5: Documentation & Merge (End of Week 3)
- [ ] Move remediation patterns guide
- [ ] API documentation updates
- [ ] CHANGELOG entry for v1.5.0
- [ ] Merge to develop branch

---

## 4. Testing Strategy

### Unit Tests for Each Remediation:

```cpp
// Template for per-file tests

TEST_F(MyModuleTest, MoveSemanticsFix_<Location>) {
  // Setup
  MyType obj = createTestObject();
  
  // Move operation
  MyType moved = std::move(obj);
  
  // Validation
  EXPECT_TRUE(isValidAfterMove(obj));
  EXPECT_EQ(moved.state(), expectedState);
  EXPECT_THROW(useMovedObject(obj), UseAfterMoveException);
}

TEST_F(MyModuleTest, MoveConstructor_<Location>) {
  MyType original = createTestObject();
  MyType moved(std::move(original));
  
  EXPECT_TRUE(isValidState(moved));
  EXPECT_TRUE(isMovedFromState(original));
}

TEST_F(MyModuleTest, MoveAssignment_<Location>) {
  MyType original = createTestObject();
  MyType dest;
  dest = std::move(original);
  
  EXPECT_TRUE(isValidState(dest));
  EXPECT_TRUE(isMovedFromState(original));
}
```

### Integration Tests for Move Chains:

```cpp
TEST_F(IntegrationTest, MoveChain_<Path>) {
  // Chain: A → B → C → D
  auto a = createA();
  auto b = processA(std::move(a));
  auto c = processB(std::move(b));
  auto d = processC(std::move(c));
  
  EXPECT_TRUE(isValid(d));
  EXPECT_TRUE(isMovedFrom(a));
  EXPECT_TRUE(isMovedFrom(b));
  EXPECT_TRUE(isMovedFrom(c));
}
```

---

## 5. Validation Checklist

For each remediated gap:

- [ ] Move constructor/assignment properly implemented
- [ ] All members moved (no missing member moves)
- [ ] Source object left in valid state after move
- [ ] Exception safety maintained (noexcept where applicable)
- [ ] Copy semantics deleted if move-only
- [ ] SafeMove validation applied (where appropriate)
- [ ] Unit tests pass (100%)
- [ ] No compiler warnings
- [ ] No new security issues
- [ ] Backward compatibility maintained

---

## 6. Common Pitfalls & Solutions

| Pitfall | Example | Solution |
|---------|---------|----------|
| Member not moved | `data_ = other.data_;` | Use `std::move()`: `data_ = std::move(other.data_);` |
| Source not cleared | `MyClass(T&& o): ptr_(o.ptr_) {}` | Set source to null: `o.ptr_ = nullptr;` |
| Missing noexcept | `MyClass(MyClass&&) { ... }` | Add noexcept: `MyClass(MyClass&&) noexcept { ... }` |
| Delete copy implicitly | `MyClass(const MyClass&) = default;` | Explicitly delete: `MyClass(const MyClass&) = delete;` |
| Exception in move | Move throws exception | Ensure move operations never throw (noexcept) |
| Incomplete move chain | Only some members moved | Use initializer list for all moves |

---

## 7. Remediation Statistics

### Gap Distribution by Type:

```
Type A (Implicit Move): 45-50 gaps
  ├─ Missing move constructors: 25-30
  ├─ Missing move assignments: 15-20
  └─ Move-only semantics not enforced: 5-10

Type B (Constructor Issues): 30-35 gaps
  ├─ Members not moved: 15-18
  ├─ Source not cleared: 10-12
  └─ Missing noexcept: 5-7

Type C (Complex Scenarios): 15-20 gaps
  ├─ Polymorphic type moves: 5-7
  ├─ Template instantiation issues: 5-7
  └─ Move chain tracking: 5-6
```

### Expected Outcomes:

- **Code Coverage:** 100% of move operations validated
- **Performance:** 5-10% faster move operations vs. copy
- **Safety:** Zero use-after-move, double-free, or move violations
- **Backward Compatibility:** 100% of existing tests pass
- **Test Coverage:** 100+ new unit tests, 20+ integration tests

---

## 8. References & Resources

- **SafeMove Library:** `include/security/safe_move.h`
- **SafeMove Tests:** `tests/security/test_safe_move.cpp`
- **SafeIterator Reference:** `include/security/safe_iterator.h` (Sprint 7 pattern)
- **Sprint 7 Report:** `ai_working/SPRINT_7_FINAL_COMPLETION_REPORT.md`
- **Phase 1-4 Batches:** `ai_working/PHASE_1_4_REMEDIATION_BATCHES.md`

---

## Next Steps

1. **Week 1:** Complete SafeMove library + Phase 2 Type A (40 gaps)
2. **Week 2:** Phase 3 Type B/C remediations (47-50 gaps)
3. **Week 3:** Testing, validation, documentation
4. **2026-07-26:** Ready for merge to develop
5. **2026-08-31:** v1.5.0 release with all 97 gaps remediated

**Total Effort:** 65-80 hours  
**Estimated Completion:** 2026-07-26  
**Release Target:** v1.5.0 (2026-08-31)
