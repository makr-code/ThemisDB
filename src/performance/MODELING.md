---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "include/performance/workload_predictor.h"
  - "include/performance/workload_adaptive_optimizer.h"
  - "include/performance/phase3/per_query_cost_model.h"
  - "src/performance/workload_predictor.cpp"
  - "src/performance/workload_adaptive_optimizer.cpp"
  - "src/performance/phase3/per_query_cost_model.cpp"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Workload Models

## Purpose and model boundaries

This document describes the workload and cost models implemented in the
performance module. It is for maintainers integrating or evaluating adaptive
behavior. These are bounded runtime heuristics and feedback models, not a
general-purpose model-training or orchestration service.

## Workload predictor

`WorkloadPredictor` accepts `WorkloadSnapshot` records containing a timestamp,
queries per second, CPU and memory utilization, average and p99 latency, and
active connection count. It retains a configurable sliding history (default 60
observations) and uses:

1. exponential moving average smoothing (`ema_alpha`, default 0.2);
2. ordinary least-squares trend extrapolation for forecast values; and
3. a coefficient-of-variation confidence score, with lower variation indicating
   greater confidence.

`predict(horizon_us)` returns a `WorkloadForecast`. With fewer than two
observations the header specifies a zero-confidence forecast. The model
calculates on demand and is internally synchronized; it does not create its own
sampling or collection thread. `recommend_scaling()` compares forecast
utilization to configured thresholds and returns suggested thread-pool and
cache sizes. Recommendations are not applied automatically by this class.

## Adaptive strategy model

`WorkloadAdaptiveOptimizer` is distinct from the predictor. It consumes query
observations and a concurrent-query count, classifies a `WorkloadProfile`, and
returns an `OptimizationStrategy` with parallelism, thread-pool size, cache
size, join algorithm, and index-type choices. The classification categories
are OLTP, OLAP, mixed, graph, vector, time-series, and unknown. Auto-adaptation
is opt-in through `enable_auto_adapt()` and runs at the requested interval.
The callback reports transitions; it is not an authorization or deployment
approval mechanism.

## Query-cost feedback

`phase3::PerQueryCostModel` uses an RAII `QueryGuard` to measure a query's
cycles and elapsed time, then retains at most 4,096 records. Callers provide a
logical query type and estimated cost; `end()` can add rows processed and pages
read. `getCalibrationFactors()` derives candidate optimizer constants from the
recorded history, and `calibrate()` applies them to an `OptimizerCostModel`.
This feedback is only as representative as the queries recorded and should be
reviewed against a stable baseline before operational use.

## Assumptions and limitations

- Input observations must represent a consistent workload window. The
  predictor's timestamps and units are caller supplied.
- The predictor's confidence score expresses signal variation, not a measured
  probability that a forecast is correct.
- Scaling recommendations are bounded by configured minimum and maximum
  resource sizes; they do not resize external pools or caches themselves.
- The adaptive optimizer's categories and strategy choices describe its
  current implementation, not a guarantee of workload-specific optimality.
- Cost calibration records and aggregate statistics should not be interpreted
  as causal evidence without controlling for hardware, data, concurrency, and
  cache state.

## Validation

The source files
[`test_workload_predictor.cpp`](../../tests/performance/test_workload_predictor.cpp)
and
[`test_performance_contract_hardening_focused.cpp`](../../tests/performance/test_performance_contract_hardening_focused.cpp)
are root-level and included by the current module CMake glob.
[`phase3/test_per_query_cost_model.cpp`](../../tests/performance/phase3/test_per_query_cost_model.cpp)
is included by the recursive phase3 test glob in
`tests/performance/CMakeLists.txt`; registration alone does not establish that
it builds or passes. Include too-few-observation, noisy and steady input,
threshold boundaries, invalid configuration, and sustained workload-shift
cases when extending coverage. For performance claims, compare identical traces
and hardware using
the measurement guidance in [RESEARCH.md](RESEARCH.md) and the benchmark
inventory in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

Exact model backend not verifiable from final document text alone.
