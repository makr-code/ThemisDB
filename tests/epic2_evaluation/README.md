# EPIC 2 evaluation tests

<!-- Status: current | partial implementation | validated: 2026-07-15 -->

## Test files

- `hardware_profile_test.cc`
- `benchmark_matrix_test.cc`
- `evaluation_metrics_test.cc`
- `approximation_rules_test.cc`
- `query_planner_test.cc`
- `artifact_lifecycle_test.cc`
- `storage_strategy_test.cc`

## Coverage goals

- Contract tests first
- Integration scenarios once public interfaces stabilize
- Benchmark comparisons live in the matching `benchmarks/` epic directory

## Reference

- `docs/TESTING_STRATEGY.md`

## Installation

`hardware_profile_test.cc` is active and wired through the local `CMakeLists.txt`.
The remaining planned files stay deferred until their matching EPIC 2 surfaces exist.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt`
to review the active hardware-profile coverage and stage the remaining EPIC 2 tests.
