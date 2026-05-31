# PERFORMANCE_EXPECTATIONS - src/ingestion

## Scope

- Module: src/ingestion
- This file defines measurable ingestion module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_ingestion_kv.cpp
  - benchmarks/bench_ingestion_quality_judge.cpp
  - benchmarks/bench_ingestion_extraction.cpp
  - benchmarks/bench_timeseries_ingestion.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| INGP-1 | core single and batch key-value ingestion throughput remains bounded | IngestionBenchFixture/SingleIngest, IngestionBenchFixture/BatchIngest |
| INGP-2 | quality-judge evaluation and observer/config paths remain bounded | BM_QJ01_EvaluateNullBackend, BM_QJ02_EvaluateSingleDimension, BM_QJ04_EvaluateSparseContext, BM_QJ05_EvaluateEntityScaling, BM_QJ06_EvaluateBulletListParsing, BM_QJ07_ObserverDispatch_Zero, BM_QJ08_ObserverDispatch_N, BM_QJ09_SetConfig, BM_QJ10_ConstructDestruct, BM_QJ11_FeedbackLoopJudgeOnly |
| INGP-3 | extraction and adapter helper paths remain bounded | DeonticExtractionFixture/BatchExtraction_Scaling, LlmAdapterFixture/ExtractorFn_Throughput, BM_DetectBinaryMimeType, BM_CheckpointStore |
| INGP-4 | timeseries ingestion pathways used by ingestion-adjacent data flows remain bounded | TimeseriesBenchmarkFixture/RawDataIngestion, TimeseriesBenchmarkFixture/BatchIngestion, TimeseriesBenchmarkFixture/MultipleMetrics, BM_GorillaCompression, BM_GorillaDecompression, TimeseriesBenchmarkFixture/TimeRangeQuery, TimeseriesBenchmarkFixture/Downsampling, TimeseriesBenchmarkFixture/OutOfOrderWrites, BM_DownsamplingThroughput |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| INGG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| INGG-2 | ingestion hot-path p99 <= release threshold | p99 from mapped ingestion benchmark cases |
| INGG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional ingestion benchmark scenarios are introduced.

## Sourcecode Verification (Module: ingestion/performance)

- Verified benchmark sources:
  - benchmarks/bench_ingestion_kv.cpp
  - benchmarks/bench_ingestion_quality_judge.cpp
  - benchmarks/bench_ingestion_extraction.cpp
  - benchmarks/bench_timeseries_ingestion.cpp
- Verified mapping surfaces:
  - core ingestion throughput and quality-judge paths
  - extraction/mime/checkpoint paths
  - ingestion-adjacent timeseries ingest/compression/query paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.