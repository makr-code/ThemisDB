**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [benchmark registration](../../benchmarks/content/CMakeLists.txt), [processor-path benchmark](../../benchmarks/content/bench_content_processor_paths.cpp), [text-extraction benchmark source](../../benchmarks/content/bench_text_extraction.cpp), [content benchmark notes](../../benchmarks/content/README.md)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

# PERFORMANCE_EXPECTATIONS - src/content

## Scope

- Module: src/content
- This file defines measurable content module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/content/bench_content_processor_paths.cpp` (registered as
    `bench_content_processor_paths`; its Office/OCR/archive paths use synthetic work,
    not the production format processors)
  - `benchmarks/content/bench_text_extraction.cpp` (contains mock extraction workloads;
    not registered in `benchmarks/content/CMakeLists.txt`)

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CNT-1 | Office-like and OCR-like synthetic loops remain within a reviewed baseline | `BM_OfficeProcessorPath`, `BM_OcrProcessorPath` in the registered synthetic benchmark |
| CNT-2 | Archive-entry synthetic loop remains within a reviewed baseline | `BM_ArchiveProcessorPath` in the registered synthetic benchmark |
| CNT-3 | PDF/DOCX extraction implementations remain within a reviewed baseline | `BM_PDFExtraction`, `BM_DOCXExtraction` exist only in the unregistered mock source; TODO: provide a registered production-processor benchmark before treating this as an active release gate |
| CNT-4 | HTML/plain-text extraction implementations remain within a reviewed baseline | `BM_HTMLExtraction`, `BM_PlainTextExtraction` exist only in the unregistered mock source; TODO: provide a registered production-processor benchmark before treating this as an active release gate |
| CNT-5 | concurrent extraction remains within a reviewed baseline | `BM_ConcurrentExtraction` exists only in the unregistered mock source; TODO: register a production-path benchmark before treating this as an active release gate |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CTG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CTG-2 | extraction path p99 <= release threshold | p99 from mapped content extraction benchmark cases |
| CTG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional processor-specific benchmarks are introduced.

## Current evidence and limitations

- The current CMake file registers `bench_content_processor_paths`,
  `bench_content_versioning`, and `bench_content_release_gates`; it does not register
  `bench_text_extraction.cpp`.
- The processor-path target performs deterministic synthetic work, not calls to
  `OfficeProcessor`, `OCRProcessor`, or `ArchiveProcessor`.
- The mapped targets and thresholds are expectations only. No run result, representative
  hardware baseline, or complete release manifest is established by these source files.
- The unresolved cases above remain TODOs; do not report them as passed release gates.