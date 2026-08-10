# SIMD-Accelerated Time-Series Compression and Continuous Aggregation in ThemisDB

**Status**: Review Candidate  
**Version**: 1.0  
**Last Updated**: 2026-08-10  
**Scope**: ThemisDB time-series module (`include/timeseries`, `src/timeseries`, related tests/benchmarks)

---

## Abstract / Zusammenfassung

This review paper documents and verifies the current ThemisDB implementation of SIMD-related Gorilla decoding, adaptive compression selection, and continuous aggregation for time-series workloads. The implementation evidence is grounded in repository artifacts (headers, source files, tests, and benchmarks), not in untraceable claims. The core verified components are: (1) `GorillaSIMDDecoder` with runtime feature detection and scalar fallback, (2) heuristic per-series compression strategy selection using `SeriesProfile`, and (3) continuous aggregation with watermark-driven incremental refresh and rollup hierarchies. We summarize the architecture, verification evidence, and current limitations, and we explicitly separate implemented behavior from future work.

---

## Introduction / Einleitung

Time-series systems must balance compression ratio, decode cost, ingestion latency, and query freshness. ThemisDB addresses this in a module-oriented architecture where time-series capabilities are implemented as first-class database components (`TSStore`, Gorilla codec, aggregate managers, query helpers), rather than as external adapters.

This article has three goals:

1. **Factual verification** of technical claims against the current ThemisDB codebase.
2. **Terminology alignment** with repository naming (`TSStore`, `GorillaSIMDDecoder`, `ContinuousAggMaterializationEngine`, `PromWriteRequest`, etc.).
3. **Review readiness** with a complete publication structure: method, evaluation, limitations, and references.

### Terminology used in this document

- **AQL / Multi-Model**: ThemisDB system-level query concepts are out of scope for this paper unless directly relevant to time-series internals.
- **Continuous Aggregate**: In this text, this means the concrete API and runtime in `continuous_agg.h/.cpp`.
- **Consistency model (time-series scope)**: watermark and late-arrival behavior are described only as implemented in `TSStore` and continuous aggregation code paths.

---

## Methodik / Ansatz

### 1) SIMD Gorilla decoding (`GorillaSIMDDecoder`)

**Primary source**: `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/gorilla_simd.h`  
**Implementation source**: `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/gorilla_simd.cpp`

Verified behavior:

- Runtime capability checks are exposed via:
  - `gorilla_simd_has_avx2()`
  - `gorilla_simd_has_neon()`
- Decoder API:
  - `size_t decodeAll(std::vector<std::pair<int64_t, double>>& out)`
  - `bool hasError() const`
  - `size_t decodedCount() const`
- Decoder contract documents:
  - AVX2 runtime detection on x86-64, NEON availability on AArch64, scalar fallback otherwise.
  - Error signaling for truncated/corrupt chunks with partial-output semantics.

Implementation note: `gorilla_simd.cpp` currently describes a batch-oriented decode path with SIMD prefix reconstruction helpers and compatibility fallback to `GorillaDecoder` when required.

### 2) Adaptive compression selection (`compression_selector`)

**Primary source**: `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/compression_selector.h`

Verified behavior:

- Strategy enum: `Gorilla`, `DeltaOfDelta`, `RLE`, `None`.
- Profiling model: `SeriesProfile` fields include sample count, value variance, timestamp regularity, run-length ratio, and mean absolute delta-of-delta.
- Rule-based selector thresholds (default config):
  - `min_samples = 4`
  - `rle_run_ratio_threshold = 0.70`
  - `dod_mean_abs_threshold = 1.0`
  - `regularity_threshold = 0.90`
- Registry semantics (`PerSeriesCompressionRegistry`):
  - priority order: pinned > cached > freshly selected
  - documented as **not thread-safe**; external synchronization required

### 3) Continuous aggregation and rollups

**Primary sources**:
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/continuous_agg.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/continuous_agg.cpp`

Verified behavior:

- `ContinuousAggregateManager` supports refresh and incremental refresh.
- `ContinuousAggWatermarkStore` persists per-aggregate watermark metadata.
- `RollupHierarchy::defaultHierarchy()` defines default levels `1m -> 5m -> 1h -> 1d`.
- Multi-shard merge helper exists (`AggShardResult`, `mergeShardResults`), plus coordinator API.

### 4) Out-of-order writes and watermark semantics

**Primary source**: `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/tsstore.h`

Verified behavior:

- `TSStore::Config::late_arrival_window_ms` controls acceptance/rejection of late points.
- Per-series watermark tracking is implemented (`watermarks_`, `watermark_mutex_` and watermark checks).
- Out-of-order behavior is exposed via statistics (`out_of_order_accepted`, `late_arrival_rejected`).

### 5) Prometheus remote-write decoding

**Primary sources**:
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/prometheus_remote_write.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/prometheus_remote_write.cpp`

Verified behavior:

- `PromWriteRequest::decode()` and `decodeSnappy()` parse protobuf/snappy payloads.
- Parser includes explicit bounds checks and varint overflow guards.
- Data structures include `PromLabel`, `PromSample`, and `PromTimeSeries`.

---

## Evaluation / Experimente

This section reports **verification evidence and benchmark/test coverage**, not unrecorded absolute performance claims.

### A) Claim-to-source verification

| Claim | Verification artifact |
|---|---|
| SIMD decoder runtime feature checks and fallback | `include/timeseries/gorilla_simd.h`, `src/timeseries/gorilla_simd.cpp` |
| Byte-level compatibility expectation with scalar path (as documented contract + tests) | `include/timeseries/gorilla_simd.h`, `tests/test_gorilla_simd.cpp` |
| Heuristic adaptive selection thresholds and decision order | `include/timeseries/compression_selector.h`, `tests/test_ts_future_interfaces.cpp` |
| Continuous aggregate rollup and watermark refresh APIs | `include/timeseries/continuous_agg.h`, `src/timeseries/continuous_agg.cpp`, `tests/timeseries/test_continuous_agg_materialization.cpp` |
| Prometheus remote-write parse/decode behavior | `include/timeseries/prometheus_remote_write.h`, `src/timeseries/prometheus_remote_write.cpp`, `tests/test_prometheus_remote_write.cpp` |

### B) Benchmark coverage currently documented in-repo

- Timeseries performance expectation mapping:  
  `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/PERFORMANCE_EXPECTATIONS.md`
- Referenced benchmark suites:
  - `/home/runner/work/ThemisDB/ThemisDB/benchmarks/bench_gorilla_codec.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/benchmarks/timeseries/bench_timeseries_ingestion.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`

The expectation file defines release-style gates (`TSRP-*`, `TSRG-*`) as bounded-regression criteria and benchmark-manifest completeness checks.

### C) Internal validity statement

- This paper intentionally avoids claims like “first in production” or fixed speedup factors unless they are reproducibly measured and published in benchmark result artifacts.
- Current evidence is strongest for **interface existence**, **algorithmic decision logic**, **error handling contracts**, and **test coverage presence**.

---

## Limitations / Known Issues

1. **Thread safety limitation**: `PerSeriesCompressionRegistry` is explicitly not thread-safe and requires external synchronization.
2. **Evidence type limitation**: Repository benchmark sources and performance expectation gates are present, but this paper does not attach machine-specific benchmark result datasets.
3. **Scope limitation (Prometheus path)**: Verified scope is decode/ingest parsing behavior in current sources; full end-to-end interoperability matrices are not part of this document.
4. **Continuous aggregate semantics**: Watermark-driven behavior is implemented, but operational guarantees still depend on deployment-level scheduling and workload profile.
5. **Roadmap dependence**: Additional hardening and expanded benchmark depth remain tracked in `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/ROADMAP.md`.

---

## Conclusion / Fazit

ThemisDB currently contains a substantial and source-verifiable time-series implementation for Gorilla codec handling, adaptive compression decision logic, and continuous aggregate materialization with watermark semantics. The strongest validated claims are architectural and contract-level, backed by concrete file-level evidence and dedicated tests. For publication-grade quantitative claims (throughput, latency, compression-ratio superiority), benchmark result artifacts should be attached alongside this text in a future revision.

---

## References

1. Pelkonen, T. et al. *Gorilla: A Fast, Scalable, In-Memory Time Series Database*. PVLDB 8(12), 2015. DOI: https://doi.org/10.14778/2824032.2824078  
2. Willhalm, T. et al. *SIMD-Scan: Ultra Fast in-Memory Table Scan using On-Chip Vector Processing Units*. PVLDB 2(1), 2009. DOI: https://doi.org/10.14778/1687627.1687641  
3. Langdale, G., Lemire, D. *Parsing Gigabytes of JSON per Second*. VLDB Journal 28, 2019. DOI: https://doi.org/10.1007/s00778-019-00552-9  
4. Timescale. *About Continuous Aggregates*. URL: https://docs.timescale.com/use-timescale/latest/continuous-aggregates/about-continuous-aggregates/  
5. Prometheus Authors. *Remote Write specification*. URL: https://prometheus.io/docs/specs/prw/remote_write_spec/  
6. Apache Flink Documentation. *Event Time and Watermarks*. URL: https://nightlies.apache.org/flink/flink-docs-stable/docs/concepts/time/  

---

## Appendix A: Key ThemisDB Artifacts Referenced

- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/gorilla_simd.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/gorilla_simd.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/compression_selector.h`
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/continuous_agg.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/continuous_agg.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/tsstore.h`
- `/home/runner/work/ThemisDB/ThemisDB/include/timeseries/prometheus_remote_write.h`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/prometheus_remote_write.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/tests/test_gorilla_simd.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/tests/test_ts_future_interfaces.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/tests/timeseries/test_continuous_agg_materialization.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/tests/test_prometheus_remote_write.cpp`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/PERFORMANCE_EXPECTATIONS.md`
- `/home/runner/work/ThemisDB/ThemisDB/src/timeseries/ROADMAP.md`
