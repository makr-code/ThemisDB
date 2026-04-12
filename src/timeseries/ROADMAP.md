# Time Series Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-ready time series storage with Gorilla compression, continuous aggregation, retention policies, and an auto-batching buffer for single-point inserts.

## Completed ✅
- [x] TSStore – optimized time series storage engine
- [x] Gorilla compression for 10–20× space reduction
- [x] TSAutoBuffer – automatic batching for single-point inserts (NEW)
- [x] Continuous aggregation for downsampling
- [x] **Named Continuous Aggregate Materialization (v1.9.0):** `ContinuousAggDefinition` (name, config, auto_refresh, status, agg_id), `ContinuousAggMaterializationStatus`, and `ContinuousAggMaterializationEngine` — TimescaleDB-style `createAggregate` / `dropAggregate` / `listAggregates` / `getAggregate` registry; `refreshAggregate` / `refreshAll` (watermark-driven incremental refresh); `queryMaterialized`; `getAggregateStatus` / `getAllStatus` (`continuous_agg.cpp`)
- [x] Time-based retention policies
- [x] High-frequency data ingestion
- [x] Configurable compression strategies
- [x] RocksDB-backed persistence
- [x] Out-of-order write support with configurable late-arrival window (Target: Q2 2026) (Issue: #1976)
- [x] **Streaming cursor API** — `TsStreamCursor` in `ts_stream_cursor.h/cpp` (2026-04-12)
  - Lazy paginated iterator over `TSStore::query()`; default page_size=4 096
  - `open()`/`valid()`/`current()`/`advance()`/`close()`; `rowsConsumed()`/`pagesFetched()` stats
  - Zero-copy; caller owns result memory; iterator invalidation on concurrent chunk rotation detected
  - 8 focused tests (SC-01…SC-08) in `tests/test_ts_stream_cursor.cpp`
- [x] **Multi-metric batch write API** — `TSStore::putBatch(std::span<const TSRow>)` in `tsstore.h/cpp` (2026-04-12)
  - `TSRow` uses `string_view` for metric/entity — zero allocation at call site
  - `BatchWriteResult` with `ok_count`, `failed_count`, `row_errors` (per-row index + message)
  - Single `rocksdb::WriteBatch` commit for the entire span (atomic, O(1) WAL writes)
  - Gorilla-compression path: groups by metric:entity, sorts by timestamp, Gorilla-encodes per group
  - 14 focused tests (TB-01…TB-14) in `tests/test_tsstore_batch.cpp`

## In Progress 🚧
- [~] Adaptive compression selection per series (Gorilla vs. Delta-of-delta vs. RLE) (Target: Q2 2026)
- [~] Distributed time series partitioning across shards (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Columnar storage layout for analytical scan queries (Issue: #2007)
- [x] Downsampling policies (min/max/avg/sum per window) — DownsamplingPipeline + TierSelector
- [?] Time series anomaly detection (Z-score, IQR-based)
- [?] Gap-filling functions (forward fill, linear interpolation)
- [?] Multi-series JOIN queries with aligned timestamps

### Long-term (6-12 months)
- [?] Tiered storage (hot/warm/cold) with automatic migration
- [?] Streaming ingestion via Kafka connector
- [I] Prometheus remote-write endpoint compatibility (Issue: #2037)
- [?] InfluxDB line protocol ingestion compatibility
- [?] Time series forecasting (ARIMA, Holt-Winters) as built-in functions

## Implementation Phases

### Phase 1: Core Storage & Compression (Status: Completed ✅)
- [x] TSStore – optimized RocksDB-backed time series storage engine
- [x] Gorilla compression (XOR delta encoding) for 10–20× space reduction
- [x] TSAutoBuffer – automatic batching for single-point inserts with configurable flush interval
- [x] Continuous aggregation engine for real-time downsampling
- [x] Time-based retention policy enforcement with background compaction
- [x] Configurable compression strategy registry

### Phase 2: Incremental Aggregation & Downsampling (Status: In Progress 🚧)
- [?] Incremental continuous aggregation (avoid full recompute on append) (Target: Q2 2026)
- [x] Multi-tier downsampling policies (1s → 1m → 1h → 1d) — `DownsamplingPipeline` + `TierSelector` in `downsampling.{h,cpp}` + `query_optimizer.cpp`
- [x] Adaptive TSAutoBuffer flush based on write pressure — `FlushController` EWMA + backpressure signalling in `ts_auto_buffer.{h,cpp}` + `timeseries_metrics.{h,cpp}`
- [x] Out-of-order write support with configurable late-arrival window (Target: Q3 2026)

### Phase 3: SIMD, Encryption & Export (Status: Planned 📋)
- [x] SIMD Gorilla decoder (AVX2/NEON) for accelerated bulk decompression — `GorillaSIMDDecoder` in `gorilla_simd.{h,cpp}` (Issue #117) (Target: v1.6.0)
- [?] Chunk-level encryption at rest with per-series key derivation
- [?] Parquet export bridge for analytical pipeline integration
- [ ] Columnar storage layout for analytical scan queries
- [x] Prometheus remote-write endpoint compatibility (`timeseries/prometheus_remote_write.cpp`) (Issue: #2037)
  - Endpoint: `POST /api/v1/prom/write`; headers: `Content-Encoding: snappy`, `Content-Type: application/x-protobuf`
  - Implemented in `prometheus_remote_write.{h,cpp}` (hand-rolled protobuf decoder) + `timeseries_api_handler.cpp`
  - Label mapping: `__name__` → metric, `instance` → entity, all other labels → tags JSON
  - Returns HTTP 204 No Content on success per Prometheus remote-write 1.0 spec
  - Error cases: 400 on malformed protobuf/snappy payload; 400 on unsupported encoding; 501 when ts feature disabled
  - Tests: 12 unit/integration tests in `tests/test_prometheus_remote_write.cpp`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — 49+ new tests added (test_downsampling, test_ts_adaptive_flush, test_prometheus_remote_write, test_tsstore_out_of_order); focused standalone targets: `DownsamplingFocusedTests`, `TSAdaptiveFlushFocusedTests`, `PrometheusRemoteWriteFocusedTests`, `TSStoreOutOfOrderFocusedTests`
- [x] Integration tests (compression round-trip, retention enforcement, aggregation accuracy)
- [?] Performance benchmarks (ingestion rate, query latency, compression ratio)
- [?] Security audit (time series key namespace isolation per tenant)
- [x] Documentation complete — all new public APIs in downsampling.h, query_optimizer.h, ts_auto_buffer.h, timeseries_metrics.h documented
- [~] API stability guaranteed — TSStore, TSAutoBuffer, DownsamplingPipeline, TierSelector APIs marked stable; TSAutoBufferConfig extensible via optional fields

## Known Issues & Limitations
- Out-of-order writes are now handled via the configurable late-arrival window (`Config::late_arrival_window_ms`); data arriving within the window is accepted regardless of order.
- Distributed sharding for time series is not yet implemented.
- TSAutoBuffer flush interval can be changed at runtime via `setConfig()`, but the background flush thread uses the interval at the time of its last wait — a config change takes effect on the next timer expiry (up to one `flush_interval` delay).

## Breaking Changes
- TSStore public API is stable from v1.x.
- Gorilla compressed wire format is versioned: v1 chunks are prefixed with a 3-byte header (`kGorillaMagic0`, `kGorillaMagic1`, `kGorillaCurrentVersion`). Legacy chunks (no header) are still decoded transparently. v2 format (with dictionary encoding) planned for v2.0.
