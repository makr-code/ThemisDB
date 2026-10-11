---
Author: ThemisDB Contributors
Created: 2026-09-29
Last Updated: 2026-09-29
Status: active
---

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
| `gate-pr-benchmark-ctest.yml` | PR (benchmarks/\*\*) | N/A | Coverage audit | Benchmark CTest registration enforcement |
| `build-clang-fast.yml` | PR (distributed_tensor) | Explicit targets | Regex | Fast feedback on tensor changes |
| `build-sanitizer-nightly.yml` | Nightly schedule | Explicit targets | Regex | Memory/UB detection |

### Benchmark Validation Gates

| Workflow | Trigger | Target | Validation | Purpose |
|----------|---------|--------|------------|---------|
| `benchmark-performance-gate.yml` | Weekly + dispatch | `themis_benchmarks_all_eligible` | PERF_EXPECTATIONS.md | Performance regression detection |
| `gate-pr-benchmark-ctest.yml` | PR (benchmarks/) | N/A | CTest registration audit | Benchmark discovery and registration enforcement |

#### Benchmark CTest Coverage Gate (2026-10-11)

**Purpose:** Enforce that all benchmark sources are registered in CMake and CTest, preventing benchmark fragmentation and ensuring consistent CI coverage.

**Trigger:** PR changes to `benchmarks/`, `CMakeLists.txt`, or `.github/workflows/gate-pr-benchmark-ctest.yml`

**Validation Method:**
1. Run `audit_benchmark_coverage.py` to discover all sources under `benchmarks/`
2. Validate 3-state chain: Source → CMake Target → CTest Registration
3. Calculate coverage: (CTest registered) / (CMake targets with source mapping) × 100%

**Gate Criteria:**
- **PASS:** Coverage ≥95% AND unregistered sources = 0
- **WARN:** Orphaned CTest entries > 0 (variant/smoke tests, non-blocking)
- **FAIL:** Coverage < 95% OR unregistered sources > 0 OR broken targets > 0

**Actions:**
- Applies PR labels: `quality/benchmarks-pass`, `quality/benchmarks-warn`, `quality/benchmarks-fail`
- Creates tracker issue on FAIL with remediation steps
- Blocks merge on FAIL status

**Baseline (2026-10-11):** 401 benchmarks registered, 103% coverage (includes smoke test variants)

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

### Benchmark CTest Coverage Audit (Local)
```bash
# Run the audit tool to validate benchmark registration status
python3 tools/ci/audit_benchmark_coverage.py --output /tmp/audit_report.json

# View results
cat /tmp/audit_report.json | python3 -m json.tool

# Expected output for current state:
# - Sources discovered: 388
# - CMake targets: 401
# - CTest registered: 401 (103% coverage including variants)
# - Unregistered sources: 0 ✅
# - Coverage: 103% ✅ (passes ≥95% threshold)
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

## Status (2026-10-11)

✅ **Release-Critical Gates:** 30 tests registered, pattern correct
✅ **Focused Module Gates:** 496 unique test targets, independently validated  
✅ **Benchmark Performance Gates:** 19 aggregate targets, performance tracking active
✅ **Benchmark CTest Coverage Gate:** 401 benchmarks registered, 103% coverage (NEW 2026-10-11)
✅ **Naming Schema:** Updated to follow WORKFLOW_GUIDELINES.md Build:/Validate: convention
✅ **Build System Alignment:** Properly adapted after build-mainline refactoring

## Known Limitations

- Focused test targets are not exhaustive (496 focused module targets; broader CTest registry also includes non-focused targets)
- Release-critical coverage focused on highest-priority paths
- Benchmark performance gates run weekly, not on every commit (performance cost)
- Benchmark CTest coverage gate treats orphaned/variant entries (smoke tests, wave patterns) as non-blocking warnings
- Some edge case validators (e.g., plugin boundary) have separate gate workflows
