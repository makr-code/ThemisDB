# PERFORMANCE_EXPECTATIONS - src/scheduler

## Scope

- Module: src/scheduler
- This file defines measurable scheduler module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_task_scheduler.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SCHP-1 | task lifecycle registration paths remain bounded | TaskSchedulerBenchFixture/RegisterUnregister, TaskSchedulerBenchFixture/ConcurrentRegister |
| SCHP-2 | execution and management query paths remain bounded | TaskSchedulerBenchFixture/ExecuteTaskNow, TaskSchedulerBenchFixture/ListTasks, TaskSchedulerBenchFixture/GetStats |
| SCHP-3 | scheduler behavior remains stable across registered-task scale args | TaskSchedulerBenchFixture/ListTasks (Args: 10, 50, 100) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SCHG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SCHG-2 | scheduler hot-path p99 <= release threshold | p99 from mapped scheduler benchmark cases |
| SCHG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional scheduler benchmark scenarios are introduced.

## Sourcecode Verification (Module: scheduler/performance)

- Verified benchmark sources:
  - benchmarks/bench_task_scheduler.cpp
- Verified mapping surfaces:
  - register/unregister, concurrent register, execute now, list tasks, stats retrieval
- Result:
  - Referenced benchmark cases exist in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.