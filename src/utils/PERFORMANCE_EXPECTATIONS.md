# PERFORMANCE_EXPECTATIONS - src/utils

## Scope

- Module: src/utils
- This file defines measurable utils module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_pii_stream_scanner.cpp
  - benchmarks/bench_simd_distance.cpp
  - benchmarks/bench_thread_pool_saturation.cpp
  - benchmarks/bench_encryption.cpp
  - benchmarks/bench_compression.cpp
  - benchmarks/bench_security.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| UTLP-1 | privacy scan and scan-plus-pseudonymize paths remain bounded | BM_ScanOnly, BM_ScanAndPseudonymize |
| UTLP-2 | SIMD distance helper hot paths remain bounded across representative vector sizes | BM_SIMD_L2, BM_SIMD_InnerProduct, BM_SIMD_CosineDistance |
| UTLP-3 | thread-pool submission, saturation, and shutdown helper paths remain bounded | ThreadPoolSaturationFixture/SubmitThroughput_IO, ThreadPoolSaturationFixture/SubmitThroughput_CPU, ThreadPoolSaturationFixture/SaturatedQueue_DropRate, ThreadPoolSaturationFixture/PriorityOrdering_UnderLoad, ThreadPoolSaturationFixture/ConcurrentProducers, ThreadPoolSaturationFixture/StatisticsQuery, BM_ShutdownLatency |
| UTLP-4 | HKDF helper and adjacent encryption helper path remain bounded | BM_HKDF_Derive_FieldKey |
| UTLP-5 | compression helper hot paths remain bounded | CompressionFixture/SequentialWrite, CompressionFixture/RandomRead |
| UTLP-6 | audit append helper paths remain bounded | BM_AuditLog_TamperEvidentAppend, BM_AuditLog_BatchAppend_100 |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| UTLG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| UTLG-2 | mapped utility hot-path p99 <= release threshold | p99 from mapped utility benchmark cases |
| UTLG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded only when additional utility hotspots need release gating.

## Sourcecode Verification (Module: utils/performance)

- Verified benchmark sources:
  - benchmarks/bench_pii_stream_scanner.cpp
  - benchmarks/bench_simd_distance.cpp
  - benchmarks/bench_thread_pool_saturation.cpp
  - benchmarks/bench_encryption.cpp
  - benchmarks/bench_compression.cpp
  - benchmarks/bench_security.cpp
- Verified mapping surfaces:
  - privacy scan and pseudonymization behavior
  - SIMD numeric helper behavior
  - thread-pool, HKDF, compression, and audit helper behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.