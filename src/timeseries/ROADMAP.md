# Time Series Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-ready time series storage with Gorilla compression, continuous aggregation, retention policies, and an auto-batching buffer for single-point inserts.

## Completed ✅
- [x] TSStore – optimized time series storage engine
- [x] Gorilla compression for 10–20× space reduction
- [x] TSAutoBuffer – automatic batching for single-point inserts (NEW)
- [x] Continuous aggregation for downsampling
- [x] Time-based retention policies
- [x] High-frequency data ingestion
- [x] Configurable compression strategies
- [x] RocksDB-backed persistence

## In Progress 🚧
- [?] Adaptive compression selection per series (Gorilla vs. Delta-of-delta vs. RLE) (Target: Q2 2026)
- [x] Out-of-order write support with configurable late-arrival window (Target: Q2 2026) (Issue: #1976)
- [?] Distributed time series partitioning across shards (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Columnar storage layout for analytical scan queries (Issue: #2007)
- [?] Downsampling policies (min/max/avg/sum per window)
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
- [?] Multi-tier downsampling policies (1s → 1m → 1h → 1d) (Target: Q2 2026)
- [?] Adaptive TSAutoBuffer flush based on write pressure (Target: Q2 2026)
- [x] Out-of-order write support with configurable late-arrival window (Target: Q3 2026)

### Phase 3: SIMD, Encryption & Export (Status: Planned 📋)
- [?] SIMD Gorilla decoder (AVX2) for accelerated bulk decompression
- [?] Chunk-level encryption at rest with per-series key derivation
- [?] Parquet export bridge for analytical pipeline integration
- [ ] Columnar storage layout for analytical scan queries
- [ ] Prometheus remote-write endpoint compatibility

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (compression round-trip, retention enforcement, aggregation accuracy)
- [?] Performance benchmarks (ingestion rate, query latency, compression ratio)
- [?] Security audit (time series key namespace isolation per tenant)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Out-of-order writes are now handled via the configurable late-arrival window (`Config::late_arrival_window_ms`); data arriving within the window is accepted regardless of order.
- Distributed sharding for time series is not yet implemented.
- TSAutoBuffer flush interval is fixed at initialization; runtime adjustment is planned.

## Breaking Changes
- TSStore public API is stable from v1.x.
- Gorilla compressed wire format is versioned; v2 format (with dictionary encoding) planned for v2.0.
