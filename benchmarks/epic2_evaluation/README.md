# EPIC 2 evaluation benchmarks

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Planned benchmark files

- `hardware_profile_bench.cc`
- `benchmark_matrix_bench.cc`
- `metrics_computation_bench.cc`
- `planner_decision_bench.cc`
- `artifact_staleness_bench.cc`
- `storage_strategy_bench.cc`

## Measurement goals

- Capture latency, throughput, and resource cost for the matching sub-issues
- Keep hardware profile, dataset, and planner assumptions explicit
- Store enough metadata to compare approximation strategies across runs

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`

## Installation

This directory is a documentation-first scaffold. No additional build step is required until the planned files move into implementation.

## Usage

Use this README together with the matching epic document and local `CMakeLists.txt` placeholder to create issues, review file ownership, and stage implementation work without enabling production targets yet.
