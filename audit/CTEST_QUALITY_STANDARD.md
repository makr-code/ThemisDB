---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "Reference for all test development"
---

# CTest Test Quality Standard (REFERENCE)

## Coverage Requirement: Mandatory for All New Tests

**Effective Date:** 2026-10-11  
**Applies To:** All new tests in Phase 3 (Batch 1-4) and beyond  
**Compliance:** Required before code review & merge

---

## 1. Test Categories (Mandatory Classification)

Every test MUST be classified into **exactly ONE** primary category:

### A. Function-Level Tests (Unit Tests)
**Purpose:** Test individual public API functions in isolation

**Characteristics:**
- Single function under test
- Mock all dependencies (databases, services, external I/O)
- Input validation + output correctness
- All parameter types and return codes
- No external state dependencies

**Example:**
```cpp
TEST(Parser, test_parse_valid_json) {
    // Test single function
    auto result = parseJSON("{\"key\": \"value\"}");
    EXPECT_EQ(result.status, ParseStatus::OK);
    EXPECT_EQ(result.value["key"], "value");
}
```

**Ratio:** 40-50% of tests per module

---

### B. Module-Level Tests (Component Integration)
**Purpose:** Test classes/subsystems as cohesive units

**Characteristics:**
- Multiple functions/classes work together
- Use real internal state (hash tables, caches, state machines)
- Mock only external boundaries (I/O, network, databases)
- Verify state transitions and invariants
- Test lifecycle (create → use → destroy)

**Example:**
```cpp
TEST_F(StorageEngineTest, test_write_read_cycle) {
    // Test engine as whole
    engine.Put("key", "value");
    auto result = engine.Get("key");
    EXPECT_EQ(result.value(), "value");
    engine.Flush();
}
```

**Ratio:** 15-20% of tests per module

---

### C. Cross-Module Tests (Integration Tests)
**Purpose:** Test interaction between 2+ modules

**Characteristics:**
- Verify contracts at module boundaries
- Use realistic data flow (not simplified mocks)
- Validate error propagation across boundaries
- Example: Server→Auth→Storage flow

**Example:**
```cpp
TEST(ServerAuthStorageIntegration, test_authenticated_write) {
    // Test server, auth, and storage together
    auto req = CreateRequest({"token": "valid_token", "data": "..."});
    auto result = server.ProcessRequest(req);
    EXPECT_EQ(result.status, StatusCode::OK);
    // Verify data reached storage
    EXPECT_TRUE(storage.Contains(req.data_id));
}
```

**Ratio:** 8-15% of tests per module

---

### D. Happy-Path Tests (Normal Operation)
**Purpose:** Most common use case with valid input

**Characteristics:**
- Success scenario
- Valid input, expected output
- Minimal error handling
- No stress or edge cases
- Baseline performance

**Example:**
```cpp
TEST(Query, test_simple_select_all) {
    // Simple, normal case
    auto result = query.Execute("SELECT * FROM users");
    EXPECT_TRUE(result.success());
    EXPECT_GT(result.row_count(), 0);
}
```

**Ratio:** 10-15% of tests per module

---

### E. Failure/Error-Path Tests (Robustness)
**Purpose:** Invalid input or resource failures

**Characteristics:**
- Invalid/malformed input (type errors, syntax errors)
- Resource exhaustion (OOM, connection limits)
- Dependency failures (service unavailable, timeout)
- Partial failures (mid-operation crash)
- Verify error codes, messages, logging
- State consistency after failure

**Example:**
```cpp
TEST(Storage, test_write_out_of_space) {
    // Test error path
    storage.SetAvailableSpace(0);
    auto result = storage.Put("key", "value");
    EXPECT_EQ(result.error_code(), ErrorCode::NO_SPACE);
    // Verify state is still consistent
    EXPECT_TRUE(storage.IsConsistent());
}
```

**Ratio:** 10-12% of tests per module

---

### F. Edge-Case Tests (Boundary Conditions)
**Purpose:** Unusual but valid inputs

**Characteristics:**
- Empty/NULL inputs (zero-length arrays, NULL values)
- Boundary values (INT_MAX, INT_MIN, size limits)
- Special characters (encoding, escaping)
- Concurrent access patterns
- Memory pressure (large allocations)
- Timeout boundaries (0ms, infinite)

**Example:**
```cpp
TEST(TensorIndexing, test_access_empty_tensor) {
    // Edge case: empty tensor
    Tensor t(0, 10);  // 0 rows
    EXPECT_THROW(t.Get(0, 0), OutOfBoundsException);
}
```

**Ratio:** 5-8% of tests per module

---

## 2. Test Distribution by Tier

### Tier A: Release-Critical (Server, Auth, Storage, Query, Transaction)

**Target Distribution:**
| Category | Percentage | Example (100 tests) |
|----------|-----------|---|
| Function-Level | 50-60% | 50-60 tests |
| Module-Level | 15-20% | 15-20 tests |
| Cross-Module | 10-15% | 10-15 tests |
| Happy-Path | 10-15% | 10-15 tests |
| Failure/Error | 8-12% | 8-12 tests |
| Edge-Case | 5-8% | 5-8 tests |
| **TOTAL** | **100%** | **100 tests** |

**Coverage Minimums:**
- ≥90% of public functions tested (Function-Level)
- ≥80% of state transitions (Module-Level)
- ≥70% of boundary contracts (Cross-Module)
- ≥95% of normal workflows pass (Happy-Path)
- ≥80% of error codes/exceptions triggered (Failure/Error)
- ≥60% of boundary values tested (Edge-Case)

---

### Tier B: Important (Index, Sharding, RAG, LLM, Security)

**Target Distribution:**
| Category | Percentage | Example (80 tests) |
|----------|-----------|---|
| Function-Level | 50-60% | 40-48 tests |
| Module-Level | 15-20% | 12-16 tests |
| Cross-Module | 8-12% | 6-10 tests |
| Happy-Path | 12-15% | 10-12 tests |
| Failure/Error | 5-8% | 4-6 tests |
| Edge-Case | 5-8% | 4-6 tests |
| **TOTAL** | **100%** | **80 tests** |

**Coverage Minimums:**
- ≥80% of public functions tested
- ≥70% of state transitions
- ≥60% of boundary contracts
- ≥90% of normal workflows pass
- ≥70% of error codes triggered
- ≥50% of boundary values tested

---

### Tier C: Optional/Experimental (GPU, Voice, Analytics)

**Target Distribution:**
| Category | Percentage | Example (30 tests) |
|----------|-----------|---|
| Function-Level | 40-50% | 12-15 tests |
| Module-Level | 15-20% | 5-6 tests |
| Cross-Module | 8-12% | 2-4 tests |
| Happy-Path | 15-20% | 5-6 tests |
| Failure/Error | 5-8% | 1-2 tests |
| Edge-Case | 5-10% | 1-3 tests |
| **TOTAL** | **100%** | **30 tests** |

**Coverage Minimums:**
- ≥60% of public functions tested
- ≥50% of state transitions
- ≥40% of boundary contracts
- ≥80% of normal workflows pass
- ≥50% of error codes triggered
- ≥30% of boundary values tested

---

## 3. Quality Standards (Non-Negotiable)

### Isolation
- ✅ Each test is completely independent
- ❌ No shared global state between tests
- ❌ No inter-test dependencies
- ✅ SetUp/TearDown cleanly initialize/destroy state

**Validation:**
```cpp
// Run same test 5 times in sequence—must pass every time
for (int i = 0; i < 5; i++) {
    EXPECT_TRUE(RunTest("test_function"));
}
```

---

### Determinism
- ✅ Same input always produces same output
- ❌ No randomness in test logic
- ❌ No time-dependent assertions (unless testing timeouts)
- ✅ No thread scheduling dependencies
- ✅ No file system/network ordering assumptions

**Validation:**
```cpp
// Run test 10 consecutive times—should never flake
GTEST_SKIP_("Manual flake check");
for (int i = 0; i < 10; i++) {
    EXPECT_TRUE(RunTest("test_function"));
}
```

---

### Speed
- ✅ Each test <100ms execution time
- ❌ No sleep() calls (use time mocks instead)
- ❌ No network I/O (mock network layer)
- ❌ No disk I/O (use in-memory storage)
- ✅ Fast failing (fail fast on first assertion)

**Validation:**
```bash
# Measure test time
time ./run_tests
# Each test should take <100ms (entire suite depends on test count)
```

---

### Clarity
- ✅ Test name describes what is being tested
- ✅ Test name follows pattern: `test_<function>_<scenario>`
- ❌ No ambiguous names ("test_1", "test_works")
- ✅ Comments explain complex setup

**Examples:**
- ✅ `test_parser_valid_json_with_nested_objects`
- ✅ `test_storage_put_out_of_memory_error`
- ❌ `test_1`
- ❌ `test_works`

---

### Completeness
- ✅ Happy-path + error-path + edge-case per public function
- ✅ All error codes tested
- ✅ All state transitions verified
- ❌ No TODO tests ("EXPECT_TRUE(true)" placeholders)
- ❌ No skipped tests without reason

**Validation:**
```cpp
// For each public function, require 3+ tests
class FunctionTests {
    TEST_F(FunctionTests, test_happy_path) { ... }
    TEST_F(FunctionTests, test_error_case) { ... }
    TEST_F(FunctionTests, test_edge_case) { ... }
};
```

---

## 4. Naming Convention (Mandatory)

```
TEST(ClassName, test_<function_name>_<scenario>)
```

### Examples:

**Function-Level:**
- `test_parse_valid_json()`
- `test_parse_invalid_json()`
- `test_parse_empty_string()`
- `test_get_element_out_of_bounds()`

**Module-Level:**
- `test_storage_write_read_cycle()`
- `test_connection_pool_lifecycle()`
- `test_cache_eviction_policy()`

**Cross-Module:**
- `test_server_auth_token_validation()`
- `test_query_storage_range_scan()`
- `test_sharding_transaction_consistency()`

**Happy-Path:**
- `test_auth_valid_token_happy_path()`
- `test_simple_query_success()`

**Error-Path:**
- `test_storage_write_no_space_error()`
- `test_connection_timeout_error()`
- `test_auth_invalid_token_error()`

**Edge-Case:**
- `test_empty_input_handling()`
- `test_maximum_size_boundary()`
- `test_NULL_pointer_safety()`

---

## 5. Code Review Checklist (Before Merge)

### Functionality
- [ ] Test actually tests the intended function/module
- [ ] All assertions are verifying meaningful behavior
- [ ] No trivial assertions (e.g., `EXPECT_TRUE(true)`)
- [ ] Error cases are properly handled

### Quality
- [ ] Test isolation: no shared state, SetUp/TearDown are clean
- [ ] Test determinism: runs consistently without flakes
- [ ] Test speed: <100ms (measure with `time`)
- [ ] Test clarity: name describes scenario, comments explain complexity

### Coverage
- [ ] Categorized correctly (function/module/cross-module/happy/error/edge)
- [ ] Part of distribution target (% per tier)
- [ ] Contributes to coverage minimum (% of functions/modules/contracts)
- [ ] No duplicate test coverage (different test != duplicate logic)

### CTest Compliance
- [ ] Naming follows `test_<function>_<scenario>` pattern
- [ ] SetUp/TearDown used (not inline initialization)
- [ ] Mock/stub external dependencies clearly
- [ ] No hardcoded paths, ports, or time-dependent values

---

## 6. Audit & Validation

### Per-Test Audit
1. **Category Check:** Does test fit exactly one category (A-F)?
2. **Naming Check:** Does name match `test_<function>_<scenario>` pattern?
3. **Isolation Check:** Run test in isolation 5 consecutive times—all pass?
4. **Speed Check:** Does test complete <100ms?
5. **Coverage Check:** Does test contribute to coverage minimum for tier?

### Per-Module Audit
1. **Distribution Check:** Do % of tests per category match tier target?
2. **Completeness Check:** ≥90% of public functions have tests (Tier A)?
3. **Consistency Check:** All tests follow same quality standards?
4. **Regression Check:** Any new tests have lower pass rate than existing?

### CI/CD Integration
- [ ] Coverage gate enabled (fail if <threshold)
- [ ] Flake detection enabled (flag tests that fail <5% of time)
- [ ] Performance tracking (log test duration per run)
- [ ] Coverage badge updated on README

---

## 7. Troubleshooting Common Issues

### Test Flakes (Intermittent Failures)
**Cause:** Time-dependent code, thread scheduling, file system ordering

**Fix:**
- Remove sleep() calls—use time mocks instead
- Eliminate thread scheduling dependencies—use deterministic test order
- Mock file system/network I/O

### Slow Tests (>100ms)
**Cause:** Real I/O (disk, network), large data sets, heavy computation

**Fix:**
- Mock external I/O (use in-memory storage)
- Reduce data size for functional tests
- Move performance tests to separate benchmark suite

### Test Isolation Failures
**Cause:** Shared global state, cleanup not working

**Fix:**
- Use SetUp/TearDown for clean state
- Avoid global variables
- Use thread-local storage for thread-scoped data

### Coverage Gaps
**Cause:** Not all error paths tested

**Fix:**
- Add tests for all error codes returned
- Test boundary conditions (0, INT_MAX, empty, NULL)
- Test all state transitions

---

## 8. Reference Documents

- `/audit/PHASE1_COVERAGE_BASELINE_2026-10-11.md` — Module audit
- `/audit/PHASE2_PRIORITIZATION_2026-10-11.md` — Test targets per module
- `/audit/PHASE3_BATCH1_IMPLEMENTATION_CHECKLIST.md` — Specific tests to implement
- `/audit/EXECUTIVE_SUMMARY_TEST_DENSITY_2026-10-11.md` — Overview & timeline

---

## Questions?

Contact: Copilot Task Agent  
Date: 2026-10-11  
Status: Active (all new tests effective immediately)

---

*This document is the authoritative standard for all test development in ThemisDB starting 2026-10-11.*
