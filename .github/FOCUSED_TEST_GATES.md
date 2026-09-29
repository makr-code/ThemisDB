# Focused Test Gates and Benchmark Gates Architecture

## Overview

This document describes how ctest focus gates and benchmark validation gates are organized and adapted to the current ThemisDB build system.

## Test Target Classifications

### 1. Release-Critical Tests (30 marked)
**Purpose:** Core functionality validation for all changes
**Trigger:** PR gates (`gate-pr-core.yml`, `gate-pr-rag-*.yml`)
**Build Target:** `themis_release_critical_tests` (aggregate)
**CTest Filter:** `--label-regex "release_critical"`
**Scope:** Essential library paths, transaction handling, RAG evaluation

Example targets:
- Tests from `tests/` module-specific CMakeLists.txt
- Registered via: `set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS ...)`

### 2. Focused Module Tests (496 module-specific)
**Purpose:** Fast feedback on specific module changes
**Trigger:** PR lanes with path-based triggers (e.g., `build-clang-fast.yml` for distributed_tensor)
**Build Target:** Explicit target selection per workflow
**CTest Filter:** Regex matching (e.g., `-R "TensorShardSummaryPhaseCTests"`)
**Scope:** Module-level correctness, API contracts

Example targets:
- `module_epic3_distributed_tensor_tensor_shard_summary_focused`
- `module_epic3_distributed_tensor_distributed_planner_test_focused`
- `module_llm_test_llm_phase1_hardening_focused`

### 3. Benchmark Tests (19 aggregate targets)
**Purpose:** Performance regression detection
**Trigger:** Weekly schedule (`benchmark-performance-gate.yml`)
**Build Target:** `themis_benchmarks_all_eligible` (aggregate)
**Validation:** Compare against `PERFORMANCE_EXPECTATIONS.md` per module
**Scope:** Performance baselines, regression thresholds

Example aggregate targets:
- `themis_wave_a_server_llm_benchmarks`
- `themis_wave_a_query_storage_transaction_benchmarks`
- `run_benchmarks_quick` (for manual testing)

## Gate Workflows

### Ctest Validation Gates

| Workflow | Trigger | Target | Filter | Purpose |
|----------|---------|--------|--------|---------|
| `gate-pr-core.yml` | PR + dispatch | `themis_release_critical_tests` | `release_critical` | Core library validation |
| `gate-pr-rag-eval.yml` | PR + dispatch | Explicit targets | `release_critical` + regex "rag" | RAG metric validation |
| `build-clang-fast.yml` | PR (distributed_tensor) | Explicit targets | Regex | Fast feedback on tensor changes |
| `build-sanitizer-nightly.yml` | Nightly schedule | Explicit targets | Regex | Memory/UB detection |

### Benchmark Validation Gates

| Workflow | Trigger | Target | Validation | Purpose |
|----------|---------|--------|------------|---------|
| `benchmark-performance-gate.yml` | Weekly + dispatch | `themis_benchmarks_all_eligible` | PERF_EXPECTATIONS.md | Performance regression detection |

## Build System Adaptation (2026-09-29)

### Key Changes After build-mainline Refactoring

1. **Focused test gates remain independent** from `build-mainline.yml`
   - `build-mainline` runs full matrix via `release-build-matrix.yml`
   - Focused gates run separately for fast PR feedback
   - No cross-dependencies between flows

2. **Aggregate target construction**
   - Release-critical: global property collection in `tests/CMakeLists.txt`
   - Benchmarks: explicit targets in `benchmarks/CMakeLists.txt`
   - Both patterns properly maintained

3. **Library dependency handling**
   - `themis_base` library is a valid CMake target (created via `themis_add_module(base ...)`)
   - Explicitly building `themis_base` in focused test workflows ensures:
     - Core library is compiled first
     - All shared dependencies are resolved
     - Reduces parallel build ordering issues
   - Pattern is correct and recommended practice

## Testing the Gates Locally

### Release-Critical Tests
```bash
cmake -S . -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target themis_release_critical_tests --parallel 8
ctest --test-dir build --label-regex "release_critical" -v
```

### Module-Specific Focused Tests
```bash
cmake -S . -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target module_epic3_distributed_tensor_tensor_shard_summary_focused --parallel 8
ctest --test-dir build -R "TensorShardSummaryPhaseCTests" -v
```

### Benchmark Tests
```bash
cmake -S . -B build -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_BUILD_TESTS=OFF
cmake --build build --target themis_benchmarks_all_eligible --parallel 8
./build/benchmarks/run_benchmarks_quick 2>&1 | tee /tmp/benchmark_results.txt
```

## Registration Checklist

### For New Release-Critical Tests
1. Define test executable in module `CMakeLists.txt`
2. Register with property:
   ```cmake
   add_test(NAME MyTestName COMMAND my_test_executable)
   set_tests_properties(MyTestName PROPERTIES LABELS "release_critical;...")
   set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS my_test_executable)
   ```
3. Verify it appears in `themis_release_critical_tests` target build output

### For New Benchmark Tests
1. Define benchmark executable in `benchmarks/CMakeLists.txt`
2. Use Google Benchmark framework (BM_XXX format)
3. If aggregate-eligible, add to appropriate `themis_wave_*_benchmarks` target
4. Ensure module has `PERFORMANCE_EXPECTATIONS.md` with thresholds

## Status (2026-09-29)

✅ **Release-Critical Gates:** 30 tests registered, pattern correct
✅ **Focused Module Gates:** 496 unique test targets, independently validated  
✅ **Benchmark Gates:** 19 aggregate targets, performance tracking active
✅ **Naming Schema:** Updated to follow WORKFLOW_GUIDELINES.md Build:/Validate: convention
✅ **Build System Alignment:** Properly adapted after build-mainline refactoring

## Known Limitations

- Focused test targets are not exhaustive (519 total, many module-specific)
- Release-critical coverage focused on highest-priority paths
- Benchmark gates run weekly, not on every commit (performance cost)
- Some edge case validators (e.g., plugin boundary) have separate gate workflows
