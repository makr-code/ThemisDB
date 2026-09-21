# PERFORMANCE_EXPECTATIONS - image_analysis

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Module: `src/image_analysis`
- This file defines measurable image analysis module performance expectations for release gating.

## Target Latencies (P99)

| Gate ID | Expectation | Source |
|---|---|---|
| IAP-1 | OCR text extraction P99 ≤ 100 ms per image | ARCHITECTURE.md §Performance Characteristics |
| IAP-2 | Object detection P99 ≤ 200 ms per image | ARCHITECTURE.md §Performance Characteristics |
| IAP-3 | Feature extraction P99 ≤ 50 ms per image | ARCHITECTURE.md §Performance Characteristics |
| IAP-4 | Cache lookup ≤ 1 ms | ARCHITECTURE.md §Performance Characteristics |
| IAP-5 | End-to-end (cache miss) P99 ≤ 300 ms | ARCHITECTURE.md §Performance Characteristics |

## Throughput

| Gate ID | Expectation | Notes |
|---|---|---|
| IAT-1 | Batch processing ≥ 5 images/sec (single-threaded, OCR) | ROADMAP.md §Phase 2 Performance Targets |
| IAT-2 | Object detection ≥ 5 images/sec (single-threaded) | ROADMAP.md §Phase 2 Performance Targets |
| IAT-3 | Cache hit rate target > 80% for typical workloads | ROADMAP.md §Phase 2 Performance Targets |

## Resource Consumption

| Gate ID | Expectation | Notes |
|---|---|---|
| IAR-1 | Memory per image < 50 MB during processing | ARCHITECTURE.md §Resource Consumption |
| IAR-2 | Model cache overhead ≤ 200 MB (Tesseract + YOLOv8 models) | ARCHITECTURE.md §Resource Consumption |
| IAR-3 | Cache entry overhead ≤ 100 KB per cached result | ARCHITECTURE.md §Resource Consumption |

## Module Hard Gates

| Gate ID | Expectation | Measurement |
|---|---|---|
| IAG-1 | Regression ≤ 10% vs release baseline | (current − baseline) / baseline |
| IAG-2 | All mapped benchmark cases run in release profile | benchmark run manifest completeness |

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain within configured thresholds.
- Performance expectations must be re-baselined in this file before merge when new backends (video, custom models) are introduced.

## Sourcecode Verification (Module: image_analysis/performance)

- Verified benchmark sources:
  - benchmarks/image_analysis/bench_image_analysis.cpp
  - benchmarks/image_analysis/bench_image_analysis_latency.cpp
  - benchmarks/image_analysis/benchmark_image_analysis.cpp
- Verified gate sources:
  - ARCHITECTURE.md §Performance Characteristics
  - ROADMAP.md §Phase 2 Performance Targets
- Result:
  - All referenced benchmark files confirmed present.
  - Latency and throughput targets are sourced from verified module architecture and roadmap documents.
