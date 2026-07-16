# PERFORMANCE_EXPECTATIONS - src/content

## Scope

- Module: src/content
- This file defines measurable content module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_content_processor_paths.cpp
  - benchmarks/bench_text_extraction.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CNT-1 | office and OCR processor-path execution remains within release baseline budget | BM_OfficeProcessorPath, BM_OcrProcessorPath |
| CNT-2 | archive processor-path execution remains bounded | BM_ArchiveProcessorPath |
| CNT-3 | PDF and DOCX extraction paths remain bounded under benchmark payload scales | BM_PDFExtraction, BM_DOCXExtraction |
| CNT-4 | HTML and plain-text extraction paths remain bounded under benchmark payload scales | BM_HTMLExtraction, BM_PlainTextExtraction |
| CNT-5 | concurrent extraction path remains bounded under benchmark thread settings | BM_ConcurrentExtraction |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CTG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CTG-2 | extraction path p99 <= release threshold | p99 from mapped content extraction benchmark cases |
| CTG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional processor-specific benchmarks are introduced.

## Sourcecode Verification (Module: content/performance)

- Verified benchmark sources:
  - benchmarks/bench_content_processor_paths.cpp
  - benchmarks/bench_text_extraction.cpp
- Verified mapping surfaces:
  - office/OCR/archive processor-path benchmark cases
  - PDF/DOCX/HTML/plain-text extraction benchmark cases
  - concurrent extraction benchmark path
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.