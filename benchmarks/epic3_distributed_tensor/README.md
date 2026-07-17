# EPIC 3 distributed tensor benchmarks

<!-- Status: current | phase-5 benchmark suite implemented | validated: 2026-07-13 -->

## Implemented benchmark files

- `placement_strategy_bench.cc`
- `integrity_verification_bench.cc`
- `recovery_rebuild_bench.cc`
- `distributed_retrieval_bench.cc`

## Measurement goals

- Capture placement, integrity, recovery, and retrieval overhead for issue #5428
- Keep topology size, shard count, and degraded-mode assumptions explicit
- Preserve reproducible workload classes for cross-epic planner comparisons

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`

## Installation

This directory now contributes focused Google Benchmark targets when
`THEMIS_BUILD_BENCHMARKS=ON` and benchmark dependencies are available.

## Usage

Use this README together with the local `CMakeLists.txt` and EPIC 3 docs to
extend measured baselines without introducing scaffold-only code paths.
