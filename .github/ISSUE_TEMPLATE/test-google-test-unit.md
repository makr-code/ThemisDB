---
name: ✅ Google Test Unit Tests
about: Implement or improve unit tests using Google Test (gtest) framework
title: '[UNITTEST] '
labels: ['type:testing', 'area:unit-tests', 'tool:google-test', 'needs-triage']
assignees: ''
---

## 🎯 Test Objective / Test-Ziel

**Component to Test:** <!-- z.B. Storage Layer, Query Parser, Vector Index -->
**Component Path:** <!-- z.B. src/storage/, src/query/parser.cpp, src/index/ -->
**Test Type:**
- [ ] New functionality testing
- [ ] Bug fix verification
- [ ] Regression prevention
- [ ] Code coverage improvement
- [ ] Refactoring validation

---

## 📋 Test Scope / Test-Umfang

### Functions/Classes to Test / Zu testende Funktionen/Klassen

**Target Coverage:**
- [ ] **Class/Function 1**: `ClassName::methodName()`
  - File: `src/path/to/file.cpp:line`
  - Current Coverage: <!-- e.g., 0%, 45%, Not covered -->
  - Target Coverage: <!-- e.g., 85%, 100% -->

- [ ] **Class/Function 2**: `ClassName::methodName()`
  - File: `src/path/to/file.cpp:line`
  - Current Coverage: 
  - Target Coverage: 

### Test Categories / Test-Kategorien

- [ ] **Happy Path**: Normal, expected usage
- [ ] **Edge Cases**: Boundary conditions, limits
- [ ] **Error Handling**: Invalid inputs, exceptions
- [ ] **State Management**: Initialization, transitions, cleanup
- [ ] **Thread Safety**: Concurrent access (if applicable)
- [ ] **Performance**: Not slow (< 100ms per test)
- [ ] **Integration**: Interaction with dependencies

---

## 🔬 Test Implementation / Test-Implementierung

### Test File Structure / Test-Datei-Struktur

**File Location:** `tests/test_<component>.cpp`

```cpp
#include <gtest/gtest.h>
#include "themisdb/<component>/<class>.h"

// Test Fixture (optional, for complex setup)
class ComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data, mock objects, etc.
        component_ = std::make_unique<Component>();
    }
    
    void TearDown() override {
        // Cleanup
        component_.reset();
    }
    
    std::unique_ptr<Component> component_;
    // Other test data members
};

// Simple test (no fixture)
TEST(ComponentBasicTest, CreatesSuccessfully) {
    Component comp;
    EXPECT_TRUE(comp.isValid());
}

// Test with fixture
TEST_F(ComponentTest, PerformsOperationCorrectly) {
    // Arrange
    component_->setup();
    
    // Act
    auto result = component_->operation();
    
    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), expected_value);
}

// Parameterized test for multiple inputs
class ParameterizedComponentTest 
    : public ComponentTest,
      public ::testing::WithParamInterface<TestInput> {
};

TEST_P(ParameterizedComponentTest, HandlesVariousInputs) {
    auto input = GetParam();
    auto result = component_->process(input);
    EXPECT_TRUE(result.isValid());
}

INSTANTIATE_TEST_SUITE_P(
    VariousInputs,
    ParameterizedComponentTest,
    ::testing::Values(
        TestInput{1, "data1"},
        TestInput{2, "data2"},
        TestInput{3, "data3"}
    )
);
```

---

## ✅ Test Cases / Testfälle

### Happy Path Tests / Glückspfad-Tests

#### Test Case 1: [Description]
```cpp
TEST_F(ComponentTest, TestName_ExpectedBehavior) {
    // Arrange: Setup preconditions
    
    // Act: Execute the operation
    
    // Assert: Verify results
    EXPECT_EQ(actual, expected);
    EXPECT_TRUE(condition);
}
```

**Covers:**
- Function: `ClassName::method()`
- Input: Valid, typical input
- Expected: Successful operation with correct result

---

### Edge Case Tests / Grenzfall-Tests

#### Test Case 2: Empty Input
```cpp
TEST_F(ComponentTest, HandlesEmptyInput) {
    auto result = component_->process("");
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), ErrorCode::INVALID_INPUT);
}
```

#### Test Case 3: Maximum Size
```cpp
TEST_F(ComponentTest, HandlesMaximumSize) {
    std::string large_input(MAX_SIZE, 'x');
    auto result = component_->process(large_input);
    EXPECT_TRUE(result.isSuccess());
}
```

#### Test Case 4: Minimum Size
```cpp
TEST_F(ComponentTest, HandlesMinimumSize) {
    std::string min_input(1, 'x');
    auto result = component_->process(min_input);
    EXPECT_TRUE(result.isSuccess());
}
```

---

### Error Handling Tests / Fehlerbehandlungs-Tests

#### Test Case 5: Null Pointer
```cpp
TEST_F(ComponentTest, RejectsNullPointer) {
    EXPECT_THROW(component_->process(nullptr), std::invalid_argument);
}
```

#### Test Case 6: Invalid State
```cpp
TEST_F(ComponentTest, FailsInInvalidState) {
    component_->invalidate();
    auto result = component_->operation();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::INVALID_STATE);
}
```

---

### Thread Safety Tests / Thread-Sicherheits-Tests

```cpp
TEST_F(ComponentTest, ThreadSafeOperation) {
    constexpr int NUM_THREADS = 10;
    constexpr int OPS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                if (component_->threadSafeOp()) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count, NUM_THREADS * OPS_PER_THREAD);
}
```

---

## 🛠️ Build Integration / Build-Integration

### CMakeLists.txt Entry

```cmake
# Add Google Test executable
add_executable(test_<component>
    tests/test_<component>.cpp
)

target_link_libraries(test_<component>
    PRIVATE
        themisdb_<component>
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# Register with CTest
gtest_discover_tests(test_<component>
    PROPERTIES
        LABELS "unit;component"
        TIMEOUT 30
)
```

### Dependencies / Abhängigkeiten

- [ ] Google Test library (gtest)
- [ ] Google Mock library (gmock) - if mocking needed
- [ ] Component library under test
- [ ] Mock objects (if needed)
- [ ] Test data files (if needed)

---

## 📊 Coverage Targets / Coverage-Ziele

### Before / Vorher

```bash
# Current coverage
lcov --capture --directory . --output-file coverage_before.info
lcov --list coverage_before.info | grep "src/<component>"
```

**Current Metrics:**
- Line Coverage: <!-- e.g., 45% -->
- Branch Coverage: <!-- e.g., 38% -->
- Function Coverage: <!-- e.g., 60% -->

### After / Nachher

**Target Metrics:**
- Line Coverage: <!-- e.g., 85% (target) -->
- Branch Coverage: <!-- e.g., 75% (target) -->
- Function Coverage: <!-- e.g., 90% (target) -->

### Coverage Gaps / Coverage-Lücken

**Untested Code:**
- `src/<component>/file.cpp:45-67` - Error handling path
- `src/<component>/file.cpp:123-145` - Edge case branch
- `src/<component>/file.cpp:234-256` - Cleanup code

---

## ✅ Acceptance Criteria / Akzeptanzkriterien

### Code Quality / Code-Qualität

- [ ] All tests pass (`ctest -R test_<component>`)
- [ ] No memory leaks (Valgrind clean)
- [ ] No undefined behavior (ASAN clean)
- [ ] No data races (TSAN clean)
- [ ] Test execution time < 1 second (fast tests)

### Coverage / Abdeckung

- [ ] Target coverage achieved (≥ 85% line coverage)
- [ ] All public methods tested
- [ ] Critical error paths tested
- [ ] Edge cases documented and tested

### Test Quality / Test-Qualität

- [ ] Tests are independent (no execution order dependency)
- [ ] Tests are deterministic (same input → same output)
- [ ] Tests are isolated (no side effects)
- [ ] Test names are descriptive (`ComponentTest_Operation_ExpectedBehavior`)
- [ ] Assertions are meaningful (not just `EXPECT_TRUE(true)`)
- [ ] Each test has clear Arrange-Act-Assert structure

### Documentation / Dokumentation

- [ ] Test purpose documented
- [ ] Complex test logic commented
- [ ] Test data sources documented
- [ ] Coverage report generated
- [ ] Added to test documentation

---

## 🧪 Test Execution / Test-Ausführung

### Run Tests Locally / Tests lokal ausführen

```bash
# Build and run specific test
mkdir build && cd build
cmake ..
make test_<component>
./test_<component>

# Run with verbose output
./test_<component> --gtest_print_time=1 --gtest_color=yes

# Run specific test case
./test_<component> --gtest_filter=ComponentTest.SpecificTest

# Run with repeat (detect flaky tests)
./test_<component> --gtest_repeat=100 --gtest_break_on_failure
```

### Generate Coverage Report / Coverage-Report generieren

```bash
# Build with coverage flags
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make test_<component>

# Run tests
./test_<component>

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report

# View report
xdg-open coverage_report/index.html
```

### CI/CD Integration / CI/CD-Integration

```yaml
# GitHub Actions example
- name: Run Unit Tests
  run: |
    cd build
    ctest -R test_<component> --output-on-failure
    
- name: Check Coverage
  run: |
    lcov --list coverage.info | grep "src/<component>"
    # Fail if coverage < 85%
```

---

## 🔗 References / Referenzen

### Google Test Documentation
- [Primer](https://google.github.io/googletest/primer.html)
- [Advanced Guide](https://google.github.io/googletest/advanced.html)
- [FAQ](https://google.github.io/googletest/faq.html)
- [Mocking](https://google.github.io/googletest/gmock_for_dummies.html)

### Internal Documentation
- [Testing Guidelines](../../docs/testing/guidelines.md)
- [Code Coverage Requirements](../../docs/testing/coverage.md)
- [CI/CD Pipeline](../../docs/ci-cd/pipeline.md)

### Related Tests
- <!-- Link to related test files/issues -->

---

## 🎓 Testing Best Practices / Best Practices

- [ ] **Test One Thing**: Each test verifies one behavior
- [ ] **Fast Tests**: Unit tests should be < 100ms each
- [ ] **No External Dependencies**: Mock external systems
- [ ] **Readable Tests**: Code as documentation
- [ ] **Arrange-Act-Assert**: Clear test structure
- [ ] **Meaningful Names**: `Test_Operation_Condition_ExpectedBehavior`
- [ ] **No Logic in Tests**: Keep tests simple
- [ ] **Test the Interface**: Don't test implementation details

---

**Created:** <!-- YYYY-MM-DD -->
**Owner:** <!-- Team/Person -->
**Priority:** <!-- P0/P1/P2/P3 -->
**Target Version:** <!-- v1.x.x -->
**Coverage Goal:** <!-- e.g., 85% -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-03  
**Maintained by:** ThemisDB Testing Team
