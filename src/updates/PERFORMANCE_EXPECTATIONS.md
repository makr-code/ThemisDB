# PERFORMANCE_EXPECTATIONS - src/updates

## Scope

- Module: src/updates
- This file defines measurable updates module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/updates/bench_update_pipeline.cpp
  - benchmarks/updates/bench_updates_coordinated_hardening.cpp
  - benchmarks/updates/bench_updates_canary_resilience.cpp
  - benchmarks/updates/bench_updates_schema_diversity.cpp
  - benchmarks/updates/bench_updates_long_run_stability.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| UPDP-1 | update state transition and rollback helper paths remain bounded | BM_UpdateStateMachine_Transition, BM_UpdateStateMachine_CurrentState, BM_UpdateStateMachine_RollbackPath |
| UPDP-2 | release manifest round-trip path remains bounded | BM_ReleaseManifest_JsonRoundTrip |
| UPDP-3 | delta patch generation and apply paths remain bounded | DeltaEngineBenchFixture/GeneratePatch, DeltaEngineBenchFixture/ApplyPatch |
| UPDP-4 | Coordinated updates throughput >= baseline | ops/sec from bench_updates_coordinated_hardening::AllNodesSuccessPath |
| UPDP-5 | Canary rollout MTTF (mean time to recovery) <= 5 seconds | mttf_ms from bench_updates_canary_resilience::AutomaticRollbackOnFailure |
| UPDP-6 | Schema migration scalability: 100K rows < 10 seconds | migration_time_ms from bench_updates_schema_diversity::DropColumnWithCascade |
| UPDP-7 | Long-run stability: no memory growth > 5% | memory_growth_pct from bench_updates_long_run_stability::LongRunStability_1MOperations |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| UPG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| UPG-2 | update hot-path p99 <= release threshold | p99 from mapped update benchmark cases |
| UPG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional update benchmark scenarios are introduced.

## Sourcecode Verification (Module: updates/performance)

- Verified benchmark sources:
  - benchmarks/bench_update_pipeline.cpp
- Verified mapping surfaces:
  - state machine, manifest round-trip, and delta patch behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.