> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Integration Tests

This directory contains integration tests for ThemisDB. Integration tests verify that multiple components work together correctly end-to-end.

## Directory Structure

```
tests/integration/
├── test_fixture.h              # Base fixture for all integration tests
├── test_data_generator.h       # Utilities for generating test data
├── storage/                    # Storage layer integration tests
│   └── backup_recovery_integration_test.cpp
├── llm/                        # LLM integration tests
│   └── llm_inference_integration_test.cpp
├── rpc/                        # RPC service integration tests
│   └── rpc_service_integration_test.cpp
├── security/                   # Security integration tests
│   └── encryption_key_rotation_integration_test.cpp
├── end_to_end/                 # End-to-end workflow tests
│   └── full_query_flow_e2e_test.cpp
└── performance/                # Performance integration tests
```

## Test Organization

### Test Categories

1. **Storage Tests** (`storage/`)
   - Database backup and recovery
   - File I/O operations
   - RocksDB integration
   - Data persistence verification

2. **LLM Tests** (`llm/`)
   - Model loading and initialization
   - Inference execution
   - Embedding generation
   - Caching behavior

3. **RPC Tests** (`rpc/`)
   - gRPC service integration
   - Client-server communication
   - Authentication and authorization
   - Connection pooling

4. **Security Tests** (`security/`)
   - Encryption key rotation
   - Field-level encryption
   - Audit logging
   - Access control

5. **End-to-End Tests** (`end_to_end/`)
   - Complete query workflows
   - Multi-component interactions
   - Real-world usage scenarios
   - Performance under load

6. **Performance Tests** (`performance/`)
   - Latency benchmarks
   - Throughput testing
   - Scalability verification
   - Resource utilization

## Writing Integration Tests

### Using the Base Fixture

All integration tests should inherit from `IntegrationTestFixture`:

```cpp
#include "../test_fixture.h"
#include "../test_data_generator.h"

class MyIntegrationTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        // Additional setup
    }
};

TEST_F(MyIntegrationTest, TestSomething) {
    // Use GetTempDir() for temporary files
    auto temp_path = GetTempDir() / "test_db";
    
    // Use WaitForCondition() for async operations
    bool success = WaitForCondition(
        []() { return CheckCondition(); },
        std::chrono::seconds(10)
    );
    EXPECT_TRUE(success);
}
```

### Using the Test Data Generator

The `TestDataGenerator` provides utilities for creating test data:

```cpp
TestDataGenerator data_gen;

// Generate random string
auto str = data_gen.GenerateRandomString(10);

// Generate test documents
auto docs = data_gen.GenerateTestDocuments(100, "test_prefix");

// Generate encryption key
auto key = data_gen.GenerateEncryptionKey(32);
```

## Test Lifecycle

1. **SetUp()**: Called before each test
   - Creates temporary directory
   - Initializes test fixtures

2. **Test Execution**: Your test code runs

3. **TearDown()**: Called after each test
   - Cleans up temporary files
   - Releases resources

## Best Practices

### 1. Test Isolation
- Each test should be independent
- Don't rely on execution order
- Clean up resources in TearDown()

### 2. Use Temporary Directories
```cpp
auto db_path = CreateTestDbPath("my_test_db");
// Database will be automatically cleaned up
```

### 3. Handle Async Operations
```cpp
// Wait for condition with timeout
bool ready = WaitForCondition(
    [&]() { return server->IsReady(); },
    std::chrono::seconds(5)
);
ASSERT_TRUE(ready) << "Server failed to start";
```

### 4. Clear Test Names
```cpp
// Good: Clear what is being tested
TEST_F(MyTest, AuthenticatedUserCanQueryDatabase)

// Bad: Unclear test purpose
TEST_F(MyTest, Test1)
```

### 5. Comprehensive Assertions
```cpp
// Verify multiple aspects
EXPECT_TRUE(result.ok()) << "Operation failed: " << result.message;
EXPECT_EQ(result.data.size(), expected_size);
EXPECT_GT(result.execution_time, 0);
```

## Running Integration Tests

### Run All Integration Tests
```bash
cd build
ctest -R integration
```

### Run Specific Test Suite
```bash
ctest -R LLMInferenceIntegration
```

### Run with Verbose Output
```bash
ctest -R integration -V
```

### Run Performance Tests
```bash
ctest -R performance -V
```

## Test Coverage

Integration tests should aim for:
- **Functional Coverage**: All major workflows tested
- **Edge Cases**: Boundary conditions and error paths
- **Performance**: Latency and throughput benchmarks
- **Concurrency**: Race conditions and thread safety

## Current Status

### Implemented Tests
- ✅ Base test infrastructure (fixture, data generator)
- ✅ Test structure organized by component

### Planned Tests (Templates Created)
- 🔄 LLM inference integration
- 🔄 Encryption key rotation
- 🔄 RPC service end-to-end
- 🔄 Full query flow
- 🔄 Backup and recovery

### Coverage Goals
- Target: >80% integration test coverage
- High-risk areas: >90% coverage
- Performance benchmarks for all major operations

## Contributing

When adding new integration tests:

1. Choose appropriate directory (storage/, llm/, rpc/, etc.)
2. Inherit from `IntegrationTestFixture`
3. Use `TestDataGenerator` for test data
4. Follow naming convention: `component_feature_integration_test.cpp`
5. Add clear documentation and acceptance criteria
6. Update this README with new test information

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [Integration Testing Best Practices](https://martinfowler.com/bliki/IntegrationTest.html)
- ThemisDB Architecture Documentation: `../../docs/architecture.md`
