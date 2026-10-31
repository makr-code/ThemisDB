# Updates Module Benchmark Map

## Scope and evidence

This page maps the benchmark cases currently present in
[`benchmarks/updates/`](../../benchmarks/updates/) and the CMake registration
file. It is not evidence that a benchmark has been run, nor does the current
registration define release-gating thresholds. Earlier UPDP/UPG target values
and measured results could not be verified against current benchmark
registration and are not carried forward as active gates.

## Registered benchmark sources

| Source | Registered cases / reported counters |
|---|---|
| [`bench_updates_release_gates.cpp`](../../benchmarks/updates/bench_updates_release_gates.cpp) | `BM_VerificationResult_Alloc`, `BM_DeploymentSlot_Switch`, `BM_UpdatesError_Cast`, `BM_VerificationResult_Copy`, `BM_VerificationResult_BatchAlloc`, `BM_UpdatesError_SwitchDispatch` |
| [`bench_updates_coordinated_hardening.cpp`](../../benchmarks/updates/bench_updates_coordinated_hardening.cpp) | `CoordinatedUpdate_AllNodesSuccessPath`; reports `nodes_updated` |
| [`bench_updates_canary_resilience.cpp`](../../benchmarks/updates/bench_updates_canary_resilience.cpp) | `NetworkDelayResilience`, `NodeFailureAtStages`, `HealthCheckTimeout`, `AutomaticRollbackOnFailure`; reports latency/stage counters |
| [`bench_updates_schema_diversity.cpp`](../../benchmarks/updates/bench_updates_schema_diversity.cpp) | `AddColumn`, `AddIndex`; reports `migration_time_ms` and `operation_count` |
| [`bench_updates_long_run_stability.cpp`](../../benchmarks/updates/bench_updates_long_run_stability.cpp) | `LongRunStability_MixedUpdateWorkload`; reports operation outcomes and `latency_ms` |

`bench_updates_cluster_stress_q4.cpp` is present in the source tree, but it is
not registered by [`benchmarks/updates/CMakeLists.txt`](../../benchmarks/updates/CMakeLists.txt).
Its source contains edge-handler, state-transition, concurrency, and rollback
statistics cases. Do not describe those as runnable CMake benchmark targets
until registration is added and verified.

## Validation requirements

- Check `THEMIS_BUILD_BENCHMARKS` and the configured build tree before running
  targets; CMake registration is conditional.
- Record the exact target, compiler, build profile, host, workload, and raw
  output for each run. Counters emitted by a benchmark are measurements, not
  acceptance thresholds.
- **TODO:** Define reviewed baseline and threshold values, register any
  intended cluster-stress benchmark, and add an auditable release-run manifest
  before claiming performance gates or current pass/fail status.

See [`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt) for
focused test registration. Tests and benchmark targets are separate evidence.
