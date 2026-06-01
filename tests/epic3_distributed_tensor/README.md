# EPIC 3 distributed tensor tests

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Planned test files

- `tensor_artifact_classes_test.cc`
- `artifact_manifest_test.cc`
- `shard_placement_test.cc`
- `integrity_verification_test.cc`
- `recovery_manager_test.cc`
- `distributed_planner_test.cc`
- `tensor_infrastructure_test.cc`

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
