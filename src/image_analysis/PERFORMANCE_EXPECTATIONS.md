# PERFORMANCE_EXPECTATIONS - image_analysis

<!-- Status: current | validated: 2026-10-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Module: `src/image_analysis`
- This file defines measurable performance expectations for the module's release gates and should be treated as the current source-backed target set, subject to fresh benchmark evidence.

## Target Latencies (P99)

| Gate ID | Expectation | Status |
|---|---|---|
| IAP-1 | OCR text extraction P99 ≤ 100 ms per image | Target |
| IAP-2 | Object detection P99 ≤ 200 ms per image | Target |
| IAP-3 | Feature extraction P99 ≤ 50 ms per image | Target |
| IAP-4 | Cache lookup ≤ 1 ms | Target |
| IAP-5 | End-to-end (cache miss) P99 ≤ 300 ms | Target |

## Throughput

| Gate ID | Expectation | Status |
|---|---|---|
| IAT-1 | Batch processing ≥ 5 images/sec (single-threaded, OCR) | Target |
| IAT-2 | Object detection ≥ 5 images/sec (single-threaded) | Target |
| IAT-3 | Cache hit rate target > 80% for typical workloads | Target |

## Resource Consumption

| Gate ID | Expectation | Status |
|---|---|---|
| IAR-1 | Memory per image < 50 MB during processing | Target |
| IAR-2 | Model cache overhead ≤ 200 MB (Tesseract + YOLOv8 models) | Target |
| IAR-3 | Cache entry overhead ≤ 100 KB per cached result | Target |

## Module Hard Gates

| Gate ID | Expectation | Status |
|---|---|---|
| IAG-1 | Regression ≤ 10% vs release baseline | Pending fresh benchmark evidence |
| IAG-2 | Mapped benchmark cases run in release profile | Pending fresh benchmark evidence |

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`

## Validation

- These expectations remain target gates until a fresh release-profile run confirms them for the active build profile.
- Any backend or contract change must re-baseline this file before claiming that the module performance remains within the target envelope.

## Sourcecode Verification (Module: image_analysis/performance)

- Verified benchmark sources:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`
- Verified gate sources:
  - `src/image_analysis/ARCHITECTURE.md`
  - `src/image_analysis/ROADMAP.md`
- Result:
  - The benchmark files exist, and the target thresholds are documented. Fresh release-profile evidence is still required to confirm them as pass gates.
