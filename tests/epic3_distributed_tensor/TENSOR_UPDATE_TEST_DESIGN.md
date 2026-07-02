# Tensor Update Test Design And Coverage Matrix

**Status:** Active (Phase 1)  
**Effective Date:** 2026-07-02  
**Scope:** CTest coverage for dynamic tensor-update paths in the Tensor-Graph architecture

---

## 1. Goal

This document defines comprehensive CTest coverage for dynamic tensor-update behavior in ThemisDB.

Design principle:

**Tests validate correctness and state consistency of tensor delta logging, manifest transitions, update worker lifecycle, planner compatibility, and snapshot rebuild paths.**

---

## 2. Test Coverage Matrix

### 2.1 Correctness Dimension

Tests that validate functional correctness of tensor update paths.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_delta_log` | append_delta_after_insert | Delta logging on INSERT | Delta logged, sequence advanced |
| `test_tensor_delta_log` | append_delta_after_update | Delta logging on UPDATE | Delta logged, sequence advanced |
| `test_tensor_delta_log` | append_delta_after_delete | Delta logging on DELETE | Delta logged, sequence advanced |
| `test_tensor_delta_log` | rollback_does_not_publish | Rollback suppresses delta | Delta not published to manifest |
| `test_tensor_delta_log` | delta_sequence_monotonic | Sequence ordering | Sequence numbers strictly increasing |
| `test_tensor_delta_log` | delta_source_tracking | Source tracking | Source reference maintained |

### 2.2 Manifest Freshness / State Handling Dimension

Tests that validate manifest lifecycle, freshness assessment, and state transitions.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_manifest` | manifest_publish_atomic | Manifest publish | Atomic visibility to all consumers |
| `test_tensor_manifest` | manifest_state_created | CREATED → ACTIVE | Verification gates state transition |
| `test_tensor_manifest` | manifest_state_active_to_stale | ACTIVE → STALE | Staleness threshold exceeded |
| `test_tensor_manifest` | manifest_state_stale_usable | Stale manifest usable | Advisory queries allowed on stale |
| `test_tensor_manifest` | manifest_state_rebuild | STALE → REBUILT → ACTIVE | Rebuild refreshes manifest |
| `test_tensor_manifest` | manifest_state_invalidate | ACTIVE/STALE → INVALIDATED | Corruption or consistency failure |
| `test_tensor_manifest` | manifest_freshness_score | Freshness calculation | Score ∈ [0.0, 1.0], monotonically decreases |
| `test_tensor_manifest` | manifest_staleness_threshold | Staleness gating | isStale() returns true when exceeded |

### 2.3 Snapshot Rebuild Consistency Dimension

Tests that validate snapshot rebuild and artifact consistency.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_snapshot_consistency` | snapshot_extraction_snapshot | Extract snapshot | Snapshot reflects manifest state at extraction time |
| `test_tensor_snapshot_consistency` | snapshot_rebuild_deterministic | Rebuild determinism | Identical snapshot + delta → identical artifact |
| `test_tensor_snapshot_consistency` | snapshot_multipart_consistency | Multi-part rebuild | All parts rebuild to consistent whole |
| `test_tensor_snapshot_consistency` | snapshot_partial_refit | Partial refit path | Subset of artifact refitted correctly |
| `test_tensor_snapshot_consistency` | snapshot_refit_vs_rebuild | Refit vs rebuild quality | Quality bounds maintained |
| `test_tensor_snapshot_consistency` | snapshot_invalid_artifact_rejected | Invalid artifact rejected | Integrity check prevents use of corrupted artifact |

### 2.4 Update Worker Behavior Dimension

Tests that validate worker lifecycle, state machine, and error handling.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_update_worker` | worker_lifecycle_start | Worker startup | Worker enters ready state |
| `test_tensor_update_worker` | worker_lifecycle_process | Worker process loop | Processes queued updates sequentially |
| `test_tensor_update_worker` | worker_patch_small_delta | Patch small delta | Delta applied successfully |
| `test_tensor_update_worker` | worker_refit_escalate | Refit escalation | Failed refit escalates to rebuild |
| `test_tensor_update_worker` | worker_rebuild_full | Full rebuild | Complete artifact regenerated |
| `test_tensor_update_worker` | worker_crash_resumable | Crash recovery | Resumes from last safe checkpoint |
| `test_tensor_update_worker` | worker_rank_growth_sensitivity | Rank growth | Worker adapts to rank changes |
| `test_tensor_update_worker` | worker_residual_tracking | Residual tracking | Quality metrics updated post-update |

### 2.5 Planner Compatibility Dimension

Tests that validate query planner interaction with tensor artifacts.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_planner_policy` | planner_stale_vs_fresh | Freshness gating | Planner uses fresh artifact when available |
| `test_tensor_planner_policy` | planner_staleness_threshold_respected | Staleness gate | Stale artifact rejected when threshold exceeded |
| `test_tensor_planner_policy` | planner_advisory_vs_exact | Advisory/Exact semantics | Advisory artifact not used for exact correctness |
| `test_tensor_planner_policy` | planner_fallback_on_stale | Fallback on stale | Exact graph fallback when tensor stale and mandatory |
| `test_tensor_planner_policy` | planner_freshness_score_correlation | Freshness score used | Score influences routing decisions |
| `test_tensor_planner_policy` | planner_rebuilt_artifact_accepted | Rebuilt artifact | REBUILT → ACTIVE transition accepted |

### 2.6 Distributed Shard Summary Dimension

Tests that validate distributed summary consistency and correctness.

| Test Suite | Test Case | Scenario | Expected Behavior |
|---|---|---|---|
| `test_tensor_shard_summary` | summary_atomic_publish | Atomic summary publish | All shards see consistent summary |
| `test_tensor_shard_summary` | summary_stale_detection | Stale summary detection | Old summaries rejected by freshness gate |
| `test_tensor_shard_summary` | summary_inconsistent_rejection | Inconsistent summaries | Cross-shard conflicts detected and rejected |
| `test_tensor_shard_summary` | summary_truth_bearing_validated | Truth-bearing summary | Integrity verified before use |
| `test_tensor_shard_summary` | summary_advisory_only_allowed | Advisory-only summary | Advisory summaries used for hints only |
| `test_tensor_shard_summary` | summary_routing_quality | Routing quality | Fan-out reduction measured |

---

## 3. Test Execution Policy

### 3.1 Test Registration

All test suites are registered in `tests/epic3_distributed_tensor/CMakeLists.txt`:

```cmake
set(TENSOR_UPDATE_TESTS
    test_tensor_delta_log.cc
    test_tensor_manifest.cc
    test_tensor_update_worker.cc
    test_tensor_snapshot_consistency.cc
    test_tensor_planner_policy.cc
    test_tensor_shard_summary.cc
)

foreach(test_src IN LISTS TENSOR_UPDATE_TESTS)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE themis_distributed_tensor GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
```

### 3.2 CTest Presets

Integrated into `CTEST.md` under T2 (Data Plane Engines) tier:

```bash
ctest --preset windows-release --output-on-failure -R "^test_tensor_(delta_log|manifest|update_worker|snapshot_consistency|planner_policy|shard_summary)$"
```

### 3.3 Smoke-Test Policy

All tests are short-running (<5s each) and suitable for CI smoke-test integration:
- Minimal external dependencies (no network, minimal I/O)
- Deterministic execution
- No hardware-specific requirements

---

## 4. Test Implementation Conventions

### 4.1 Fixture Structure

Each test suite uses a consistent fixture pattern:

```cpp
namespace themis {
namespace distributed_tensor {

class TensorUpdateTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize artifact manifest
        // Initialize tensor metadata
        // Create mock/test data
    }

    void TearDown() override {
        // Clean up state
    }

    // Helper methods for test support
};

} // namespace distributed_tensor
} // namespace themis
```

### 4.2 Assertion Patterns

Tests follow standard GTest patterns:

- `EXPECT_*` for correctness assertions (continue on failure)
- `ASSERT_*` for precondition assertions (abort on failure)
- `EXPECT_EQ`, `EXPECT_TRUE`, `EXPECT_THAT` for standard assertions
- Custom matchers for complex state validations

### 4.3 Test Naming Convention

Test names follow GTest pattern:

```
TEST_F(FixtureName, TestCaseName)
```

Example:
```cpp
TEST_F(TensorDeltaLogTest, AppendDeltaAfterInsert) { ... }
TEST_F(TensorManifestTest, ManifestStateCreatedToActive) { ... }
```

---

## 5. Dependencies And Linking

### 5.1 Required Libraries

- `GTest::gtest_main` - Test framework
- `themis_distributed_tensor` - Tensor infrastructure (artifact manifest, artifact classes, shard placement)
- `themis_core` - Core dependencies (logging, time utilities)

### 5.2 Header Includes

Tests include:

```cpp
#include <gtest/gtest.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include "distributed_tensor/include/tensor_artifact_classes.h"
#include "distributed_tensor/include/shard_placement.h"
```

---

## 6. Metrics And Validation

### 6.1 Test Metrics

Each test suite reports:

- **Coverage:** Test case count, assertion count per scenario
- **Execution Time:** Per-test duration (target <5s)
- **Determinism:** Pass rate on repeated runs (target 100%)

### 6.2 Validation Checkpoints

Before test acceptance:

- [ ] All test cases pass on windows-release preset
- [ ] All test cases pass on linux-release preset
- [ ] No flaky failures (run 5x with 100% pass rate)
- [ ] Execution time <5s per test
- [ ] No external I/O or network dependencies
- [ ] Code review approval from @makr-code

---

## 7. Acceptance Criteria

✅ **Test Coverage Complete When:**
- [ ] All 6 test suites implemented and passing
- [ ] Coverage matrix dimensions covered (36 test scenarios minimum)
- [ ] Smoke-test policy met (all tests <5s)
- [ ] CTest preset configured and documented
- [ ] No flaky failures on repeated runs
- [ ] Code reviewed and merged to develop

---

## 8. References

- `src/distributed_tensor/include/artifact_manifest.h` - Manifest schema and API
- `src/distributed_tensor/include/tensor_artifact_classes.h` - Artifact lifecycle and classification
- `tests/epic3_distributed_tensor/artifact_manifest_test.cc` - Existing manifest tests (reference)
- `tests/epic3_distributed_tensor/tensor_artifact_classes_test.cc` - Existing artifact tests (reference)
- `DISTRIBUTED_TENSOR_SHARDING.md` - Tensor architecture and update semantics
- `CTEST.md` - CTest inventory and registration policy
