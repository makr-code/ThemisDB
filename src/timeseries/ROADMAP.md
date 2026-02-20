# Time Series Module Roadmap

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
- [ ] Adaptive compression selection per series (Gorilla vs. Delta-of-delta vs. RLE) (Target: Q2 2026)
- [ ] Out-of-order write support with configurable late-arrival window (Target: Q2 2026)
- [ ] Distributed time series partitioning across shards (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Columnar storage layout for analytical scan queries
- [ ] Downsampling policies (min/max/avg/sum per window)
- [ ] Time series anomaly detection (Z-score, IQR-based)
- [ ] Gap-filling functions (forward fill, linear interpolation)
- [ ] Multi-series JOIN queries with aligned timestamps

### Long-term (6-12 months)
- [ ] Tiered storage (hot/warm/cold) with automatic migration
- [ ] Streaming ingestion via Kafka connector
- [ ] Prometheus remote-write endpoint compatibility
- [ ] InfluxDB line protocol ingestion compatibility
- [ ] Time series forecasting (ARIMA, Holt-Winters) as built-in functions

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (compression round-trip, retention enforcement, aggregation accuracy)
- [ ] Performance benchmarks (ingestion rate, query latency, compression ratio)
- [ ] Security audit (time series key namespace isolation per tenant)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Out-of-order writes are not currently handled; data must arrive in timestamp order.
- Distributed sharding for time series is not yet implemented.
- TSAutoBuffer flush interval is fixed at initialization; runtime adjustment is planned.

## Breaking Changes
- TSStore public API is stable from v1.x.
- Gorilla compressed wire format is versioned; v2 format (with dictionary encoding) planned for v2.0.
