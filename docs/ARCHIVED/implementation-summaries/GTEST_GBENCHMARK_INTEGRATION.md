# ThemisDB Complete Testing and Benchmarking Integration Guide

## Übersicht / Overview

**English:** This guide provides complete documentation for gtest (Google Test) and gbenchmark (Google Benchmark) integration in ThemisDB, including examples, best practices, and usage patterns.

**Deutsch:** Dieser Leitfaden bietet eine vollständige Dokumentation für die Integration von gtest (Google Test) und gbenchmark (Google Benchmark) in ThemisDB, einschließlich Beispielen, Best Practices und Verwendungsmustern.

---

## Table of Contents / Inhaltsverzeichnis

1. [Current Integration Status](#1-current-integration-status)
2. [Google Test (gtest)](#2-google-test-gtest)
3. [Google Benchmark (gbenchmark)](#3-google-benchmark-gbenchmark)
4. [Installation](#4-installation)
5. [Examples](#5-examples)
6. [Best Practices](#6-best-practices)
7. [CI/CD Integration](#7-cicd-integration)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Current Integration Status

### 1.1 Google Test (gtest) ✅

**Status:** Fully integrated and operational

- **Location:** `tests/CMakeLists.txt`
- **Test Files:** 57+ test files
- **Framework:** Google Test 1.14.0
- **Integration:** CMake `find_package(GTest QUIET CONFIG)`

**Examples:**
- `tests/test_gpu_safe_fail.cpp` (15 tests)
- `tests/test_database_connection_manager.cpp` (20 tests)
- `tests/test_disk_space_monitor.cpp` (22 tests)
- `tests/test_rocksdb_wrapper_comprehensive.cpp`
- `tests/test_vector_index_comprehensive.cpp`

### 1.2 Google Benchmark (gbenchmark) ✅

**Status:** Fully integrated and operational

- **Location:** `benchmarks/performance_optimizations/CMakeLists.txt`
- **Benchmark Files:** 100+ benchmark files
- **Framework:** Google Benchmark 1.8.3
- **Integration:** CMake `find_package(benchmark CONFIG)`

**Examples:**
- `benchmarks/performance_optimizations/benchmark_mimalloc.cpp`
- `benchmarks/performance_optimizations/benchmark_huge_pages.cpp`
- `benchmarks/performance_optimizations/benchmark_rcu_index.cpp`
- `benchmarks/bench_crud.cpp`
- `benchmarks/bench_vector_search.cpp`

---

## 2. Google Test (gtest)

### 2.1 Quick Start

**Build and run tests:**
```bash
# Configure with tests enabled
cmake -B build -DTHEMIS_BUILD_TESTS=ON

# Build
cmake --build build

# Run all tests
cd build
ctest --output-on-failure

# Or run directly
./themis_tests
```

### 2.2 Writing Tests

**Basic Test Structure:**
```cpp
#include <gtest/gtest.h>
#include "your_module.h"

// Simple test
TEST(ModuleName, TestName) {
    EXPECT_EQ(2 + 2, 4);
    ASSERT_TRUE(true);
}

// Test with fixture
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
        obj_ = std::make_unique<MyClass>();
    }
    
    void TearDown() override {
        // Cleanup after each test
        obj_.reset();
    }
    
    std::unique_ptr<MyClass> obj_;
};

TEST_F(MyTest, UseFixture) {
    EXPECT_NE(obj_, nullptr);
    EXPECT_TRUE(obj_->isValid());
}
```

### 2.3 Common Assertions

```cpp
// Equality
EXPECT_EQ(actual, expected);      // ==
EXPECT_NE(actual, expected);      // !=

// Comparison
EXPECT_LT(val1, val2);            // <
EXPECT_LE(val1, val2);            // <=
EXPECT_GT(val1, val2);            // >
EXPECT_GE(val1, val2);            // >=

// Boolean
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// String comparison
EXPECT_STREQ(str1, str2);         // C-strings
EXPECT_STRNE(str1, str2);

// Floating point (with tolerance)
EXPECT_FLOAT_EQ(val1, val2);
EXPECT_DOUBLE_EQ(val1, val2);
EXPECT_NEAR(val1, val2, tolerance);

// Exceptions
EXPECT_THROW(code, exception_type);
EXPECT_NO_THROW(code);
EXPECT_ANY_THROW(code);

// ASSERT vs EXPECT:
// EXPECT: Continue after failure
// ASSERT: Stop immediately on failure
```

### 2.4 Running Tests

```bash
# Run all tests
./themis_tests

# Filter by name
./themis_tests --gtest_filter=*SafeFail*

# Filter by test case
./themis_tests --gtest_filter=GPUSafeFailTest.*

# Run with color
./themis_tests --gtest_color=yes

# Repeat tests
./themis_tests --gtest_repeat=10

# Shuffle order
./themis_tests --gtest_shuffle

# List tests
./themis_tests --gtest_list_tests

# Run specific test
./themis_tests --gtest_filter=GPUSafeFailTest.CircuitBreaker
```

### 2.5 Real Example from ThemisDB

**From `tests/test_gpu_safe_fail.cpp`:**

```cpp
#include <gtest/gtest.h>
#include "llm/gpu_safe_fail.h"

using namespace themis::llm;

class GPUSafeFailTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = GPUSafeFailManager::Config();
        config_.failure_threshold = 3;
        config_.circuit_reset_timeout = std::chrono::seconds(2);
    }
    
    GPUSafeFailManager::Config config_;
};

TEST_F(GPUSafeFailTest, InitialState) {
    GPUSafeFailManager manager(config_);
    EXPECT_TRUE(manager.isHealthy());
}

TEST_F(GPUSafeFailTest, CPUFallback) {
    GPUSafeFailManager manager(config_);
    
    auto result = manager.executeWithFallback(
        []() -> int { throw std::runtime_error("GPU failed"); },
        []() -> int { return 42; },
        "test_op"
    );
    
    EXPECT_EQ(result, 42);
}
```

---

## 3. Google Benchmark (gbenchmark)

### 3.1 Quick Start

**Build and run benchmarks:**
```bash
# Configure with benchmarks enabled
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON

# Build
cmake --build build

# Run benchmark
cd build/benchmarks
./benchmark_mimalloc
```

### 3.2 Writing Benchmarks

**Basic Benchmark Structure:**
```cpp
#include <benchmark/benchmark.h>
#include "your_module.h"

// Simple benchmark
static void BM_Operation(benchmark::State& state) {
    for (auto _ : state) {
        // Code to benchmark
        perform_operation();
    }
}
BENCHMARK(BM_Operation);

// Benchmark with arguments
static void BM_OperationWithSize(benchmark::State& state) {
    int size = state.range(0);
    std::vector<int> data(size);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(data);
        process(data);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_OperationWithSize)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// Benchmark with range
static void BM_RangeTest(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark code
    }
}
BENCHMARK(BM_RangeTest)->Range(8, 8<<10);  // 8, 16, 32, ..., 8192

// Benchmark with fixture
class MyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Setup
    }
};

BENCHMARK_F(MyFixture, TestOp)(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark code
    }
}

BENCHMARK_MAIN();
```

### 3.3 Benchmark Control

```cpp
// Prevent compiler optimization
benchmark::DoNotOptimize(value);
benchmark::ClobberMemory();

// Pause/Resume timing
state.PauseTiming();
// Setup code
state.ResumeTiming();

// Custom metrics
state.SetItemsProcessed(n);        // Items/sec
state.SetBytesProcessed(n);        // Bytes/sec
state.SetLabel("label");

// Access parameters
int size = state.range(0);
int64_t iterations = state.iterations();
```

### 3.4 Running Benchmarks

```bash
# Run benchmark
./benchmark_safe_fail

# Filter benchmarks
./benchmark_safe_fail --benchmark_filter=BM_GPU.*

# Repetitions for statistical confidence
./benchmark_safe_fail --benchmark_repetitions=10

# Output formats
./benchmark_safe_fail --benchmark_format=json > results.json
./benchmark_safe_fail --benchmark_format=csv > results.csv
./benchmark_safe_fail --benchmark_format=console

# Control timing
./benchmark_safe_fail --benchmark_min_time=5.0

# List benchmarks
./benchmark_safe_fail --benchmark_list_tests

# Show only aggregates (mean, median, stddev)
./benchmark_safe_fail --benchmark_report_aggregates_only=true
```

### 3.5 Real Example from ThemisDB

**Create `benchmarks/performance_optimizations/benchmark_safe_fail.cpp`:**

```cpp
#include <benchmark/benchmark.h>
#include "llm/gpu_safe_fail.h"
#include "storage/database_connection_manager.h"
#include "storage/disk_space_monitor.h"

using namespace themis;

// GPU Safe-Fail Benchmark
static void BM_GPUSafeFail_HealthCheck(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(manager.isHealthy());
    }
}
BENCHMARK(BM_GPUSafeFail_HealthCheck);

// Connection Manager Benchmark
class TestConnection : public storage::DatabaseConnectionManager::Connection {
public:
    bool isValid() const override { return true; }
    bool ping() override { return true; }
    std::chrono::steady_clock::time_point getLastUsed() const override {
        return std::chrono::steady_clock::now();
    }
};

class TestConnectionManager : public storage::DatabaseConnectionManager {
protected:
    std::shared_ptr<Connection> createConnection() override {
        return std::make_shared<TestConnection>();
    }
};

static void BM_ConnectionManager_AcquireRelease(benchmark::State& state) {
    storage::DatabaseConnectionManager::Config config;
    TestConnectionManager manager(config);
    
    for (auto _ : state) {
        auto conn = manager.acquireConnection();
        benchmark::DoNotOptimize(conn);
        manager.releaseConnection(conn, false);
    }
}
BENCHMARK(BM_ConnectionManager_AcquireRelease);

// Disk Monitor Benchmark
static void BM_DiskMonitor_CanWrite(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(monitor.canWrite(1024*1024));
    }
}
BENCHMARK(BM_DiskMonitor_CanWrite);

BENCHMARK_MAIN();
```

**Add to CMakeLists.txt:**
```cmake
if(THEMIS_BUILD_BENCHMARKS)
    add_perf_benchmark(benchmark_safe_fail benchmark_safe_fail.cpp)
endif()
```

---

## 4. Installation

### 4.1 Via vcpkg (Recommended)

```bash
# Install gtest
vcpkg install gtest

# Install benchmark
vcpkg install benchmark

# Verify
vcpkg list | grep gtest
vcpkg list | grep benchmark
```

### 4.2 Via System Package Manager

**Ubuntu/Debian:**
```bash
sudo apt-get install libgtest-dev libbenchmark-dev
```

**macOS:**
```bash
brew install googletest google-benchmark
```

### 4.3 Verify Integration

```bash
# Configure
cmake -B build \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Check output for:
# -- GTest found - building unified test binary
# -- Found Google Benchmark: ...
```

---

## 5. Examples

### 5.1 Complete Test Example

**File: `tests/test_example.cpp`**

```cpp
#include <gtest/gtest.h>
#include <vector>
#include <string>

// Test fixture
class VectorTest : public ::testing::Test {
protected:
    std::vector<int> vec;
    
    void SetUp() override {
        vec = {1, 2, 3, 4, 5};
    }
};

TEST_F(VectorTest, Size) {
    EXPECT_EQ(vec.size(), 5);
}

TEST_F(VectorTest, Access) {
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[4], 5);
}

// Parameterized test
class SquareTest : public ::testing::TestWithParam<std::pair<int, int>> {};

TEST_P(SquareTest, ComputeSquare) {
    auto [input, expected] = GetParam();
    EXPECT_EQ(input * input, expected);
}

INSTANTIATE_TEST_SUITE_P(
    SquareValues,
    SquareTest,
    ::testing::Values(
        std::make_pair(2, 4),
        std::make_pair(3, 9),
        std::make_pair(4, 16)
    )
);
```

### 5.2 Complete Benchmark Example

**File: `benchmarks/performance_optimizations/benchmark_example.cpp`**

```cpp
#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>

// Simple benchmark
static void BM_VectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v;
        v.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_VectorPushBack);

// Benchmark with arguments
static void BM_VectorSort(benchmark::State& state) {
    int size = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> v(size);
        for (int i = 0; i < size; ++i) {
            v[i] = size - i;
        }
        state.ResumeTiming();
        
        std::sort(v.begin(), v.end());
        benchmark::DoNotOptimize(v);
    }
    
    state.SetComplexityN(size);
}
BENCHMARK(BM_VectorSort)
    ->Range(8, 8<<10)
    ->Complexity();

BENCHMARK_MAIN();
```

---

## 6. Best Practices

### 6.1 Testing Best Practices

1. **One Assertion per Test (when possible)**
   ```cpp
   // Good
   TEST(Math, Addition) {
       EXPECT_EQ(2 + 2, 4);
   }
   
   TEST(Math, Subtraction) {
       EXPECT_EQ(5 - 3, 2);
   }
   
   // Avoid (multiple unrelated assertions)
   TEST(Math, Everything) {
       EXPECT_EQ(2 + 2, 4);
       EXPECT_EQ(5 - 3, 2);
       EXPECT_EQ(3 * 4, 12);
   }
   ```

2. **Use Fixtures for Setup**
   ```cpp
   class DatabaseTest : public ::testing::Test {
   protected:
       void SetUp() override {
           db_ = std::make_unique<Database>();
           db_->open("test.db");
       }
       
       void TearDown() override {
           db_->close();
           std::remove("test.db");
       }
       
       std::unique_ptr<Database> db_;
   };
   ```

3. **Test Edge Cases**
   ```cpp
   TEST(StringTest, EdgeCases) {
       EXPECT_EQ(process(""), "");           // Empty
       EXPECT_EQ(process("a"), "a");         // Single char
       EXPECT_NO_THROW(process(nullptr));    // Null pointer
   }
   ```

### 6.2 Benchmarking Best Practices

1. **Prevent Optimization**
   ```cpp
   static void BM_Example(benchmark::State& state) {
       for (auto _ : state) {
           auto result = compute();
           benchmark::DoNotOptimize(result);  // Prevent dead code elimination
           benchmark::ClobberMemory();         // Prevent memory optimization
       }
   }
   ```

2. **Setup Outside Loop**
   ```cpp
   static void BM_Example(benchmark::State& state) {
       // Setup once
       std::vector<int> data(1000000);
       
       for (auto _ : state) {
           // Only benchmark this
           process(data);
       }
   }
   ```

3. **Use Pause/Resume for Complex Setup**
   ```cpp
   static void BM_Example(benchmark::State& state) {
       for (auto _ : state) {
           state.PauseTiming();
           auto data = generateTestData();  // Not measured
           state.ResumeTiming();
           
           process(data);  // Measured
       }
   }
   ```

4. **Report Metrics**
   ```cpp
   static void BM_Example(benchmark::State& state) {
       int items_processed = 0;
       
       for (auto _ : state) {
           items_processed += process_batch();
       }
       
       state.SetItemsProcessed(items_processed);
       state.SetBytesProcessed(items_processed * sizeof(Item));
   }
   ```

---

## 7. CI/CD Integration

### 7.1 GitHub Actions

```yaml
name: Tests and Benchmarks

on: [push, pull_request]

jobs:
  test-and-benchmark:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup vcpkg
        run: |
          git clone https://github.com/Microsoft/vcpkg.git
          ./vcpkg/bootstrap-vcpkg.sh
      
      - name: Install dependencies
        run: |
          ./vcpkg/vcpkg install gtest benchmark
      
      - name: Configure
        run: |
          cmake -B build \
            -DTHEMIS_BUILD_TESTS=ON \
            -DTHEMIS_BUILD_BENCHMARKS=ON \
            -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
      
      - name: Build
        run: cmake --build build -j
      
      - name: Run Tests
        run: |
          cd build
          ctest --output-on-failure -j
      
      - name: Run Benchmarks
        run: |
          cd build/benchmarks
          ./benchmark_safe_fail --benchmark_format=json \
            --benchmark_out=results.json
      
      - name: Upload Results
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results
          path: build/benchmarks/results.json
```

### 7.2 CTest Configuration

```cmake
# In tests/CMakeLists.txt
enable_testing()

add_test(NAME AllTests COMMAND themis_tests)
add_test(NAME SafeFailTests 
         COMMAND themis_tests --gtest_filter=*SafeFail*)

set_tests_properties(AllTests PROPERTIES
    TIMEOUT 600
    LABELS "unit;integration"
)

set_tests_properties(SafeFailTests PROPERTIES
    TIMEOUT 120
    LABELS "safe-fail;unit"
)
```

**Run:**
```bash
cd build
ctest --output-on-failure
ctest -L safe-fail
ctest -j 4  # Parallel
```

---

## 8. Troubleshooting

### 8.1 Common Issues

**Issue: GTest not found**
```
CMake Warning: GTest not found

Solution:
vcpkg install gtest
# or
sudo apt-get install libgtest-dev
```

**Issue: Benchmark not found**
```
CMake Warning: Google Benchmark not found

Solution:
vcpkg install benchmark
# or
sudo apt-get install libbenchmark-dev
```

**Issue: Tests don't compile**
```
Error: undefined reference to `testing::Test::Test()'

Solution:
Check that GTest::gtest and GTest::gtest_main are linked:
target_link_libraries(test_target PRIVATE
    GTest::gtest
    GTest::gtest_main
)
```

**Issue: Benchmarks show inconsistent results**
```
Solution:
1. Use --benchmark_repetitions=10
2. Close other applications
3. Disable CPU frequency scaling:
   sudo cpupower frequency-set --governor performance
4. Check thermal throttling
```

### 8.2 Debugging

**Debug test failures:**
```bash
# Run with verbose output
./themis_tests --gtest_color=yes --gtest_print_time=1

# Run single test with debugging
gdb --args ./themis_tests --gtest_filter=MyTest.Fails
```

**Debug benchmark issues:**
```bash
# Verbose output
./benchmark_safe_fail --v=1

# Single benchmark
./benchmark_safe_fail --benchmark_filter=BM_Specific
```

---

## 9. Summary

### 9.1 Integration Status

| Framework | Status | Files | Usage |
|-----------|--------|-------|-------|
| **Google Test** | ✅ Integrated | 57+ | Unit & Integration tests |
| **Google Benchmark** | ✅ Integrated | 100+ | Performance benchmarks |

### 9.2 Key Files

**Testing:**
- Configuration: `tests/CMakeLists.txt`
- Examples: `tests/test_gpu_safe_fail.cpp`, `tests/test_database_connection_manager.cpp`

**Benchmarking:**
- Configuration: `benchmarks/performance_optimizations/CMakeLists.txt`
- Examples: `benchmarks/performance_optimizations/benchmark_*.cpp`

### 9.3 Quick Commands

```bash
# Build with tests and benchmarks
cmake -B build -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build

# Run tests
cd build && ctest --output-on-failure

# Run benchmarks
cd build/benchmarks && ./benchmark_safe_fail
```

### 9.4 Next Steps

1. ✅ GTest and gbenchmark are fully integrated
2. ✅ 57+ tests and 100+ benchmarks available
3. 📝 Add benchmark for safe-fail mechanisms (see section 3.5)
4. 📝 Expand test coverage for new features
5. 📝 Set up continuous benchmarking in CI/CD

---

## 10. References

- **Google Test Documentation:** https://google.github.io/googletest/
- **Google Benchmark Documentation:** https://github.com/google/benchmark
- **vcpkg:** https://vcpkg.io/
- **ThemisDB Repository:** https://github.com/makr-code/ThemisDB

---

**Document Status:** Complete ✅
**Last Updated:** 2026-04-06
**Maintained By:** ThemisDB Development Team
