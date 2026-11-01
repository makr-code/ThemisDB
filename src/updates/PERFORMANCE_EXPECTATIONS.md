---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Updates Module Benchmark Map

## Scope and evidence

This page maps the benchmark cases currently present in
[`benchmarks/updates/`](../../benchmarks/updates/) and the CMake registration
files. Registration is not evidence that a benchmark has been run or that a
release gate has been approved. Earlier UPDP/UPG measured results could not be
reproduced and are not carried forward as active gates.

## Registered benchmark sources

| Source | Registered cases / reported counters |
|---|---|
| [`bench_updates_release_gates.cpp`](../../benchmarks/updates/bench_updates_release_gates.cpp) | `BM_VerificationResult_Alloc`, `BM_DeploymentSlot_Switch`, `BM_UpdatesError_Cast`, `BM_VerificationResult_Copy`, `BM_VerificationResult_BatchAlloc`, `BM_UpdatesError_SwitchDispatch` |
| [`bench_updates_coordinated_hardening.cpp`](../../benchmarks/updates/bench_updates_coordinated_hardening.cpp) | `CoordinatedUpdate_AllNodesSuccessPath`; reports `nodes_updated` |
| [`bench_updates_canary_resilience.cpp`](../../benchmarks/updates/bench_updates_canary_resilience.cpp) | `NetworkDelayResilience`, `NodeFailureAtStages`, `HealthCheckTimeout`, `AutomaticRollbackOnFailure`; reports latency/stage counters |
| [`bench_updates_schema_diversity.cpp`](../../benchmarks/updates/bench_updates_schema_diversity.cpp) | `AddColumn`, `AddIndex`; reports `migration_time_ms` and `operation_count` |
| [`bench_updates_long_run_stability.cpp`](../../benchmarks/updates/bench_updates_long_run_stability.cpp) | `LongRunStability_MixedUpdateWorkload`; reports operation outcomes and `latency_ms` |
| [`bench_updates_cluster_stress_q4.cpp`](../../benchmarks/updates/bench_updates_cluster_stress_q4.cpp) | `BM_EdgeCaseHandler_Detect_IDLE`, `BM_StateMachine_Transition_Rate`, `BM_EdgeCaseHandler_Concurrent_4Threads`, `BM_RollbackStats_Tracking`; registered separately in [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt) with a CTest smoke entry |

The cluster-stress target is not listed in the module-local CMake file; the
repository-level CMake file registers it conditionally when benchmarks are
enabled. The source comments declare the following candidate expectations:

| Case | Source-commented expectation |
|---|---:|
| `BM_EdgeCaseHandler_Detect_IDLE` | ≥ 500k operations/s |
| `BM_StateMachine_Transition_Rate` | ≥ 1M operations/s |
| `BM_EdgeCaseHandler_Concurrent_4Threads` | ≥ 100k operations/s |
| `BM_RollbackStats_Tracking` | ≥ 1M operations/s |

These are not owner-reviewed release thresholds and no run has verified them.
In particular, the first three cases do not call `SetItemsProcessed`; their
Google Benchmark output does not directly report `items_per_second`.

The focused test `CSS_12_ThroughputGoalMet` in
[`test_updates_cluster_scheduling_stress_focused.cpp`](../../tests/updates/test_updates_cluster_scheduling_stress_focused.cpp)
contains an executable assertion of at least 2,000 state transitions/second.
That test-level threshold is distinct from a representative benchmark baseline
or a deployment performance guarantee; the test has not completed in this
review.

## Validation requirements

- Check `THEMIS_BUILD_BENCHMARKS` and the configured build tree before running
  targets; CMake registration is conditional.
- Record the exact target, compiler, build profile, host, workload, and raw
  output for each run. Counters emitted by a benchmark are measurements, not
  acceptance thresholds.
- Diagnostic configuration with tests and benchmarks enabled completed on
  2026-11-01. The focused target build did not complete in this environment; no
  benchmark timing or pass/fail result is available.
- **TODO:** Obtain a complete focused build and benchmark run, review or replace
  the candidate thresholds, and attach host/build/raw-output evidence before
  claiming a performance gate or current pass/fail status.

See [`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt) for
focused test registration. Tests and benchmark targets are separate evidence.
