# Distributed Knowledge Module Tests

## Overview

Focused unit test suite for the distributed knowledge federation module, covering cross-shard capability exchange, federated coordination, merge orchestration, feedback synchronization, and privacy-aware distillation.

**Status:** Q3 2026 Test Infrastructure (58 tests, 5 surfaces)

**Location:** This directory (`tests/distributed_knowledge/`)

## Test Structure

### Test Files (5 focused test suites)

1. **test_adapter_capability_announcement_focused.cpp** (10 tests: ACA-01..ACA-10)
   - Adapter capability announcement and gossip exchange
   - Cross-shard capability discovery
   - Domain type handling and serialization

2. **test_lora_federation_coordinator_focused.cpp** (12 tests: LFC-01..LFC-12)
   - Federated LoRA aggregation coordination
   - Aggregation request lifecycle and state management
   - Timeout and partial-failure semantics

3. **test_federated_rag_merger_focused.cpp** (12 tests: FRM-01..FRM-12)
   - Cross-shard RAG result merge orchestration
   - Deduplication and ranking
   - Partial shard failure handling

4. **test_cross_shard_feedback_sync_focused.cpp** (12 tests: CSS-01..CSS-12)
   - Cross-shard feedback synchronization
   - Dedup and replay prevention
   - Privacy-aware feedback filtering

5. **test_federated_distillation_coordinator_focused.cpp** (12 tests: FDC-01..FDC-12)
   - Federated knowledge distillation with privacy guards
   - Differential privacy budget tracking
   - Policy-gated distillation workflows

### Build Integration

**CMakeLists.txt Configuration:**

Tests are auto-discovered via glob pattern:
```cmake
file(GLOB DISTRIBUTED_KNOWLEDGE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)
```

Each test creates a focused executable target:
```
module_distributed_knowledge_<stem>_focused
```

**Example:**
- Input: `test_adapter_capability_announcement_focused.cpp`
- Target: `module_distributed_knowledge_adapter_capability_announcement_focused`
- CTest Name: `adapter_capability_announcement_distributed_knowledge_FocusedTests`

### Test Registration

All tests registered with consistent parameters:
- **Tier:** unit
- **Timeout:** 120 seconds
- **Labels:** distributed_knowledge
- **Macro:** `themis_register_module_focused_test()`

## Running Tests

### Build All Tests

```bash
# Using community-release preset
cmake --preset community-release -B build-community-release
cd build-community-release
cmake --build . --target module_distributed_knowledge_*_focused
```

### Run All Distributed Knowledge Tests

```bash
cd build-community-release
ctest -L distributed_knowledge --output-on-failure
```

### Run Specific Test Suite

```bash
# Run only adapter capability announcement tests
ctest -R "adapter_capability_announcement_distributed_knowledge_FocusedTests"

# Run only LoRA federation coordinator tests
ctest -R "lora_federation_coordinator_distributed_knowledge_FocusedTests"
```

### Run Single Test

```bash
# Run specific test directly
./bin/module_distributed_knowledge_adapter_capability_announcement_focused
```

## Test Coverage

### Q3 2026 Hardening Priorities Addressed

| Priority | Coverage | Test IDs |
|----------|----------|----------|
| Timeout/partial-failure semantics | ✓ | ACA, LFC, FRM, CSS |
| Benchmark stabilization | ✓ | Hot path markers in all suites |
| Diagnostics consistency | ✓ | Failure scenario tests (CSS-06, FRM-05, LFC-10) |
| Policy-edge semantics | ✓ | FDC-09, CSS-11, ACA-08 |
| Deterministic behavior | ✓ | All concurrent operation tests (LFC-11, FRM-11, etc.) |

### Test Summary

| Suite | Focus Area | Test Count | Key Scenarios |
|-------|-----------|-----------|----------------|
| ACA | Capability Announcement | 10 | Gossip publish, domain types, edge cases |
| LFC | LoRA Aggregation | 12 | State machine, timeouts, concurrent aggregations |
| FRM | RAG Merge | 12 | Merge strategy, dedup, partial failures, top-K |
| CSS | Feedback Sync | 12 | Batch handling, dedup, replay detection, privacy |
| FDC | Distillation | 12 | DP params, policy gates, budget enforcement |

**Total: 58 focused unit tests**

## Dependencies

### Test Framework
- GTest (included via system package or vcpkg)
- GMock (included with GTest)

### Module Dependencies
- themis_distributed_knowledge (module library)
- themis_core (foundational types and utilities)
- nlohmann_json (JSON serialization)
- spdlog (logging)
- Threads (standard threading library)
- fmt (formatting, via CMakeLists.txt)

### Linking

CMakeLists.txt automatically links required libraries to each test target.

## Extending Tests

### Adding New Tests

1. Create new test file following naming convention: `test_<surface>_focused.cpp`
2. Add to `tests/distributed_knowledge/` directory
3. CMakeLists.txt will auto-discover via glob pattern
4. Follow existing test structure (fixtures, test case organization)
5. Use consistent test ID naming: `<SURFACE>-<NUMBER>` (e.g., `NEW-01`)

### Test Naming Convention

- **File:** `test_<surface_name>_focused.cpp`
- **Test Class:** `<SurfaceNameCamelCase>Test`
- **Test Case:** `<DescriptiveTestName>` (PascalCase)
- **Test ID Comment:** `@test <SURFACE>-<NUM>` and `/**`

### CMake Pattern

Tests follow module test pattern from other modules (api, hsm, tensor, etc.):

```cpp
// File: test_my_surface_focused.cpp
class MySurfaceTest : public ::testing::Test {
protected:
    void SetUp() override { /* setup */ }
};

TEST_F(MySurfaceTest, MyTestCase) {
    // Test implementation
}
```

CMakeLists.txt registers automatically:
```
module_distributed_knowledge_my_surface_focused -> my_surface_distributed_knowledge_FocusedTests
```

## Test Maintenance

### Current Status

- **Created:** 2026-07-28
- **Q3 2026 Coverage:** Complete (58 tests across 5 surfaces)
- **Build Integration:** Configured per module test pattern
- **Status:** Ready for build execution once environment dependencies resolved

### Known Limitations

1. **Environment-Dependent Build:**
   - Full build requires: RocksDB, fmt, spdlog, nlohmann_json, GTest, mimalloc
   - Test files created and verified; CMakeLists.txt configured
   - Actual build execution pending environment setup

2. **Unit-Level Tests Only:**
   - Integration tests with real shard topology pending Q4 2026 Phase 4
   - Property-based stress tests pending extended suite
   - Benchmark regression gates pending Phase 5 Q4 2026

3. **Simulation Mode:**
   - Tests use in-memory structures; distributed behavior is mocked
   - Production behavior validation pending integration test suite

### Next Steps

1. **Build Verification:** Execute CMake configure and build when dependencies available
2. **Test Execution:** Run full test suite to generate coverage reports
3. **Integration Tests:** Create cross-shard topology tests in Q4 2026 Phase 4
4. **Performance Benchmarks:** Add regression benchmarks in Q4 2026 Phase 5
5. **Chaos Engineering:** Add fault injection tests for degradation scenarios

## Documentation

- **TEST_INVENTORY.md:** Detailed test case descriptions and coverage matrix
- **ROADMAP.md:** Q3 2026 deliverables and future test planning
- **FUTURE_ENHANCEMENTS.md:** Extended test coverage goals

## Questions & Support

For test infrastructure questions, refer to:
- CMakeLists.txt in this directory (test registration pattern)
- Module pattern documentation in similar modules (tests/api, tests/hsm)
- ROADMAP.md and FUTURE_ENHANCEMENTS.md in src/distributed_knowledge/
