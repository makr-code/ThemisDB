# EPIC 3 distributed tensor benchmarks

<!-- Status: current | phase-5 benchmark suite implemented | validated: 2026-07-13 -->

## Implemented benchmark files

- `placement_strategy_bench.cc`
- `integrity_verification_bench.cc`
- `recovery_rebuild_bench.cc`
- `distributed_retrieval_bench.cc`

## Deterministic benchmark assets

- `phase5_workload_profiles.json` — canonical workload inventory and seed usage
- `release_gate_manifest_epic3.json` — measurable Phase 5 gates and pass/fail rules
- `report_variance_epic3.py` — compare captured result bundles against the gate manifest
- `RUNBOOK_EPIC3_PHASE5.md` — execution checklist once runtime benchmarks land
- `REPRO_TRIAGE_EPIC3_PHASE5.md` — failure triage for degraded-mode and recovery regressions

## Measurement goals

- Capture placement, integrity, recovery, and retrieval overhead for issue #5428
- Keep topology size, shard count, and degraded-node assumptions explicit for every workload profile
- Use canonical seed `42` for reproducible artifact layouts and retry schedules
- Preserve reproducible workload classes for cross-epic planner comparisons
- Carry variance and gate output forward into Phase 6 acceptance documentation

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`
- `src/distributed_tensor/PERFORMANCE_EXPECTATIONS.md`
- `src/distributed_tensor/ROADMAP.md`

## Installation

This directory now contributes focused Google Benchmark targets when
`THEMIS_BUILD_BENCHMARKS=ON` and benchmark dependencies are available.

## Usage

Use this README together with the local `CMakeLists.txt` and EPIC 3 docs to
extend measured baselines without introducing scaffold-only code paths.

