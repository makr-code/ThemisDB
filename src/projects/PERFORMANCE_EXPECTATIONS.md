# PERFORMANCE_EXPECTATIONS - src/projects

## Scope

- Module: src/projects
- This file defines measurable projects module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_content_versioning.cpp
  - benchmarks/bench_query_lazy_eval.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| PRJP-1 | snapshot/version lifecycle paths remain bounded | BM_VersionCreation, BM_VersionRetrieval, BM_ConcurrentVersioning |
| PRJP-2 | diff and version-storage overhead paths remain bounded | BM_DiffComputation, BM_StorageOverhead |
| PRJP-3 | projection-sensitive project-like data path remains bounded (proxy) | BM_LazyEval_FilterProject |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| PRJG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| PRJG-2 | project hot-path p99 <= release threshold | p99 from mapped projects benchmark cases |
| PRJG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as dedicated project-collaboration/template benchmark scenarios are introduced.

## Sourcecode Verification (Module: projects/performance)

- Verified benchmark sources:
  - benchmarks/bench_content_versioning.cpp
  - benchmarks/bench_query_lazy_eval.cpp
- Verified mapping surfaces:
  - snapshot/version creation-retrieval, diff/storage overhead, projection-sensitive proxy path
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Current mapping uses one explicit proxy case for project-like projection behavior until module-native benchmark depth is expanded.