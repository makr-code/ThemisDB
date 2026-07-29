# EPIC 2 evaluation benchmarks

<!-- Status: current | phase-5 benchmark sources present | validated: 2026-07-29 -->

## Implemented benchmark files

- `benchmark_matrix_bench.cc`
- `planner_decision_bench.cc`
- `artifact_staleness_bench.cc`
- `storage_strategy_bench.cc`

## Measurement goals

- Capture planner, placement, integrity, and recovery overhead for issue #5428
- Keep shard count, topology size, and degraded-mode assumptions explicit
- Preserve reproducible scenario matrices for cross-epic EPIC 2 / EPIC 3 comparisons

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`

## Installation

This directory contains the module's benchmark sources. Registered benchmark targets are
still gated by `THEMIS_BUILD_BENCHMARKS=ON` and dependency availability, and some source
files remain follow-up measurement work rather than currently wired executables. Current
issue status still requires refreshed executable benchmark evidence for closure.

## Usage

Use this README together with `docs/EPIC2_BENCHMARK_FRAMEWORK.md` to extend
planner and storage-strategy baselines without introducing scaffold-only code.
