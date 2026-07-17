# EPIC 2 evaluation benchmarks

<!-- Status: current | phase-5 benchmark baseline implemented | validated: 2026-07-13 -->

## Implemented benchmark files

- `benchmark_matrix_bench.cc`
- `planner_decision_bench.cc`
- `storage_strategy_bench.cc`

## Measurement goals

- Capture planner, placement, integrity, and recovery overhead for issue #5428
- Keep shard count, topology size, and degraded-mode assumptions explicit
- Preserve reproducible scenario matrices for cross-epic EPIC 2 / EPIC 3 comparisons

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`

## Installation

This directory now contributes focused Google Benchmark targets when
`THEMIS_BUILD_BENCHMARKS=ON` and benchmark dependencies are available.

## Usage

Use this README together with `docs/EPIC2_BENCHMARK_FRAMEWORK.md` to extend
planner and storage-strategy baselines without introducing scaffold-only code.
