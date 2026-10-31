---
Author: "ThemisDB Contributors"
Created: "2026-07-28"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Module Tests

## Overview

Focused unit test suite for the distributed knowledge federation module, covering cross-shard capability exchange, federated coordination, merge orchestration, feedback synchronization, and privacy-aware distillation.

**Current source inventory:** 107 GoogleTest declarations across 9 translation units. This is a static source count, not evidence that tests passed.

**Location:** This directory (`tests/distributed_knowledge/`)

## Current Test Source Inventory

The directory CMake file discovers `test_*.cpp`; the current source set is:

| Source file | Declared cases | Scope and evidence boundary |
|---|---:|---|
| `test_adapter_capability_announcement_focused.cpp` | 16 | Capability announcements and trust-policy interface cases |
| `test_lora_federation_coordinator_focused.cpp` | 12 | LoRA aggregation coordinator |
| `test_federated_rag_merger_focused.cpp` | 12 | Federated RAG merge |
| `test_cross_shard_feedback_sync_focused.cpp` | 12 | Feedback publish/receive, dedup, and policy callbacks |
| `test_federated_distillation_coordinator_focused.cpp` | 15 | Distillation and bounded-policy cases |
| `test_dk_contract_hardening_focused.cpp` | 16 | Contract checks; several cases model behavior with local mocks/simulations |
| `test_dk_diagnostics_focused.cpp` | 14 | Diagnostic emitter and hardening-policy semantics |
| `test_distributed_knowledge_llm_focused.cpp` | 7 | RAG merge and prompt-context path |
| `test_distributed_knowledge_highcardinality_stress.cpp` | 3 | In-process stub stress; not real shard or module-runtime validation |
| **Total** | **107** | Static count of `TEST`/`TEST_F` declarations |

## Test Structure

### Original component-suite summaries

1. **test_adapter_capability_announcement_focused.cpp** (16 declared cases; ACA-01..ACA-10 plus inbound and trust-policy cases)
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

5. **test_federated_distillation_coordinator_focused.cpp** (15 declared cases; FDC-01..FDC-12 plus bounded-policy cases)
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
- Target: `module_distributed_knowledge_test_adapter_capability_announcement_focused`
- CTest Name: `test_adapter_capability_announcement_distributed_knowledge_FocusedTests`

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
cmake --build .  # or select an exact target listed in TEST_INVENTORY.md
```

### Run All Distributed Knowledge Tests

```bash
cd build-community-release
ctest -L distributed_knowledge --output-on-failure
```

### Run Specific Test Suite

```bash
# Run only adapter capability announcement tests
ctest -R "test_adapter_capability_announcement_distributed_knowledge_FocusedTests"

# Run only LoRA federation coordinator tests
ctest -R "test_lora_federation_coordinator_distributed_knowledge_FocusedTests"
```

### Run Single Test

```bash
# Run specific test directly
./bin/module_distributed_knowledge_test_adapter_capability_announcement_focused
```

## Test Coverage

### Original five-suite coverage summary

| Priority | Coverage | Test IDs |
|----------|----------|----------|
| Timeout/partial-failure semantics | Partial | Direct timeout/partial-response cases are in LFC and FRM; no real peer topology is exercised |
| Benchmark execution/baseline | Not established | See benchmark sources; unit tests do not establish benchmark results |
| Diagnostics behavior | Separate suite | `test_dk_diagnostics_focused.cpp` covers the event emitter |
| Policy-edge semantics | Some cases | FDC policy/bounded-policy and CSS inbound-policy cases; trust-policy interface cases in ACA |
| Deterministic behavior | Scenario assertions | Does not establish production concurrency or distributed determinism |

### Test Summary

| Suite | Focus Area | Test Count | Key Scenarios |
|-------|-----------|-----------|----------------|
| ACA | Capability Announcement | 16 | Gossip publish, domain types, inbound dispatch, trust-policy behavior |
| LFC | LoRA Aggregation | 12 | Round lifecycle, participant thresholds, timeouts |
| FRM | RAG Merge | 12 | Merge strategy, dedup, partial failures, top-K |
| CSS | Feedback Sync | 12 | Batch handling, dedup, replay detection, privacy |
| FDC | Distillation | 15 | DP params, policy gates, budget enforcement |

**Subtotal for these five source files: 67 declarations.** The four supplemental files in the current inventory above bring the source total to 107.

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
module_distributed_knowledge_test_my_surface_focused -> test_my_surface_distributed_knowledge_FocusedTests
```

## Test Maintenance

### Current Status

- **Created:** 2026-07-28
- **Current source inventory:** 107 declarations across 9 files; source inventory only, not a test result
- **Build Integration:** Configured per module test pattern
- **Status:** CMake registration is present; build and execution were not performed for this audit

### Known Limitations

1. **Environment-Dependent Build:**
   - Full build requires: RocksDB, fmt, spdlog, nlohmann_json, GTest, mimalloc
   - Test sources and CMake registration are present; compilation is not established by this inventory
   - This documentation audit did not execute a build

2. **Unit-Level Tests Only:**
   - No real shard-topology test source is present in this directory; broader integration evidence may exist elsewhere and was not assessed here
   - Property-based stress tests are not present in the current source inventory
   - Benchmark source targets exist; successful execution and baseline-comparison evidence are not established by this test-source inventory

3. **Simulation Mode:**
   - Contract-hardening cases use local mocks/simulations; the high-cardinality stress file explicitly uses stubs
   - These test sources do not validate production behavior across real shards

### Next Steps

1. **Build Verification:** Execute CMake configure and build when dependencies available
2. **Test Execution:** Run full test suite to generate coverage reports
3. **Integration Tests:** Establish whether real shard-topology coverage exists elsewhere; add coverage if it does not
4. **Performance Benchmarks:** Run the mapped benchmark targets and record their results against the release baseline
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
