# Dynamic Tensor Update Tests & Benchmarks: Quick Start Guide

## Overview

This guide helps you quickly understand and use the tensor update test and benchmark suites implemented in Phase 1-3.

---

## What's New?

### 6 New Test Suites
Located in: `tests/epic3_distributed_tensor/`

1. **test_tensor_delta_log** - Delta logging correctness
2. **test_tensor_manifest** - Manifest lifecycle & freshness
3. **test_tensor_update_worker** - Worker operations & lifecycle
4. **test_tensor_snapshot_consistency** - Rebuild determinism
5. **test_tensor_planner_policy** - Query planner integration
6. **test_tensor_shard_summary** - Distributed consistency

### 5 New Benchmark Suites
Located in: `benchmarks/`

1. **bench_tensor_commit_overhead** - Write-path performance
2. **bench_tensor_update_worker** - Update throughput
3. **bench_tensor_query_routing** - Routing quality
4. **bench_tensor_cpu_gpu_break_even** - GPU utility analysis
5. **bench_tensor_snapshot_rebuild** - Rebuild latency

---

## Quick Start (5 minutes)

### Step 1: Build Tests
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset windows-release
cmake --build --preset windows-release --target themis_tests
```

### Step 2: Run All Tensor Tests
```bash
ctest --preset windows-release \
  -R "^test_tensor_" \
  --output-on-failure
```

### Step 3: Run Specific Test Suite
```bash
# Run only delta logging tests
ctest --preset windows-release \
  -R "test_tensor_delta_log" \
  --verbose

# Run only planner policy tests
ctest --preset windows-release \
  -R "test_tensor_planner_policy" \
  --verbose
```

### Step 4: Build Benchmarks
```bash
cmake --build --preset windows-release --target bench_tensor_commit_overhead
cmake --build --preset windows-release --target bench_tensor_update_worker
cmake --build --preset windows-release --target bench_tensor_query_routing
cmake --build --preset windows-release --target bench_tensor_cpu_gpu_break_even
cmake --build --preset windows-release --target bench_tensor_snapshot_rebuild
```

### Step 5: Run Benchmarks
```bash
# Run one benchmark
./bin/bench_tensor_commit_overhead

# Run all with standard settings
./bin/bench_tensor_update_worker --benchmark_min_time=0.1
```

---

## Test File Index

| File | Tests | Purpose |
|------|-------|---------|
| test_tensor_delta_log.cc | 12 | Delta recording, rollback, sequence ordering |
| test_tensor_manifest.cc | 22 | Manifest state machine, freshness scoring |
| test_tensor_update_worker.cc | 13 | Worker lifecycle, path selection, crash recovery |
| test_tensor_snapshot_consistency.cc | 15 | Snapshot extraction, rebuild determinism |
| test_tensor_planner_policy.cc | 20 | Freshness gating, fallback behavior, semantics |
| test_tensor_shard_summary.cc | 16 | Atomic publish, consistency, routing quality |

---

## Benchmark File Index

| File | Benchmarks | Purpose |
|------|-----------|---------|
| bench_tensor_commit_overhead.cpp | 5 | Measure write-path overhead with delta logging |
| bench_tensor_update_worker.cpp | 8 | Measure update path throughput (patch/refit/rebuild) |
| bench_tensor_query_routing.cpp | 7 | Measure routing quality and freshness impact |
| bench_tensor_cpu_gpu_break_even.cpp | 8 | Identify GPU utility thresholds |
| bench_tensor_snapshot_rebuild.cpp | 9 | Measure snapshot and rebuild latency |

---

## Example: Running a Single Test

```bash
# Build
cmake --preset windows-release
cmake --build --preset windows-release --target test_tensor_manifest

# Run
ctest --preset windows-release -R "test_tensor_manifest" --verbose

# Expected output:
# DeltaLogging_AppendOnInsert ... PASSED
# DeltaLogging_AppendOnUpdate ... PASSED
# DeltaLogging_AppendOnDelete ... PASSED
# ... (22 tests total)
```

---

## Example: Running a Benchmark

```bash
# Build
cmake --build --preset windows-release --target bench_tensor_cpu_gpu_break_even

# Run
./bin/bench_tensor_cpu_gpu_break_even

# Expected output:
# Benchmark                                      Time           CPU      Iterations
# DensitySweep_CPU_Compute/1                    X.XXms       X.XXms           XXX
# DensitySweep_CPU_Compute/2                    X.XXms       X.XXms           XXX
# RankSweep_GPU_Compute/1                       X.XXms       X.XXms           XXX
# ... (various parametrized sweeps)
```

---

## Understanding the Test Coverage

### ✅ Delta Logging (6 scenarios)
- Tests that changes are properly logged
- Tests that rollbacks don't create delta records
- Tests that delta sequence is monotonic
- Tests source tracking

### ✅ Manifest State (8 scenarios)
- Tests lifecycle transitions
- Tests freshness scoring
- Tests staleness detection
- Tests publish atomicity

### ✅ Worker Operations (8 scenarios)
- Tests worker startup/shutdown
- Tests path selection (patch/refit/rebuild)
- Tests rank growth handling
- Tests crash recovery

### ✅ Snapshot Consistency (6 scenarios)
- Tests snapshot extraction
- Tests rebuild determinism
- Tests consistency under partial refit
- Tests integrity validation

### ✅ Planner Integration (6 scenarios)
- Tests freshness gating
- Tests fallback on stale manifests
- Tests advisory/exact semantics
- Tests planner confidence scoring

### ✅ Distributed Summaries (6 scenarios)
- Tests atomic visibility
- Tests stale detection
- Tests consistency validation
- Tests routing quality metrics

---

## Understanding the Benchmarks

### Commit Overhead
Measures how much overhead delta logging adds to write operations:
```
Baseline RocksDB transaction
+ Delta logging
+ Manifest invalidation
+ Batch variations
+ Payload sweeps
```

### Worker Throughput
Measures update operation performance under different conditions:
```
Patch path (small deltas)
Refit path (partial rebuild)
Full rebuild path
Rank growth sensitivity
Mixed workload routing
```

### Query Routing
Measures routing quality with different freshness conditions:
```
ANN-only vs ANN+Tensor vs ANN+Tensor+Graph
Summary-first vs direct exact graph fetch
Fresh vs stale artifact impact
Fan-out reduction measurement
```

### CPU/GPU Break-Even
Identifies when GPU is beneficial:
```
Batch size sweep (1-512)
Density sweep (10%-90%)
Rank sweep (1-64)
Transfer overhead analysis
Break-even point identification
```

### Snapshot Rebuild
Measures snapshot and rebuild performance:
```
Snapshot extraction cost
Rebuild latency by size
Artifact publish/swap cost
End-to-end workflow measurement
```

---

## Key Design Files

- **Test Design Spec:** `tests/epic3_distributed_tensor/TENSOR_UPDATE_TEST_DESIGN.md`
  - Coverage matrix
  - Naming conventions
  - Fixture patterns
  - Execution policy

- **Benchmark Design Spec:** `benchmarks/TENSOR_UPDATE_BENCHMARK_DESIGN.md`
  - Benchmark matrix
  - Output format
  - Baseline management
  - Integration strategy

---

## Debugging a Test

If a test fails:

1. **Run with verbose output:**
```bash
ctest --preset windows-release -R "test_name" --verbose
```

2. **Run directly (if built as executable):**
```bash
./bin/test_tensor_delta_log --gtest_filter="DeltaLogging_AppendOnInsert"
```

3. **Check the test file:**
The test will show which assertion failed. Look at the test code:
```bash
cat tests/epic3_distributed_tensor/test_tensor_delta_log.cc
```

4. **Common failure types:**
- **State transition:** Check artifact_manifest.h for lifecycle rules
- **Freshness:** Check staleness threshold constants
- **Consistency:** Check mock implementations for determinism
- **Planner:** Check query planner integration interface

---

## Adding a New Test

To add a new test to an existing suite:

1. Edit the corresponding `.cc` file
2. Add a new TEST_F method to the fixture class
3. Follow naming convention: `FixtureName_CaseName`
4. Use EXPECT_* and ASSERT_* appropriately
5. Run: `ctest --preset windows-release -R "new_test_name"`

Example:
```cpp
TEST_F(DeltaLoggingFixture, AppendDeltaWithComplexData) {
  // Setup
  ManifestRecord record = CreateTestManifest();
  
  // Execute
  ASSERT_TRUE(logger.AppendDelta(record));
  
  // Verify
  EXPECT_EQ(logger.GetSequenceNumber(), 1);
  EXPECT_TRUE(logger.Verify());
}
```

---

## Performance Baseline Tracking

Benchmarks output performance metrics that can be tracked over time:

```bash
# First run establishes baseline
./bin/bench_tensor_commit_overhead > baseline_initial.txt

# Later run compares against baseline
./bin/bench_tensor_commit_overhead > baseline_current.txt

# Compare results
diff baseline_initial.txt baseline_current.txt
```

See `benchmarks/TENSOR_UPDATE_BENCHMARK_DESIGN.md` for baseline management strategy.

---

## Documentation References

| Document | Purpose |
|----------|---------|
| TENSOR_UPDATE_TEST_DESIGN.md | Test coverage matrix, fixtures, conventions |
| TENSOR_UPDATE_BENCHMARK_DESIGN.md | Benchmark strategy, metrics, baselines |
| TENSOR_UPDATE_IMPLEMENTATION_SUMMARY.md | Complete implementation overview |
| TENSOR_UPDATE_PHASE_3_COMPLETION_STATUS.md | Phase completion summary |
| tests/epic3_distributed_tensor/README.md | Test suite inventory & execution |
| benchmarks/README.md | Benchmark inventory & execution |

---

## Next Steps

1. ✅ Build and run tests (verify compilation)
2. ✅ Run smoke-tests to validate correctness
3. 📋 Phase 4: Add failure handling & edge cases
4. 📋 Phase 5: Establish performance baselines
5. 📋 Phase 6: Document acceptance criteria
6. 📋 Phase 7: Integrate into CI/CD pipeline

---

## Getting Help

- **Test Failures:** Check test design spec and mock implementations
- **Benchmark Issues:** Review benchmark design and baseline management
- **Build Problems:** Ensure CMakePresets.json is properly configured
- **Integration:** See CTEST.md for CTest configuration

---

**Last Updated:** 2026-07-02  
**Status:** Phase 1-3 Complete, Ready for Phase 4
