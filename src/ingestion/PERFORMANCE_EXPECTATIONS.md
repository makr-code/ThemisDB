# PERFORMANCE_EXPECTATIONS - src/ingestion

<!-- Status: current mapping; release enforcement requires CI/run evidence | validated: 2026-10-31 -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Scope

- Module: src/ingestion
- This file defines measurable ingestion module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/ingestion/bench_ingestion_kv.cpp
  - benchmarks/ingestion/bench_ingestion_quality_judge.cpp
  - benchmarks/ingestion/bench_ingestion_extraction.cpp
  - benchmarks/ingestion/bench_ingestion_release_gates.cpp
  - benchmarks/ingestion/bench_ingestion_dedicated_gates.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| INGP-1 | core single and batch key-value ingestion throughput remains bounded | IngestionBenchFixture/SingleIngest, IngestionBenchFixture/BatchIngest |
| INGP-2 | quality-judge evaluation and observer/config paths remain bounded | BM_QJ01_EvaluateNullBackend, BM_QJ02_EvaluateSingleDimension, BM_QJ04_EvaluateSparseContext, BM_QJ05_EvaluateEntityScaling, BM_QJ06_EvaluateBulletListParsing, BM_QJ07_ObserverDispatch_Zero, BM_QJ08_ObserverDispatch_N, BM_QJ09_SetConfig, BM_QJ10_ConstructDestruct, BM_QJ11_FeedbackLoopJudgeOnly |
| INGP-3 | extraction and adapter helper paths remain bounded | DeonticExtractionFixture/BatchExtraction_Scaling, LlmAdapterFixture/ExtractorFn_Throughput, BM_DetectBinaryMimeType, BM_CheckpointStore |
| INGP-4 | TODO: no timeseries benchmark source is present in `benchmarks/ingestion/`; do not treat timeseries results as this module's benchmark coverage. | None in this module |

## Source-declared thresholds (not a verified release result)

| Gate ID | Expectation | Measurement |
|---|---|---|
| INGG-1 | TODO: a <= 10 percent regression limit is not tied here to a checked-in baseline artifact or verified CI enforcement. | (current - baseline) / baseline |
| INGG-2 | Per-case thresholds are declared in benchmark source comments; validate the applicable threshold and measured backend for each run. | p99 from mapped benchmark cases |
| INGG-3 | TODO: verify benchmark manifest completeness enforcement in the current release workflow before calling this a blocking gate. | benchmark run manifest completeness |

## Validation

- A benchmark source and its labels are not evidence that a release run occurred or passed.
- `bench_ingestion_release_gates.cpp` uses in-memory/mock operations; it does not measure end-to-end connector, storage, or deployment capacity.
- The dedicated IN-BM cases are conditionally added by `benchmarks/ingestion/CMakeLists.txt` when the source file exists.
- Mapping should be expanded as additional ingestion benchmark scenarios are introduced.

## Sourcecode Verification (Module: ingestion/performance)

- Verified benchmark sources:
  - benchmarks/ingestion/bench_ingestion_kv.cpp
  - benchmarks/ingestion/bench_ingestion_quality_judge.cpp
  - benchmarks/ingestion/bench_ingestion_extraction.cpp
  - benchmarks/ingestion/bench_ingestion_release_gates.cpp
  - benchmarks/ingestion/bench_ingestion_dedicated_gates.cpp
- Verified mapping surfaces:
  - core ingestion throughput and quality-judge paths
  - extraction/mime/checkpoint paths
  - no timeseries benchmark in this module directory
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - source-declared thresholds require CI/run evidence before they can be described as enforced gates.