# PERFORMANCE_EXPECTATIONS - src/temporal

## Scope

- Module: src/temporal
- This file defines measurable temporal module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_temporal_queries.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TMPP-1 | bitemporal insert/update/delete mutation paths remain bounded | BM_BiTemporalTable_Insert, BM_BiTemporalTable_Update, BM_BiTemporalTable_Delete |
| TMPP-2 | bitemporal as-of and valid-time query paths remain bounded | BM_BiTemporalTable_QueryBiTemporal, BM_BiTemporalTable_QueryCurrentByValidTime |
| TMPP-3 | bitemporal history retrieval path remains bounded | BM_BiTemporalTable_GetHistory |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TMPG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TMPG-2 | temporal hot-path p99 <= release threshold | p99 from mapped temporal benchmark cases |
| TMPG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional temporal benchmark scenarios are introduced.

## Sourcecode Verification (Module: temporal/performance)

- Verified benchmark sources:
  - benchmarks/bench_temporal_queries.cpp
- Verified mapping surfaces:
  - bitemporal mutation, query, and history retrieval behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.