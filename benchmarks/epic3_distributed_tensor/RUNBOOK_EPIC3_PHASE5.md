# EPIC 3 Phase 5 Benchmark Runbook

## Goal

Execute the distributed tensor Phase 5 benchmark suite only after the EPIC 3
runtime implementation and benchmark source files exist.

## Required inputs

- benchmark executables matching the planned files in this directory
- `phase5_workload_profiles.json`
- `release_gate_manifest_epic3.json`
- a result bundle conforming to `epic3_phase5_results_v1`

### Current executable/profile mapping

- `bench_epic3_distributed_tensor_distributed_retrieval_bench` → `planner_latency_healthy`
- `bench_epic3_distributed_tensor_placement_strategy_bench` → `placement_throughput_balanced`
- `bench_epic3_distributed_tensor_integrity_verification_bench` → `integrity_under_manifest_churn`
- `bench_epic3_distributed_tensor_recovery_rebuild_bench` → `recovery_rebuild_degraded`
- `bench_epic3_distributed_tensor_infrastructure_bench` → `infrastructure_control_plane_stability`
- `bench_epic3_distributed_tensor_bench_tensor_summary_first` → Phase C benchmark evidence
- `bench_epic3_distributed_tensor_bench_tensor_partial_refit` → Phase B benchmark evidence
- `bench_epic3_distributed_tensor_bench_tensor_cpu_gpu_breakeven` → CPU baseline bundle for later GPU comparison

## Execution sequence

1. Build with benchmarks enabled.
2. Run each EPIC 3 benchmark with canonical seed `42`.
3. Capture topology metadata, compiler/build metadata, and raw metric output.
4. Aggregate results into a single JSON bundle.
5. Compare the bundle against the gate manifest with `report_variance_epic3.py`.
6. Attach the summary output to Phase 6 acceptance documentation.

## CPU/GPU break-even note

`bench_tensor_cpu_gpu_breakeven.cc` currently establishes the CPU baseline only.
Do not mark GPU break-even validation complete until an accelerator-backed result
bundle shows the required speedup and records the hardware configuration used.

## Mandatory evidence per run

- hardware and topology description
- exact seed used for every profile
- raw benchmark output for all latency/throughput/recovery metrics
- pass/fail summary for `GATE-E3-P5-01` through `GATE-E3-P5-06`
- notes for any degraded-mode retries, integrity failures, or skipped profiles

## Promotion rule

Do not mark Phase 5 complete or start Phase 6 acceptance sign-off until every
gate has a recorded pass/fail result and any failure has a triage note.
