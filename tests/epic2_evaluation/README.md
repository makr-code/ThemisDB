# EPIC 2 evaluation tests

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Planned test files

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

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
