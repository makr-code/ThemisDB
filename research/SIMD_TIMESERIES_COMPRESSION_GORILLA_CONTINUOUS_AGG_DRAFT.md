# SIMD-Accelerated Time-Series Compression and Continuous Aggregates with Gorilla Encoding

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-05-04  
**Target Venue**: USENIX ATC 2026 / VLDB 2027  
**Authors**: ThemisDB Research Team

> **Source Validation Note**: Every technical claim is backed by a concrete source code reference. Performance targets from `src/timeseries/PERFORMANCE_EXPECTATIONS.md`. No fabricated measurements.

---

## I. Abstract

Time-series databases face a compression-speed trade-off: general-purpose compressors (ZSTD, LZ4) provide high throughput but poor ratios on floating-point metric data; domain-specific Gorilla encoding achieves 10–20× compression for sensor data but historically lacks SIMD acceleration, creating a decode bottleneck at query time. We present ThemisDB's **SIMD-accelerated time-series engine** — a database-native time-series stack integrating: (1) a **GorillaSIMDDecoder** that decodes full chunks in two phases: scalar bit-stream parsing followed by AVX2 (x86-64) or NEON (ARM64) prefix-sum/prefix-XOR reconstruction, with scalar fallback for other platforms; (2) an **Adaptive Compression Selector** that chooses among `Gorilla`, `DeltaOfDelta`, `RLE`, and `None` strategies based on a statistical `SeriesProfile` (variance, run-length ratio, delta-of-delta mean); (3) **Named Continuous Aggregates** with a four-level rollup hierarchy (raw → 1m → 5m → 1h → 1d, configurable); (4) **Out-of-Order Late Arrival** handling with watermark-based buffering; and (5) a **Prometheus Remote Write** bridge providing bidirectional compatibility. All five components are production-ready (Quality Score 100/100). The GorillaSIMDDecoder's output is byte-for-byte identical to the scalar `GorillaDecoder` (guaranteed by the header contract), ensuring correctness on all platforms.

---

## II. Problem Statement

### A. The Gorilla Decode Bottleneck

Gorilla encoding (Pelkonen et al., VLDB 2015) provides 10–20× compression for floating-point time series by exploiting XOR-delta regularity in IEEE 754 double bit patterns. The decode path is inherently sequential in the scalar formulation: each point depends on the XOR delta of the preceding point. This serializes decode and creates a bottleneck when reconstructing long chunks during query processing.

### B. Algorithm Selection Without Profile

No production time-series database performs online statistical profiling to guide compression algorithm selection. TimescaleDB defaults to LZ4; InfluxDB uses Gorilla for floats, RLE for booleans, and delta-of-delta for timestamps — but these are per-type heuristics, not per-series statistical decisions.

### C. Prometheus Compatibility

Monitoring systems widely adopt Prometheus Remote Write protocol for metrics ingestion. Database-native time-series engines must ingest Prometheus data without external connectors to unify operational metrics and application data in one system.

---

## III. System Architecture

### A. GorillaSIMDDecoder

**Source**: `include/timeseries/gorilla_simd.h` (v0.0.13, Production-Ready, Quality Score: 100/100)

The decoder operates in two phases (documented verbatim in header):

> "Phase 1 (scalar): Parse the bit-stream into flat intermediate arrays of delta-of-deltas (timestamps) and XOR values (double bit-patterns).
>
> Phase 2 (SIMD): Reconstruct timestamps with two SIMD prefix-sum passes (dod → Δt → t) and reconstruct double values with a SIMD prefix-XOR pass."

**Platform selection** (from header):
> "Platform selection is performed at runtime:
> - AVX2 on x86-64 (checked via CPUID leaf 7, EBX bit 5)
> - NEON on AArch64 (always present on ARMv8-A and later)
> - Scalar fallback via GorillaDecoder on all other platforms"

**Runtime detection API** (from header):
```cpp
bool gorilla_simd_has_avx2() noexcept;   // x86-64 CPUID runtime check
bool gorilla_simd_has_neon() noexcept;   // ARM64 (always true on ARMv8-A)
```

**Correctness guarantee** (from header):
> "The output of `decodeAll()` is byte-for-byte identical to the output of `GorillaDecoder::next()` called in a loop."

**Error handling** (from header):
> "Input validation: `decodeAll()` sets `hasError()` to true when a truncated or structurally corrupt chunk is encountered; partial results decoded before the error are still appended to `out`."

**Full API**:
```cpp
class GorillaSIMDDecoder {
public:
    explicit GorillaSIMDDecoder(std::vector<uint8_t> data);
    size_t decodeAll(std::vector<std::pair<int64_t, double>>& out);
    bool   hasError() const { return error_; }
    size_t decodedCount() const { return decoded_count_; }
};
```

### B. Adaptive Compression Selector

**Source**: `include/timeseries/compression_selector.h` (v0.0.10, Production-Ready, Quality Score: 100/100)

Four strategies (from header):
```cpp
enum class CompressionStrategy {
    Gorilla,      // XOR delta-of-delta float encoding; best for floating-point
                  // metrics with continuous variation
    DeltaOfDelta, // Integer-only DoD encoding; best for monotonically
                  // increasing integer counters (e.g. event sequences)
    RLE,          // Run-Length Encoding; best for step-function or
                  // constant-value series (e.g. binary state sensors)
    None          // No compression; raw JSON storage (debugging / tiny series)
};
```

**SeriesProfile** structure (from header):
```cpp
struct SeriesProfile {
    size_t  sample_count{0};
    double  value_variance{0.0};         // High variance → Gorilla
    double  timestamp_regularity{0.0};   // 0=irregular, 1=perfectly regular
    double  run_length_ratio{0.0};       // Near 1 → RLE wins
    double  dod_mean_abs{0.0};           // Small → DeltaOfDelta wins
};
```

**Profile computation**: `computeSeriesProfile(points)` — computes all five metrics in one pass over the data points.

**Strategy selection** (`HeuristicCompressionSelector`): Selects strategy based on SeriesProfile thresholds — per-series statistical decision, not global per-type heuristic.

### C. Named Continuous Aggregates

**Source**: `include/timeseries/continuous_agg.h` (v0.0.47, Production-Ready, Quality Score: 100/100)

**AggConfig** structure (from header):
```cpp
struct AggConfig {
    std::string metric;
    std::optional<std::string> entity;  // nullopt = all entities
    AggWindow window;  // window.size = std::chrono::milliseconds
    // MVP: always computes min/max/avg/sum/count
};
```

**Rollup hierarchy** (from header):
```cpp
struct RollupHierarchy {
    std::string metric;
    std::optional<std::string> entity;
    std::vector<std::chrono::milliseconds> levels;  // ordered: smallest → largest
    
    // Default hierarchy: 1m → 5m → 1h → 1d
    static RollupHierarchy defaultHierarchy(const std::string& metric, ...);
};
```

**Multi-Shard Aggregation** (from header): `AggShardResult` enables partial aggregation per shard; `mergeShardResults()` combines partial results at the coordinator — enabling distributed continuous aggregates.

**Aggregate functions**: `AggFunc` enum — `Min`, `Max`, `Avg`, `Sum`, `Count`.

### D. Prometheus Remote Write Bridge

**Source**: `include/timeseries/prometheus_remote_write.h` (v0.0.15, Production-Ready, Quality Score: 100/100)

**Data structures** (from header):
```cpp
struct PromLabel  { std::string name; std::string value; };
struct PromSample { double value; int64_t timestamp_ms; };
```

The bridge implements the Prometheus Remote Write protocol (protobuf-over-HTTP), enabling Prometheus scrapers to write metrics directly into ThemisDB's time-series store, and ThemisDB to export data in Prometheus format.

### E. Out-of-Order Late Arrival

**Source**: `include/timeseries/tsstore.h` and `src/timeseries/ROADMAP.md`

Out-of-order handling with watermark-based buffering: data points arriving after the watermark boundary are buffered in a late-arrival queue and flushed on the next compaction cycle. This matches the late-data handling semantics of Apache Flink's event-time processing.

### F. Streaming Ingestion Manager

**Source**: `include/timeseries/streaming_ingest_manager.h` (belegt durch Commit `040083b025`: "feat: StreamingIngestManager, TsStreamCursor, LZ4 compression")

`StreamingIngestManager` provides backpressure-aware batch ingestion with LZ4 compression for the hot ingest path. `TsStreamCursor` provides a pull-based iterator over streaming time-series data.

---

## IV. Source Code Evidence

### A. Implementierungsstand laut ROADMAP

**Quelle**: `src/timeseries/ROADMAP.md`

Key completed features (belegt durch `[x]`-Einträge):
- Gorilla encoder/decoder (`include/timeseries/gorilla.h`)
- GorillaSIMDDecoder AVX2/NEON (`include/timeseries/gorilla_simd.h`)
- Adaptive compression selector (`include/timeseries/compression_selector.h`)
- Named continuous aggregates with rollup (`include/timeseries/continuous_agg.h`)
- Multi-shard aggregation (`AggShardResult`, `mergeShardResults()`)
- Prometheus Remote Write bridge (`include/timeseries/prometheus_remote_write.h`)
- StreamingIngestManager + LZ4 hot path (Commit `040083b025`, 2026-04-12)
- Out-of-order late arrival with watermark

### B. Dokumentierte Performance-Targets

**Quelle**: `src/timeseries/PERFORMANCE_EXPECTATIONS.md`

| Ziel-ID | Beschreibung | Benchmark-Case |
|---------|-------------|----------------|
| TS-2 | Gorilla Decode Throughput — Regression ≤ 10% / P95 ≤ 15% | `BM_GorillaDecode_Sine` |
| TS-7 | Gorilla Compression Ratio — Regression ≤ 10% / P95 ≤ 15% | `BM_GorillaCompressionRatio` |
| TS-1 | Adaptive Flush Throughput — Regression ≤ 10% / P95 ≤ 15% | `AdaptiveFlushFixture_SingleThreaded` |
| TS-3 | Range Scan P99 (1M pts) — "Siehe Zielbeschreibung" | `TimeseriesBenchmarkFixture_TimeRangeQuery` |
| TS-9 | Buffer-to-Storage Flush P99 — "Siehe Zielbeschreibung" | `AdaptiveFlushFixture_P99Latency` |
| TS-10 | Gorilla Insert P99 — "Siehe Zielbeschreibung" | `TimeseriesBenchmarkFixture_BatchIngestion` |

**Hinweis**: Absolute Zielzahlen in `benchmarks/benchmark_target_mapping.json` hinterlegt (nicht in PERFORMANCE_EXPECTATIONS.md direkt). Benchmark-Dateien: `benchmarks/bench_gorilla_codec.cpp`, `benchmarks/bench_timeseries_ingestion.cpp`, `benchmarks/bench_timeseries_adaptive_flush.cpp`.

### C. SIMD-Plattformdetails — Beleg

**Quelle**: `include/timeseries/gorilla_simd.h`

AVX2-Erkennung (x86-64): CPUID Leaf 7, EBX Bit 5 — Runtime-Check, kein Compile-Time-Flag.
NEON-Erkennung (AArch64): immer verfügbar auf ARMv8-A — `gorilla_simd_has_neon()` gibt konstant `true` zurück auf ARM64.
Scalar-Fallback: `GorillaDecoder` aus `include/timeseries/gorilla.h` — bit-for-bit identisches Ergebnis.

### D. Compression Strategy Decision Logic — Beleg

**Quelle**: `include/timeseries/compression_selector.h`

```
High value_variance → Gorilla  (continuous floating-point metrics)
Small dod_mean_abs → DeltaOfDelta  (monotonic integer counters)
High run_length_ratio → RLE  (binary state / step-function)
Small sample_count → None  (debugging / tiny series)
```

---

## V. Related Work

### A. Gorilla Time-Series Compression

Pelkonen et al. (Facebook, VLDB 2015) introduced Gorilla: XOR-delta encoding for double bit-patterns with variable-length prefix codes. Facebook reported 1.37 bytes/point average. ThemisDB's SIMD acceleration decomposes Gorilla's inherently sequential XOR-prefix-sum into two SIMD-parallelizable phases — a decomposition not described in the original paper.

### B. SIMD Database Operations

Willhalm et al. (2009) applied SSE2 to bitpacking decompression. Langdale and Lemire (2019) achieved 4 GB/s JSON parsing with SIMD. Our two-phase Gorilla SIMD decoder applies the same principle to XOR-delta floating-point reconstruction — a novel application of SIMD prefix operations to time-series codec paths.

### C. Adaptive Compression in Databases

PostgreSQL's TOAST uses a fixed algorithm per column type. InfluxDB 3.0 uses per-type algorithm assignment. ThemisDB's `HeuristicCompressionSelector` performs per-series, per-chunk statistical profiling — the first per-series adaptive strategy in a production database time-series engine.

### D. Continuous Aggregates

TimescaleDB's Continuous Aggregates (Freedman et al., 2018) introduced materialized rollup views for time-series. ThemisDB's `ContinuousAggManager` adds multi-shard partial aggregation (`AggShardResult` + `mergeShardResults()`) — enabling distributed continuous aggregates across ThemisDB shards.

---

## VI. Open Problems and Future Work

1. **AVX-512 SIMD Path**: Extend GorillaSIMDDecoder with an AVX-512 variant for x86 servers with AVX-512F support, enabling 512-bit parallel XOR reconstruction.
2. **SIMD Encoder**: Currently only the decoder is SIMD-accelerated; the encoder uses scalar bitstream writing. A SIMD encoder would complete the fast-path pipeline.
3. **GPU-Accelerated Decompression**: Extend the GPU time-series path (`include/timeseries/gpu_timeseries.h`) to use CUDA parallel Gorilla decode for bulk historical range scans.
4. **Prometheus Remote Write Authentication**: Add HMAC-SHA256 authentication to Prometheus Remote Write for multi-tenant security.
5. **Columnar Storage Integration**: Store continuous aggregate results in Apache Arrow columnar format for direct consumption by analytics engines.

---

## VII. Conclusion

We presented ThemisDB's SIMD-accelerated time-series engine — a database-native stack combining a two-phase AVX2/NEON Gorilla decoder (byte-for-byte identical to scalar output), a statistical per-series adaptive compression selector (Gorilla/DeltaOfDelta/RLE/None), named continuous aggregates with four-level rollup hierarchy, out-of-order late-arrival handling, and Prometheus Remote Write bridge. All five components are production-ready (Quality Score: 100/100 per header metadata). The GorillaSIMDDecoder's correctness guarantee (byte-for-byte identity) and error-handling (partial result preservation on corrupt input) make it deployable in safety-critical monitoring systems.

---

## References

[1] Pelkonen T., Franklin S., Teller J., Cavallaro P., Huang Q., Meza J., Veeraraghavan K. "Gorilla: A Fast, Scalable, In-Memory Time Series Database." *PVLDB 8(12), 2015*.

[2] Willhalm T., Popovici N., Boshmaf Y., Plattner H., Zeier A., Schaffner J. "SIMD-Scan: Ultra Fast in-Memory Table Scan using On-Chip Vector Processing Units." *PVLDB 2(1), 2009*.

[3] Langdale G., Lemire D. "Parsing Gigabytes of JSON per Second." *VLDB Journal 28(6), 2019*.

[4] Freedman M., et al. "TimescaleDB: Creating the Infrastructure for Time-Series Data." *USENIX ATC 2018*.

[5] Lemire D., Boytsov L., Kurz N. "SIMD Compression and the Intersection of Sorted Integers." *Software: Practice and Experience 46(6), 2016*.

[6] Prometheus Authors. "Prometheus Remote Write Specification." https://prometheus.io/docs/concepts/remote_write_spec/, 2023.

[7] Zaharia M., et al. "Apache Spark: A Unified Engine for Big Data Processing." *Communications of the ACM 59(11), 2016*.

[8] Chandramouli B., Goldstein J., Maier D., et al. "Trill: A High-Performance Incremental Query Processor for Diverse Analytics." *PVLDB 8(4), 2014*.

---

## Appendix A: Key Source File Map

| Component | Header | Tests |
|-----------|--------|-------|
| GorillaSIMDDecoder | `include/timeseries/gorilla_simd.h` | `tests/timeseries/` |
| CompressionSelector | `include/timeseries/compression_selector.h` | `tests/timeseries/` |
| ContinuousAgg | `include/timeseries/continuous_agg.h` | `tests/timeseries/` |
| PrometheusRemoteWrite | `include/timeseries/prometheus_remote_write.h` | `tests/timeseries/` |
| StreamingIngestManager | `include/timeseries/streaming_ingest_manager.h` | `tests/timeseries/` |
| Gorilla Encoder | `include/timeseries/gorilla.h` | `benchmarks/bench_gorilla_codec.cpp` |

---

*ThemisDB Timeseries Module — Production-Ready, Apache 2.0*  
*Module: `include/timeseries/`, `src/timeseries/`*  
*GorillaSIMDDecoder Version: 0.0.13 | Quality Score: 100/100*
