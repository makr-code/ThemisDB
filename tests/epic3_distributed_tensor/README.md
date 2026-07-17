# EPIC 3 distributed tensor tests

<!-- Status: current | phase-4 contract coverage + phase-a manifest tests active | validated: 2026-07-17 -->

## Implemented test files

- `test_phase3_failure_semantics.cpp`
- `test_phase4_contract_coverage.cpp`
- `test_manifest_store_phase_a.cpp`
- `integrity_verification_test.cc`
- `integrity_verification_bench.cc`

## Remaining planned expansion files

- `test_tensor_delta_log.cpp`
- `test_tensor_rebuild_fallback.cpp`
- `test_tensor_shard_summary.cpp`

## Coverage goals

- Focused Phase 3 safety gates are active in CTest
- Phase 4 contract and fault-path coverage is active through `test_phase4_contract_coverage.cpp`
- Phase A ManifestStore advisory-only behavior is covered by `MS-01..12`
- Benchmark comparisons live in the matching `benchmarks/` epic directory

## Reference

- `docs/TESTING_STRATEGY.md`

## Installation

This directory now builds the focused EPIC 3 regression targets via the repository
test harness. Phase 4 broadened contract coverage and Phase A ManifestStore tests are now part of
the active test set; additional delta-log and shard-summary scenarios remain queued for later phases.

## Usage

Use this README together with the local `CMakeLists.txt` to extend EPIC 3 focused
tests without reintroducing scaffold-only placeholders.
