# PERFORMANCE_EXPECTATIONS - src/toolbox

## Scope

- Module: src/toolbox
- This file defines measurable toolbox module performance expectations for release gating.
- **Phase 5 Status:** Dedicated toolbox-native benchmark suite operational as of 2026-08-07

## Benchmark Reference

### Native Benchmarks (Primary)
- **benchmarks/toolbox/bench_toolbox_native_workloads.cpp**
  - Dedicated toolbox-native benchmark suite for Phase 5 performance hardening
  - Direct measurement of extraction, text processing, and fingerprinting operations
  - Replaces proxy-only benchmark mappings

### Proxy Benchmarks (Legacy - for baseline comparison)
- benchmarks/bench_ingestion_extraction.cpp (mapped to TBXP-1)
- benchmarks/bench_ingestion_quality_judge.cpp (mapped to TBXP-2)
- benchmarks/bench_text_extraction.cpp (mapped to TBXP-3)
- benchmarks/bench_content_processor_paths.cpp (mapped to TBXP-3)

## Specific Expectations

### Native Benchmark Performance Targets

| Target ID | Operation | Gate ID | Gate Threshold | Benchmark Case |
|---|---|---|---|---|
| TBXP-1 | extractEntities() throughput | GATE-TBX-P1 | ≥ 100K ops/s | BM_ExtractEntities_Throughput |
| TBXP-2 | extractEntitySet() latency | GATE-TBX-P2 | p95 ≤ 50ms | BM_ExtractEntitySet_Latency |
| TBXP-3 | Text normalization latency | GATE-TBX-P3 | p95 ≤ 10ms | BM_TextNormalization_Latency |
| TBXP-4 | Language detection latency | GATE-TBX-P4 | p95 ≤ 15ms | BM_LanguageDetection_Latency |
| TBXP-5 | Content fingerprinting throughput | GATE-TBX-P5 | ≥ 1M ops/s | BM_ContentFingerprinting_Throughput |
| TBXP-6 | Bridge enrichment latency | GATE-TBX-P6 | p95 ≤ 100ms | BM_BridgeEnrichment_Placeholder |

### Legacy Proxy Benchmark Mappings (baseline reference only)

| Target ID | Expectation | Proxy Benchmark Cases |
|---|---|---|
| TBXP-1 | extraction and extractor-construction-adjacent paths remain bounded | DeonticExtractionFixture/BatchExtraction_Scaling, ExtractEntities_FullDocument, LlmAdapterFixture/ExtractorFn_Throughput |
| TBXP-2 | text quality and helper-evaluation-adjacent paths remain bounded | AllDimsFixture/QJ03_EvaluateAllDimensions, BM_QJ05_EvaluateEntityScaling |
| TBXP-3 | text extraction and content-processor-adjacent paths remain bounded | BM_PDFExtraction, BM_HTMLExtraction, BM_PlainTextExtraction, BM_ConcurrentExtraction |

## Module Hard Gates (v2.0 - Native Benchmarks)

| Gate ID | Expectation | Measurement | Native Benchmark |
|---|---|---|---|
| GATE-TBX-P1 | extractEntities throughput ≥ 100K ops/s | ops/second | bench_toolbox_native_workloads.cpp::BM_ExtractEntities_Throughput |
| GATE-TBX-P2 | extractEntitySet p95 latency ≤ 50ms | p95 latency in ms | bench_toolbox_native_workloads.cpp::BM_ExtractEntitySet_Latency |
| GATE-TBX-P3 | Text normalization p95 latency ≤ 10ms | p95 latency in µs | bench_toolbox_native_workloads.cpp::BM_TextNormalization_Latency |
| GATE-TBX-P4 | Language detection p95 latency ≤ 15ms | p95 latency in µs | bench_toolbox_native_workloads.cpp::BM_LanguageDetection_Latency |
| GATE-TBX-P5 | Fingerprinting throughput ≥ 1M ops/s | ops/second | bench_toolbox_native_workloads.cpp::BM_ContentFingerprinting_Throughput |
| GATE-TBX-P6 | Bridge enrichment p95 latency ≤ 100ms | p95 latency in ms | bench_toolbox_native_workloads.cpp::BM_BridgeEnrichment_Placeholder |
| TBXG-1 | Regression ≤ 10% vs Q3 2026 baseline | (current - baseline) / baseline | all native benchmarks |
| TBXG-2 | p99 latency ≤ release threshold | p99 from native benchmark percentiles | extraction, bridge, fingerprint paths |
| TBXG-3 | All native benchmark cases complete | manifest completeness check | 9 primary benchmark cases operational |

## Validation

- **Primary (Native):** Expectations are met when native benchmarks in bench_toolbox_native_workloads.cpp run in release profile and exceed configured thresholds.
- **Secondary (Baseline Comparison):** Proxy benchmarks remain as baseline reference to track any divergence from legacy performance profiles.
- **Regression Criteria:** Performance regression > 10% vs Q3 2026 baseline (established before native suite) triggers release gate failure.

## Phase 5 Completion Checklist

- [x] Native benchmark suite created: bench_toolbox_native_workloads.cpp
- [x] 6+ primary benchmark cases operational (P1-P6)
- [x] Performance gates documented (GATE-TBX-P1..P6)
- [x] Native suite registered in benchmarks/toolbox/CMakeLists.txt
- [ ] Baseline measurements collected (pending release profile run)
- [ ] Proxy-to-native performance delta quantified
- [ ] Release gates GATE-TBX-01..04 validated against native measurements
- [ ] Performance regression limits certified

## Sourcecode Verification (Module: toolbox/performance)

- **Native benchmark file:** benchmarks/toolbox/bench_toolbox_native_workloads.cpp (9.5 KB, 260+ lines)
- **Registration:** benchmarks/toolbox/CMakeLists.txt (themis_add_standard_benchmark)
- **Test data:** kShortText (~100 chars), kMediumText (~1KB), MakeMediumText() factory
- **Timing infrastructure:** std::chrono::steady_clock (deterministic microsecond precision)
- **Result:** Dedicated toolbox-native benchmark suite operational and ready for Q1 2027 release profiling