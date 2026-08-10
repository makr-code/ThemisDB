# EPIC 3 distributed tensor benchmarks

<!-- Status: current | phase-5 benchmark package implemented, measurement evidence pending | validated: 2026-08-10 -->

## Implemented benchmark files

- `placement_strategy_bench.cc`
- `integrity_verification_bench.cc`
- `recovery_rebuild_bench.cc`
- `distributed_retrieval_bench.cc`
- `bench_tensor_partial_refit.cc`
- `bench_tensor_summary_first.cc`
- `bench_tensor_cpu_gpu_breakeven.cc`
- `infrastructure_bench.cc`

## Benchmark-package status

- Phase B benchmark source is present for partial-refit/rebuild decision baselines.
- Phase C benchmark source is present for summary-first routing and exact-on-demand fetch.
- Phase D CPU/GPU break-even work is **CPU-baseline-ready** via `bench_tensor_cpu_gpu_breakeven.cc`.
- GPU pass/fail evidence is still pending real accelerator-backed result bundles.
- Phase 5 gate outcomes remain pending until the aggregated result bundle is compared against `release_gate_manifest_epic3.json`.

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

This directory now contributes Google Benchmark targets when
`THEMIS_BUILD_BENCHMARKS=ON` and benchmark dependencies are available.

Key target-to-profile mapping:

- `bench_epic3_distributed_tensor_placement_strategy_bench` → `placement_throughput_balanced`
- `bench_epic3_distributed_tensor_integrity_verification_bench` → `integrity_under_manifest_churn`
- `bench_epic3_distributed_tensor_recovery_rebuild_bench` → `recovery_rebuild_degraded`
- `bench_epic3_distributed_tensor_distributed_retrieval_bench` → `planner_latency_healthy`
- `bench_epic3_distributed_tensor_infrastructure_bench` → `infrastructure_control_plane_stability`
- `bench_epic3_distributed_tensor_bench_tensor_summary_first` → Phase C summary-first gate
- `bench_epic3_distributed_tensor_bench_tensor_cpu_gpu_breakeven` → Phase D CPU/GPU baseline

## Usage

Use this README together with the local `CMakeLists.txt` and EPIC 3 docs to
extend measured baselines without introducing scaffold-only code paths.
