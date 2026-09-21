# PERFORMANCE_EXPECTATIONS - src/access_model

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Scope

- Module: src/access_model
- This file defines measurable access_model performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/access_model/bench_access_coordinator_gates.cpp`

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| ACM-1 | L1→L2 promotion p99 <= 50 µs | GATE-ACM-01 |
| ACM-2 | Cache eviction→storage feedback p99 <= 100 µs | GATE-ACM-02 |
| ACM-3 | Cold→warm promotion p99 <= 100 ms | GATE-ACM-03 |
| ACM-4 | Event processing throughput >= 10 K events/sec | GATE-ACM-04 |
| ACM-5 | Coordinator memory overhead <= 50 MB | GATE-ACM-05 |
| ACM-6 | Age-based policy decision p99 <= 10 µs | GATE-ACM-06 |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CG-2 | Promotion/demotion hot-path p99 <= release threshold | p99 from mapped GATE-ACM-01..06 cases |
| CG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |
| CG-4 | Event queue depth remains bounded under sustained load | gauge: event_queue_depth |
| CG-5 | Worker thread utilization < 100% at rated event throughput | gauge: worker_thread_utilization |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain
  inside configured thresholds.
- Gates GATE-ACM-01..06 are defined in `bench_access_coordinator_gates.cpp`; re-capture on
  representative hardware is required before Wave B GA promotion.
- Regression baseline: ±10% tolerance on all mapped benchmark cases.

## Sourcecode Verification (Module: access_model/performance)

- Verified benchmark sources:
  - `benchmarks/access_model/bench_access_coordinator_gates.cpp`
- Verified mapping surfaces:
  - L1→L2 promotion latency path in `access_coordinator.cpp`
  - cache eviction→storage feedback path (eviction listener → coordinator → storage)
  - cold→warm promotion path (storage hot detection → coordinator → cache)
  - event processing throughput (background worker pool and event queue)
  - memory overhead tracking via `access_metrics.cpp`
  - policy decision path in `age_based_policy.cpp`
- Result:
  - All six GATE-ACM benchmark cases are defined in the benchmark source.
  - Release gates remain tied to reproducible benchmark runs and hardware baselines.
