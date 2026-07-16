# PERFORMANCE_EXPECTATIONS - src/scraper

## Scope

- Module: src/scraper
- This file defines measurable scraper module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_text_extraction.cpp
  - benchmarks/bench_content_processor_paths.cpp
  - benchmarks/bench_ingestion_extraction.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SCRP-1 | text extraction throughput paths remain bounded across content formats | BM_PDFExtraction, BM_DOCXExtraction, BM_HTMLExtraction, BM_PlainTextExtraction, BM_ConcurrentExtraction |
| SCRP-2 | processor-path extraction behavior remains bounded across common processor routes | BM_OfficeProcessorPath, BM_OcrProcessorPath, BM_ArchiveProcessorPath |
| SCRP-3 | ingestion extraction and adapter-sensitive paths remain bounded | DeonticExtractionFixture/SingleSentence_Obligation, DeonticExtractionFixture/SingleSentence_AllCategories, DeonticExtractionFixture/BatchExtraction_Scaling, DeonticExtractionFixture/LongText_MultiParagraph, DeonticExtractionFixture/ExtractEntities_FullDocument, LlmAdapterFixture/BuildExtractorFn, LlmAdapterFixture/BuildExtractor_Factory, LlmAdapterFixture/ExtractorFn_Throughput, BM_DetectBinaryMimeType, BM_CheckpointStore |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SCRG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SCRG-2 | scraper hot-path p99 <= release threshold | p99 from mapped scraper benchmark cases |
| SCRG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as dedicated scraper benchmark scenarios are introduced.

## Sourcecode Verification (Module: scraper/performance)

- Verified benchmark sources:
  - benchmarks/bench_text_extraction.cpp
  - benchmarks/bench_content_processor_paths.cpp
  - benchmarks/bench_ingestion_extraction.cpp
- Verified mapping surfaces:
  - extraction format throughput, processor paths, ingestion/extractor adapter behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Current mapping uses verified adjacent benchmark suites until dedicated scraper benchmarks are expanded.