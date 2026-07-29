# EPIC 2 evaluation tests

<!-- Status: current | partial implementation | validated: 2026-07-29 -->

## Test files

- `hardware_profile_test.cc`
- `query_planner_test.cc`
- `benchmark_matrix_test.cc`
- `retrieval_metrics_test.cc`
- `approximation_rules_test.cc`
- `ablation_framework_test.cc`
- `artifact_lifecycle_test.cc`
- `test_query_planner_cache.cc`

## Coverage goals

- Contract tests first
- Integration scenarios once public interfaces stabilize
- Benchmark comparisons live in the matching `benchmarks/` epic directory

## Reference

- `docs/TESTING_STRATEGY.md`

## Installation

The local `CMakeLists.txt` wires active focused and GTest-based EPIC 2 coverage behind
the same target-availability gates used by the module libraries. Current issue status
still requires refreshed executable evidence, and not every source file listed above is
currently registered as an executable test target.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt`
to review the active hardware-profile and hybrid-planner coverage and stage the
remaining EPIC 2 tests.
