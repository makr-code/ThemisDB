# EPIC 3 distributed tensor tests

<!-- Status: current | phase-3 focused coverage | validated: 2026-07-13 -->

## Implemented test files

- `test_phase3_failure_semantics.cpp`

## Planned expansion files

- `tensor_artifact_classes_test.cc`
- `artifact_manifest_test.cc`
- `shard_placement_test.cc`
- `integrity_verification_test.cc`
- `recovery_manager_test.cc`
- `distributed_planner_test.cc`
- `tensor_infrastructure_test.cc`

## Coverage goals

- Focused Phase 3 safety gates are active in CTest
- Expand to per-component contract tests next
- Benchmark comparisons live in the matching `benchmarks/` epic directory
- Reserve hardening scenarios for degraded mode, retry/recovery, and integrity under load

## Reference

- `docs/TESTING_STRATEGY.md`

## Installation

This directory now builds the focused Phase 3 regression target via the repository
test harness. Additional fault-path and integration scenarios remain queued for
Phase 4 expansion.

## Usage

Use this README together with the local `CMakeLists.txt` to extend EPIC 3 focused
tests without reintroducing scaffold-only placeholders.
