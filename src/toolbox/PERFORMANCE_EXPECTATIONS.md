# PERFORMANCE_EXPECTATIONS - src/toolbox

## Scope

- Module: src/toolbox
- This file defines measurable toolbox module performance expectations for release gating.

## Benchmark Reference

- No dedicated toolbox-native benchmark file is present in current benchmark sources.
- Verified adjacent proxy benchmark files:
  - benchmarks/bench_ingestion_extraction.cpp
  - benchmarks/bench_ingestion_quality_judge.cpp
  - benchmarks/bench_text_extraction.cpp
  - benchmarks/bench_content_processor_paths.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TBXP-1 | extraction and extractor-construction-adjacent paths remain bounded | DeonticExtractionFixture/BatchExtraction_Scaling, DeonticExtractionFixture/LongText_MultiParagraph, DeonticExtractionFixture/ExtractEntities_FullDocument, LlmAdapterFixture/BuildExtractorFn, LlmAdapterFixture/BuildExtractor_Factory, LlmAdapterFixture/ExtractorFn_Throughput |
| TBXP-2 | text quality and helper-evaluation-adjacent paths remain bounded | BM_QJ01_EvaluateNullBackend, BM_QJ02_EvaluateSingleDimension, AllDimsFixture/QJ03_EvaluateAllDimensions, BM_QJ05_EvaluateEntityScaling, BM_QJ06_EvaluateBulletListParsing, BM_QJ11_FeedbackLoopJudgeOnly |
| TBXP-3 | text extraction and content-processor-adjacent paths remain bounded | BM_PDFExtraction, BM_DOCXExtraction, BM_HTMLExtraction, BM_PlainTextExtraction, BM_ConcurrentExtraction, BM_OfficeProcessorPath, BM_OcrProcessorPath, BM_ArchiveProcessorPath |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TBXG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TBXG-2 | toolbox-adjacent hot-path p99 <= release threshold | p99 from mapped proxy benchmark cases |
| TBXG-3 | No mapped proxy benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when the mapped proxy benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Dedicated toolbox benchmarks should replace these proxy mappings as soon as native suites are introduced.

## Sourcecode Verification (Module: toolbox/performance)

- Verified benchmark sources:
  - benchmarks/bench_ingestion_extraction.cpp
  - benchmarks/bench_ingestion_quality_judge.cpp
  - benchmarks/bench_text_extraction.cpp
  - benchmarks/bench_content_processor_paths.cpp
- Verified mapping surfaces:
  - extraction, quality-judge-adjacent, text extraction, and content-processor-adjacent behavior
- Result:
  - Referenced proxy benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible proxy runs until dedicated toolbox suites exist.