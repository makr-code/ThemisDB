# PERFORMANCE_EXPECTATIONS - src/tensor

## Scope

- Module: src/tensor
- This file defines measurable tensor module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_tensor_fingerprint_graph.cpp
  - benchmarks/bench_tensor_fingerprint.cpp
  - benchmarks/bench_tensor_deduplication_manager.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| TENP-1 | tensor fingerprint graph insert/query/neighbour operations remain bounded | BM_TFG_Insert_Throughput, BM_TFG_Insert_SingleNode, BM_TFG_FindSimilar, BM_TFG_Neighbours |
| TENP-2 | tensor fingerprint graph concurrent-read and metadata/export paths remain bounded | BM_TFG_ConcurrentReads, BM_TFG_NodeCount, BM_TFG_ExportPersistedGraph |
| TENP-3 | tensor fingerprint fixture insert/similarity/storage-ratio paths remain bounded | FingerprintInsertFixture/BM_FingerprintInsert, FindSimilarFixture/BM_FindSimilar_100K, StorageReductionFixture/BM_StorageReductionRatio |
| TENP-4 | tensor dedup snapshot/replay throughput paths remain bounded | BM_TDM_SnapshotRestoreRoundTrip, BM_TDM_JournalReplayThroughput |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| TENG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| TENG-2 | tensor hot-path p99 <= release threshold | p99 from mapped tensor benchmark cases |
| TENG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional tensor benchmark scenarios are introduced.

## Sourcecode Verification (Module: tensor/performance)

- Verified benchmark sources:
  - benchmarks/bench_tensor_fingerprint_graph.cpp
  - benchmarks/bench_tensor_fingerprint.cpp
  - benchmarks/bench_tensor_deduplication_manager.cpp
- Verified mapping surfaces:
  - fingerprint graph, fingerprint fixtures, and dedup snapshot/replay behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.