> **Aktueller Build-Flow:** `cmake --preset linux-release && cmake --build --preset linux-release`

> **Aktueller Test-Flow:** `cmake --preset linux-release && ctest --preset linux-release`

# Integration Test Guidelines

## Purpose

This document provides guidelines for writing, organizing, and maintaining integration tests for ThemisDB.

## What Are Integration Tests?

Integration tests verify that multiple components work together correctly. Unlike unit tests that test individual functions in isolation, integration tests:

- Test complete workflows across multiple components
- Use real dependencies (databases, files, network)
- Verify end-to-end functionality
- May have longer execution times
- Require setup and teardown of complex state

## Test Organization Principles

### Directory Structure

```
tests/integration/
├── pipeline/      # Cross-module end-to-end pipeline tests (offline mocks)
├── storage/        # Storage layer (RocksDB, backups, file I/O)
├── llm/           # LLM model loading, inference, caching
├── rpc/           # gRPC services, authentication, networking
├── security/      # Encryption, key rotation, audit logging
├── end_to_end/    # Multi-component workflows
└── performance/   # Benchmarks and load tests
```

### Naming Conventions

- **File**: `{component}_{feature}_integration_test.cpp`
  - Example: `backup_recovery_integration_test.cpp`

- **Test Suite**: `{Component}{Feature}IntegrationTest`
  - Example: `BackupRecoveryIntegrationTest`

- **Test Case**: `{Action}{Scenario}`
  - Example: `FullBackupAndRestore`

### Pipeline Test Naming and IDs

- **Pipeline File**: `{pipeline_name}_pipeline_test.cpp`
  - Examples:
    - `query_execution_pipeline_test.cpp`
    - `transaction_replication_pipeline_test.cpp`
- **Pipeline Test Case IDs**:
  - Query: `QP-01..QP-05`
  - Ingestion: `IP-01..IP-04`
  - RAG/AI: `RAG-01..RAG-04`
  - Transaction/Replication: `TXR-01..TXR-04`
  - Security: `SEC-01..SEC-06`
  - Analytics/Export: `AEP-01..AEP-03`
  - Application Profile E2E: `APP-01..APP-13`
  - Wave 6 — Critical Journey Hardening: `RCJ-01..RCJ-08` (`w6a`)
  - Wave 6 — Stress/Soak/Stability: `SSS-01..SSS-08` (`w6b`)
  - Wave 6 — Failure Injection/Recovery: `FIR-01..FIR-08` (`w6c`)
- **CTest Label**: `pipeline_integration`
- **Wave 6 CTest Labels**: `wave6;w6a;release_candidate` / `wave6;w6b;stress_soak` / `wave6;w6c;failure_injection`
- **Expectation**: Pipeline tests must run offline with deterministic mocks (no GPU, no external LLM service, no Kafka dependency).

### Shared Pipeline Test Helpers

`IntegrationTestFixture` now provides reusable helpers for cross-module pipeline tests:

- `CreateInMemoryStorage()`
- `CreateMockIndex()`
- `CreateMockAuth()`
- `CreateMockLlmBackend()`
- `CreateAuditLog()`

`TestDataGenerator` now provides pipeline-oriented builders:

- `GeneratePipelineToken()`
- `GenerateAqlQuery()`
- `GenerateTerms()`
- `GenerateEmbedding()`
- `GenerateCdcEvent()`

## Writing Integration Tests

### 1. Use the Base Fixture

```cpp
#include "../test_fixture.h"
#include "../test_data_generator.h"

class MyIntegrationTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }
    
    std::unique_ptr<TestDataGenerator> data_gen_;
};
```

### 2. Structure Tests with Clear Acceptance Criteria

```cpp
/**
 * @test Verify complete authenticated query flow
 * 
 * Acceptance Criteria:
 * - Client authenticates successfully
 * - Query is executed end-to-end
 * - Results are returned correctly
 * - Audit log records the operation
 */
TEST_F(MyIntegrationTest, AuthenticatedQueryFlow) {
    // Step 1: Setup
    auto db = CreateTestDatabase();
    
    // Step 2: Action
    auto result = db->Query("SELECT * FROM test");
    
    // Step 3: Verification
    EXPECT_TRUE(result.ok());
    EXPECT_GT(result.data.size(), 0);
    
    // Step 4: Additional checks
    auto audit = db->GetAuditLog();
    EXPECT_GT(audit.size(), 0);
}
```

### 3. Use Descriptive Steps

```cpp
TEST_F(MyIntegrationTest, ComplexWorkflow) {
    // Step 1: Create test database with data
    auto db_path = CreateTestDbPath("workflow_test");
    auto test_data = data_gen_->GenerateTestDocuments(100);
    
    // Step 2: Perform operation A
    auto result_a = PerformOperationA(db_path, test_data);
    ASSERT_TRUE(result_a.ok()) << "Operation A failed";
    
    // Step 3: Verify intermediate state
    EXPECT_EQ(GetRecordCount(db_path), 100);
    
    // Step 4: Perform operation B
    auto result_b = PerformOperationB(db_path);
    ASSERT_TRUE(result_b.ok()) << "Operation B failed";
    
    // Step 5: Verify final state
    VerifyFinalState(db_path);
}
```

### 4. Handle Async Operations

```cpp
TEST_F(MyIntegrationTest, AsyncOperation) {
    auto server = StartAsyncServer();
    
    // Wait for server to be ready
    bool ready = WaitForCondition(
        [&]() { return server->IsReady(); },
        std::chrono::seconds(10)
    );
    ASSERT_TRUE(ready) << "Server failed to start within timeout";
    
    // Proceed with test
    auto result = server->ProcessRequest(request);
    EXPECT_TRUE(result.ok());
}
```

### 5. Clean Up Resources

```cpp
class MyIntegrationTest : public IntegrationTestFixture {
protected:
    void TearDown() override {
        // Clean up custom resources
        if (server_) {
            server_->Shutdown();
        }
        
        // Base cleanup (temp directories, etc.)
        IntegrationTestFixture::TearDown();
    }
    
    std::unique_ptr<Server> server_;
};
```

## Test Data Management

### Using TestDataGenerator

```cpp
TestDataGenerator data_gen;

// Random strings
auto username = data_gen.GenerateRandomString(10);

// Random integers
auto user_id = data_gen.GenerateRandomInt(1000, 9999);

// Test documents
auto docs = data_gen.GenerateTestDocuments(100, "test_prefix");

// Encryption keys
auto key = data_gen.GenerateEncryptionKey(32);
```

### Creating Realistic Test Data

```cpp
// Good: Realistic test scenario
auto user = CreateUser("test_user", "test@example.com");
auto documents = LoadDocumentsFromFile("test_data/sample_documents.json");

// Avoid: Unrealistic data
auto user = CreateUser("x", "y");
auto documents = {{"id": 1}};
```

## Error Handling in Tests

### 1. Expect Errors When Appropriate

```cpp
TEST_F(MyIntegrationTest, InvalidInputHandling) {
    auto db = CreateTestDatabase();
    
    // Should fail with invalid query
    auto result = db->Query("INVALID SQL SYNTAX");
    EXPECT_FALSE(result.ok());
    EXPECT_THAT(result.error, HasSubstr("syntax error"));
}
```

### 2. Use Assertions for Setup

```cpp
TEST_F(MyIntegrationTest, TestFeature) {
    auto db = CreateTestDatabase();
    ASSERT_TRUE(db != nullptr) << "Failed to create test database";
    
    auto result = db->Query("SELECT 1");
    EXPECT_TRUE(result.ok());  // Test can continue if this fails
}
```

### 3. Provide Helpful Error Messages

```cpp
// Good: Informative message
EXPECT_EQ(result.size(), expected_size) 
    << "Expected " << expected_size << " records but got " << result.size();

// Bad: No context
EXPECT_EQ(result.size(), expected_size);
```

## Performance Considerations

### 1. Set Reasonable Timeouts

```cpp
// Set test timeout in CMakeLists.txt
set_tests_properties(my_integration_test PROPERTIES TIMEOUT 300)

// Or use WaitForCondition with appropriate timeout
bool ready = WaitForCondition(
    [&]() { return condition(); },
    std::chrono::seconds(30)  // Adjust based on expected operation time
);
```

### 2. Parallelize When Possible

```cpp
// Tests that don't share state can run in parallel
TEST_F(MyIntegrationTest, IndependentTest1) { /* ... */ }
TEST_F(MyIntegrationTest, IndependentTest2) { /* ... */ }

// Mark serial tests explicitly if needed
// (use test properties in CMakeLists.txt)
```

### 3. Use Appropriate Data Sizes

```cpp
// Good: Reasonable test data size
auto test_data = data_gen_->GenerateTestDocuments(100);

// Avoid: Excessive data that slows tests
auto test_data = data_gen_->GenerateTestDocuments(1000000);
```

## Testing Edge Cases

### 1. Boundary Conditions

```cpp
TEST_F(MyIntegrationTest, EmptyDatabase) {
    auto db = CreateTestDatabase();
    auto result = db->Query("SELECT * FROM empty_table");
    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.size(), 0);
}

TEST_F(MyIntegrationTest, SingleRecord) {
    auto db = CreateTestDatabase();
    db->Insert(CreateTestRecord());
    auto result = db->Query("SELECT * FROM table");
    EXPECT_EQ(result.size(), 1);
}
```

### 2. Error Conditions

```cpp
TEST_F(MyIntegrationTest, NetworkFailure) {
    auto server = StartServer();
    SimulateNetworkFailure();
    
    auto result = client->SendRequest(request);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error_code, ErrorCode::NETWORK_ERROR);
}
```

### 3. Resource Exhaustion

```cpp
TEST_F(MyIntegrationTest, LowMemoryCondition) {
    SetMemoryLimit(100_MB);
    
    auto result = PerformMemoryIntensiveOperation();
    // Should handle gracefully, not crash
    EXPECT_TRUE(result.ok() || result.error_code == ErrorCode::OUT_OF_MEMORY);
}
```

## Concurrent Testing

### 1. Test Thread Safety

```cpp
TEST_F(MyIntegrationTest, ConcurrentWrites) {
    auto db = CreateTestDatabase();
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            auto result = db->Insert(CreateTestRecord(i));
            if (result.ok()) {
                success_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, 10);
    EXPECT_EQ(db->GetRecordCount(), 10);
}
```

## Coverage Goals

### Target Coverage by Component

| Component | Target Coverage | Priority |
|-----------|----------------|----------|
| Storage Layer | >80% | High |
| Query Engine | >80% | High |
| RPC Service | >70% | Medium |
| Security | >90% | Critical |
| LLM Integration | >70% | Medium |

### Measuring Coverage

```bash
# Build with coverage preset
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release -R integration

# Run only the cross-module pipeline suite
ctest --preset linux-release -L pipeline_integration

# Generate integration test coverage report
./scripts/integration_test_coverage.sh

# View HTML report
firefox build/coverage_integration/html/index.html
```

> <!-- TODO: verify against current source – coverage script path may differ -->

## CI/CD Integration

Integration tests should:
1. Run on every PR to develop
2. Run nightly for performance tests
3. Block merges if critical tests fail
4. Report coverage metrics

## Troubleshooting Test Failures

### 1. Flaky Tests

```cpp
// Use proper synchronization
bool ready = WaitForCondition(
    [&]() { return IsReady(); },
    std::chrono::seconds(10)
);

// Avoid: Sleep-based timing
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

### 2. Test Isolation

```cpp
// Ensure each test uses unique resources
auto db_path = CreateTestDbPath("test_" + GetTestName());

// Avoid: Shared resources between tests
auto db_path = "/tmp/shared_test_db";  // Bad!
```

### 3. Debugging Failed Tests

```cpp
TEST_F(MyIntegrationTest, DebugExample) {
    auto result = PerformOperation();
    
    // Add diagnostic output for failures
    if (!result.ok()) {
        std::cout << "Operation failed:" << std::endl;
        std::cout << "  Error: " << result.error << std::endl;
        std::cout << "  State: " << GetCurrentState() << std::endl;
    }
    
    EXPECT_TRUE(result.ok());
}
```

## Best Practices Summary

✅ **DO**:
- Inherit from `IntegrationTestFixture`
- Use `TestDataGenerator` for test data
- Write clear acceptance criteria
- Test error conditions
- Clean up resources in TearDown()
- Use appropriate timeouts
- Add diagnostic output for failures

❌ **DON'T**:
- Share state between tests
- Use hardcoded paths or ports
- Ignore test failures
- Create excessive test data
- Skip error handling
- Use sleep for synchronization
- Leave resources uncleaned

## Further Reading

- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Integration Testing Best Practices](https://martinfowler.com/bliki/IntegrationTest.html)
- ThemisDB Test Coverage Report: `tests/TEST_COVERAGE_REPORT.md`
