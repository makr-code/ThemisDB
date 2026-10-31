# PERFORMANCE_EXPECTATIONS - image_analysis

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** OpenAI ChatGPT
**Human_Review:** false
**Human_Author:** ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

<!-- Status: source review: 2026-10-31; benchmarks not run -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Scope

- Module: `src/image_analysis`
- The source tree does not establish an accepted numeric production performance target for real OCR or object detection. Values previously listed below are retained as unverified proposals only; they are not release gates.

## Previously Listed Latencies (Unverified Proposals)

| ID | Former expectation | Status |
|---|---|---|
| IAP-1 | OCR text extraction P99 ≤ 100 ms per image | Unverified; not a gate |
| IAP-2 | Object detection P99 ≤ 200 ms per image | Unverified; not a gate |
| IAP-3 | Feature extraction P99 ≤ 50 ms per image | No corresponding feature implementation in these two plugins; not a gate |
| IAP-4 | Cache lookup ≤ 1 ms | No module cache path found; not applicable |
| IAP-5 | End-to-end (cache miss) P99 ≤ 300 ms | No measured end-to-end path or cache evidence; not a gate |

## Previously Listed Throughput (Unverified Proposals)

| ID | Former expectation | Status |
|---|---|---|
| IAT-1 | OCR batch processing ≥ 5 images/sec | Plugins report batch processing unsupported; not a gate |
| IAT-2 | Object detection ≥ 5 images/sec | Unverified; not a gate |
| IAT-3 | Cache hit rate > 80% | No module cache path found; not applicable |

## Previously Listed Resource Budgets (Unverified Proposals)

| ID | Former expectation | Status |
|---|---|---|
| IAR-1 | Memory per image < 50 MB | Unverified; not a gate |
| IAR-2 | Model cache overhead ≤ 200 MB | No model cache implementation found; not applicable |
| IAR-3 | Cache entry overhead ≤ 100 KB | No result cache implementation found; not applicable |

## Evidence Status

- Available benchmark source files:
  - `benchmarks/image_analysis/bench_image_analysis.cpp`
  - `benchmarks/image_analysis/bench_image_analysis_latency.cpp`
  - `benchmarks/image_analysis/benchmark_image_analysis.cpp`
- These sources instantiate synthetic/mock plugins and exercise embedding/captioning or simulated workloads; they do not measure the Tesseract or YOLOv8 plugin implementations.
- No baseline, regression percentage, cache performance, OCR/detection latency, or release-profile result is established by these files or by this documentation review.

## Requirements Before Setting Numeric Gates

- Measure real plugin paths with declared model/runtime versions, image fixtures, build options, and target hardware.
- Define latency, throughput, and memory thresholds from those measurements and identify the owning CI gate.
- Do not claim a target is met until a reproducible run records the configured optional dependencies and results.
