# ThemisDB - Testing Guide

## Test Framework

ThemisDB uses **Google Test** for unit and integration testing.

## Running Tests

### All Tests

```bash
# CMake + CTest
cmake --build build --target test

# Or directly with CTest
cd build && ctest --output-on-failure
```

### Specific Test Suites

```bash
# Run specific test executable
./build/tests/storage_tests
./build/tests/aql_tests
./build/tests/vector_tests

# With Google Test filters
./build/tests/storage_tests --gtest_filter="RocksDB*"
```

### Critical Tests Only

```bash
# Windows
.\run_critical_tests.bat

# Linux
./run_critical_tests.sh
```

### Extended Test Suite

```bash
# Windows
.\run_extended_tests.bat

# Linux
./run_extended_tests.sh
```

## Test Categories

### Unit Tests

- Test individual components in isolation
- Fast execution (< 1 second per test)
- No external dependencies (mock/stub as needed)
- Location: `tests/unit/`

```cpp
TEST(VectorIndexTest, InsertAndSearch) {
    VectorIndex index(128);  // dimension
    
    std::vector<float> vec = generateRandomVector(128);
    index.insert(1, vec);
    
    auto results = index.search(vec, 5);  // top-5
    ASSERT_EQ(results[0].id, 1);
}
```

### Integration Tests

- Test interaction between components
- May use real dependencies (RocksDB, etc.)
- Slower execution (< 10 seconds per test)
- Location: `tests/integration/`

```cpp
TEST(DatabaseIntegrationTest, QueryWithIndex) {
    Database db("test_db");
    db.createTable("users", schema);
    db.createIndex("users", "name");
    
    db.insert("users", user_data);
    
    auto results = db.query("SELECT * FROM users WHERE name = 'Alice'");
    ASSERT_EQ(results.size(), 1);
}
```

### Benchmark Tests

- Performance measurement
- Uses Google Benchmark
- Location: `benchmarks/`

```cpp
static void BM_VectorSearch(benchmark::State& state) {
    VectorIndex index(128);
    prepareIndex(index, 10000);  // 10k vectors
    
    auto query = generateRandomVector(128);
    
    for (auto _ : state) {
        auto results = index.search(query, 10);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_VectorSearch);
```

## Test Requirements

### For New Features

When adding a new feature:

1. **Unit tests** for core functionality
2. **Integration tests** for component interaction
3. **Edge case tests** for boundary conditions
4. **Error handling tests** for failure modes

```cpp
// Example: Complete test coverage for a feature
TEST(FeatureTest, BasicFunctionality) { /* ... */ }
TEST(FeatureTest, EdgeCase_EmptyInput) { /* ... */ }
TEST(FeatureTest, EdgeCase_MaxSize) { /* ... */ }
TEST(FeatureTest, ErrorHandling_InvalidInput) { /* ... */ }
TEST(FeatureTest, ErrorHandling_OutOfMemory) { /* ... */ }
```

### For Bug Fixes

1. **Regression test** that reproduces the bug
2. Test should fail before fix, pass after fix

```cpp
// Example: Regression test for bug #123
TEST(RegressionTest, Issue123_MemoryLeakInSnapshotCleanup) {
    // Setup that triggers the bug
    Database db;
    auto snapshot = db.createSnapshot();
    
    // This should not leak memory
    snapshot.reset();
    
    // Verify cleanup (using memory profiling tools)
    ASSERT_NO_MEMORY_LEAKS();
}
```

## Code Coverage

### Generate Coverage Report

```bash
# Configure with coverage enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_ENABLE_COVERAGE=ON

# Build and run tests
cmake --build build
cd build && ctest

# Generate report (requires lcov/gcovr)
cmake --build build --target coverage
```

### Coverage Targets

- **Critical paths:** 90%+ coverage
- **Core modules:** 80%+ coverage
- **Overall project:** 70%+ coverage

### Focus Areas

Priority for test coverage:
1. Data integrity (storage, transactions)
2. Query correctness (AQL parser, executor)
3. Concurrency safety (MVCC, locks)
4. API contracts (public interfaces)

## Test Data Management

### Test Fixtures

Use fixtures for shared setup:

```cpp
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<Database>(test_db_path_);
        db_->createTable("test_table", schema_);
    }
    
    void TearDown() override {
        db_.reset();
        cleanupTestData();
    }
    
    std::unique_ptr<Database> db_;
    std::string test_db_path_ = "/tmp/test_db";
    Schema schema_ = createTestSchema();
};

TEST_F(DatabaseTest, InsertAndQuery) {
    // Use db_ directly
    db_->insert("test_table", data);
    auto results = db_->query("SELECT * FROM test_table");
    ASSERT_EQ(results.size(), 1);
}
```

### Test Data Location

- **Small test data:** `tests/data/`
- **Generated data:** Create in test setUp/fixture
- **Temporary data:** Use `/tmp/` or OS temp directory

## Mocking

### When to Mock

- External services (network, databases)
- Expensive operations (file I/O, computation)
- Non-deterministic behavior (time, random)

### Mock Example

```cpp
class MockVectorIndex : public VectorIndex {
public:
    MOCK_METHOD(SearchResults, search, 
                (const std::vector<float>&, int), 
                (override));
    MOCK_METHOD(void, insert, 
                (int, const std::vector<float>&), 
                (override));
};

TEST(QueryExecutorTest, UsesVectorIndex) {
    MockVectorIndex mock_index;
    QueryExecutor executor(&mock_index);
    
    EXPECT_CALL(mock_index, search(_, 10))
        .WillOnce(Return(mock_results));
    
    executor.execute("VECTOR SEARCH ...");
}
```

## Performance Testing

### Micro-Benchmarks

```cpp
BENCHMARK(BM_InsertOneMillionRecords)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(5.0);  // Run for at least 5 seconds
```

### Benchmark Targets

Document expected performance:

```cpp
// Target: < 10ms for 10k vector search
TEST(PerformanceTest, VectorSearchLatency) {
    auto start = std::chrono::high_resolution_clock::now();
    index.search(query, 10);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    EXPECT_LT(duration, 10) << "Search took " << duration << "ms";
}
```

## Continuous Integration

Tests run automatically on:
- Every push to feature branches
- Every pull request
- Scheduled nightly builds

### CI Test Configuration

See `.github/workflows/ci.yml` for CI setup.

## Troubleshooting

### Tests Hanging

```bash
# Run with timeout
ctest --timeout 30

# Or with specific test timeout
./build/tests/my_tests --gtest_timeout=10000  # 10 seconds
```

### Flaky Tests

- Identify and fix race conditions
- Use deterministic test data
- Add retries only as last resort

### Test Isolation

Each test should be independent:

```cpp
// ✅ Good: Clean state for each test
TEST_F(DatabaseTest, Test1) {
    db_->insert(...);  // Fresh db_ from setUp()
}

// ❌ Bad: Tests depend on execution order
TEST_F(DatabaseTest, Test2) {
    // Assumes Test1 already ran - BAD!
    db_->query(...);
}
```

## Documentation

For architecture-specific testing:
- Threading tests: Document in `docs/architecture.md`
- Concurrency tests: Document locking strategies

## Best Practices

1. **Fast tests:** Keep unit tests under 1 second
2. **Clear assertions:** Use descriptive messages
3. **One concept per test:** Test one thing at a time
4. **Arrange-Act-Assert:** Structure tests clearly
5. **No test logic:** Tests should be simple and obvious

```cpp
TEST(ExampleTest, DescriptiveName) {
    // Arrange: Set up test data
    Database db;
    auto data = createTestData();
    
    // Act: Perform the operation
    auto result = db.insert(data);
    
    // Assert: Verify the outcome
    ASSERT_TRUE(result.success);
    EXPECT_EQ(db.count(), 1);
}
```
