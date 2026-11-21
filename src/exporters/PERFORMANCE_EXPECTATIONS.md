---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "benchmarks/exporters/CMakeLists.txt; benchmarks/exporters/bench_exporters.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# PERFORMANCE_EXPECTATIONS - src/exporters

Exact model backend not verifiable from final document text alone.

## Scope

- Module: src/exporters
- This file defines measurable exporters module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/exporters/bench_exporters.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| EXPP-1 | JSONL batch export path remains within release baseline budget | BM_JsonlExport_BatchThroughput |
| EXPP-2 | JSONL template and compressed paths remain bounded | BM_JsonlExport_FormatTemplate, BM_JsonlExport_Compressed |
| EXPP-3 | streaming export throughput path remains bounded | BM_StreamingExport_Throughput |
| EXPP-4 | incremental export full and delta paths remain bounded | BM_IncrementalExport_Full, BM_IncrementalExport_Delta |
| EXPP-5 | large-row Parquet and CSV export paths remain bounded | No matching format-specific benchmark |

The legacy `BM_Export_Parquet_1M` and `BM_Export_CSV_1M` cases both call
`JSONLLLMExporter` in 100 batches; neither exercises a Parquet or CSV exporter.
They must not be used as evidence for EXPP-5. A format-specific benchmark
and an appropriate baseline are still needed.

The incremental cases use explicit `_seq` fields. The delta case seeds a
watermark at sequence `n - 10` and fails the benchmark if a measured export
does not emit exactly the ten new rows. EXPP-4 remains incomplete if either
case fails or is absent. The compressed JSONL case measures ZSTD compression.

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| EXG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| EXG-2 | exporters hot-path p99 <= release threshold | p99 from mapped exporters benchmark cases |
| EXG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional exporters benchmark scenarios are introduced.

## Sourcecode Verification (Module: exporters/performance)

- Verified benchmark sources:
  - benchmarks/exporters/bench_exporters.cpp
- Verified mapping surfaces:
  - JSONL batch/template/compressed and streaming benchmark cases
  - incremental export full and delta cases
  - no format-specific large-row Parquet/CSV benchmark is currently mapped
- Result:
  - The mapped JSONL and streaming cases exist in current benchmark sources.
  - The incremental delta fixture seeds `_seq` values and checks the exported row count.
  - The legacy large-row cases are JSONL proxies, not evidence for Parquet/CSV.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.