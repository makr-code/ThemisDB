> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Time Series Module

Time series data management and compression implementation for ThemisDB.

## Module Purpose

Provides time series data management and compression for ThemisDB, offering Gorilla compression, continuous aggregation, retention management, and automatic batching for high-frequency single-point inserts.

## Subsystem Scope

**In scope:** Time series storage (TSStore), Gorilla delta-delta compression, continuous aggregation for downsampling, time-based retention, TSAutoBuffer for auto-batching, anomaly detection, hypertable partitioning, streaming cursors, batch ingestion, Prometheus remote-write endpoint, and chunk-level encryption.

**Out of scope:** General temporal data (handled by temporal module), event streaming (handled by cdc module), raw metrics collection (handled by observability module).

## Relevant Interfaces

Public header entry points (see [`../../include/timeseries/README.md`](../../include/timeseries/README.md) for full API):

- `tsstore.h/cpp` — time series storage backend (primary write/query API)
- `timeseries.h/cpp` — simplified TimeSeriesStore API (MVP entry point)
- `gorilla.h/cpp` — Gorilla compression codec (delta-of-delta + XOR float)
- `gorilla_simd.h/cpp` — SIMD-accelerated Gorilla decoder (AVX2/NEON)
- `continuous_agg.h/cpp` — continuous aggregation engine (materialized views)
- `retention.h/cpp` — retention policy enforcement (per-metric TTL)
- `ts_auto_buffer.h/cpp` — automatic batching buffer for single-point inserts
- `ts_auto_buffer_adaptive.h/cpp` — adaptive flush controller
- `downsampling.h/cpp` — multi-tier downsampling pipeline
- `hypertable.h/cpp` — TimescaleDB-style time-chunk partitioning
- `anomaly_detection.h/cpp` — Z-score and IQR anomaly detection
- `gap_fill.h/cpp` — gap-fill interpolation for sparse series
- `ts_stream_cursor.h/cpp` — lazy streaming cursor for large result sets
- `prometheus_remote_write.h/cpp` — Prometheus remote-write ingestion endpoint
- `encrypted_chunk_store.h/cpp` — AES-256-GCM encrypted chunk storage
- `ts_encrypted_key_rotation.h/cpp` — background key rotation for encrypted chunks

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — TSStore, Gorilla compression, continuous aggregation, retention policies, auto-batching, anomaly detection, gap fill, streaming cursor, multi-tier downsampling, Prometheus remote-write, and chunk-level encryption are operational.

## Components

### Core Storage

| Source file | Role |
|-------------|------|
| `timeseries.cpp` | Public `TimeSeriesStore` API (simplified MVP entry point) |
| `tsstore.cpp` | `TSStore` — RocksDB-backed time-series store with Gorilla compression |
| `hypertable.cpp` | `Hypertable` — TimescaleDB-style time-chunk partitioning via RocksDB Column Families |

### Compression

| Source file | Role |
|-------------|------|
| `gorilla.cpp` | `GorillaEncoder` / `GorillaDecoder` — delta-of-delta + XOR float codec |
| `gorilla_simd.cpp` | `GorillaSIMDDecoder` — AVX2/NEON accelerated bulk decompression |
| `compression_selector.cpp` | `HeuristicCompressionSelector` — automatic per-series codec selection |

### Query & Aggregation

| Source file | Role |
|-------------|------|
| `query_optimizer.cpp` | Timeseries-specific query optimizer (chunk pruning, tier selection) |
| `aggregates.cpp` | Built-in aggregate functions (min, max, avg, sum, count, p50, p99) |
| `aggregate_scheduler.cpp` | Background aggregate refresh scheduler |
| `aggregate_scheduler_helper.cpp` | Manual backfill helper for gap recovery |
| `continuous_agg.cpp` | `ContinuousAggregateManager` / `ContinuousAggMaterializationEngine` |
| `downsampling.cpp` | `DownsamplingPipeline` / `TierSelector` — multi-tier downsampling |
| `gap_fill.cpp` | `ForwardFillGapFiller`, `LinearInterpolationGapFiller`, `BackwardFillGapFiller` |

### Retention & Lifecycle

| Source file | Role |
|-------------|------|
| `retention.cpp` | `RetentionManager` — per-metric TTL enforcement with audit logging |
| `ts_auto_buffer.cpp` | `TSAutoBuffer` — auto-batching buffer (size/time/memory flush thresholds) |
| `ts_auto_buffer_adaptive.cpp` | Adaptive flush controller (EWMA latency feedback) |
| `ts_stream_cursor.cpp` | `TsStreamCursor` — lazy paginated iterator over `TSStore::query()` results |

### Encryption

| Source file | Role |
|-------------|------|
| `encrypted_chunk_store.cpp` | `EncryptedChunkStore` — AES-256-GCM compress-then-encrypt wrapper |
| `ts_encrypted_key_rotation.cpp` | Background job for re-encrypting stale chunks |

### Monitoring & Ingestion

| Source file | Role |
|-------------|------|
| `timeseries_metrics.cpp` | Prometheus metrics (ingest rate, compression ratio, flush lag) |
| `anomaly_detection.cpp` | `ZScoreDetector`, `IQRDetector`, `AnomalyDetector` |
| `prometheus_remote_write.cpp` | `POST /api/v1/prom/write` endpoint (Prometheus remote-write 1.0) |

## Features

- Optimized RocksDB-backed time-series storage
- Gorilla delta-of-delta compression (10–20× ratio) with SIMD-accelerated decoder
- **Automatic batching for single-point inserts** (`TSAutoBuffer`) with adaptive flush
- Multi-tier downsampling (raw → 1 min → 1 hour → 1 day)
- Continuous aggregation with watermark-driven incremental refresh
- Time-based retention policies with staged deletion and compliance audit logging
- High-frequency data ingestion via `putBatch()` (single `WriteBatch`, ≥1 M rows/s target)
- Streaming cursor for large result sets without full materialization
- Z-score and IQR anomaly detection
- Gap-fill interpolation (forward-fill, linear, backward-fill)
- Chunk-level AES-256-GCM encryption with background key rotation
- Prometheus remote-write endpoint (`POST /api/v1/prom/write`)
- Out-of-order write support with configurable late-arrival window
- Zero-copy batch write via `std::span<const TSRow>` (`putBatch`)

## Usage Example

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/retention.h"

// 1. Configure and create TSStore
TSStore::Config config;
config.compression = TSStore::CompressionType::Gorilla;
config.chunk_size_hours = 24;
TSStore store(db, cf, config);

// 2. Single-point inserts via auto-buffer (enables Gorilla for IoT workloads)
TSAutoBufferConfig buf_cfg;
buf_cfg.max_points_per_buffer = 500;
buf_cfg.flush_interval = std::chrono::seconds(5);
TSAutoBuffer buffer(&store, buf_cfg);
store.setAutoBuffer(&buffer);
buffer.start();

TSStore::DataPoint pt{ "cpu_usage", "server-01", now_ms(), 75.5, {}, {} };
store.putDataPoint(pt); // routed through buffer → Gorilla-encoded on flush

// 3. Zero-copy batch write
std::vector<TSStore::TSRow> rows = {
    {"cpu_usage", "server-01", now_ms(),     0.72},
    {"cpu_usage", "server-01", now_ms() + 1, 0.74},
};
auto result = store.putBatch(rows);

// 4. Range query
TSStore::QueryOptions opts;
opts.metric = "cpu_usage";
opts.entity = "server-01";
opts.from_timestamp_ms = t_start;
opts.to_timestamp_ms   = t_end;
auto points = store.query(opts);

// 5. Retention (per-metric TTL)
RetentionPolicy policy;
policy.per_metric["cpu_usage"] = std::chrono::days(30);
RetentionManager rm(&store, policy);
rm.startAsync(std::chrono::hours(1));

buffer.stop();
```

## Documentation

| Document | Path |
|----------|------|
| Public API reference | [`../../include/timeseries/README.md`](../../include/timeseries/README.md) |
| Architecture guide | [`ARCHITECTURE.md`](ARCHITECTURE.md) |
| Roadmap | [`ROADMAP.md`](ROADMAP.md) |
| Future enhancements | [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) |
| Changelog | [`CHANGELOG.md`](CHANGELOG.md) |
| Security | [`SECURITY.md`](SECURITY.md) |
| Storage methods (de) | [`../../docs/de/timeseries/STORAGE_METHODS.md`](../../docs/de/timeseries/STORAGE_METHODS.md) |
| Auto-buffer guide (de) | [`../../docs/de/timeseries/AUTO_BUFFER.md`](../../docs/de/timeseries/AUTO_BUFFER.md) |
| Configuration guide | [`../../docs/timeseries/CONFIG_GUIDE.md`](../../docs/timeseries/CONFIG_GUIDE.md) |
| Module overview (de) | [`../../docs/de/timeseries/README.md`](../../docs/de/timeseries/README.md) |

## Troubleshooting

### High write latency or dropped points

- Check `TSAutoBuffer.getStats()`: if `backpressure_events` is growing, increase `max_memory_bytes` or enable `enable_adaptive_flush`.
- Verify `flush_interval` is not too long for your latency budget; default is 5 s.
- Under extreme load, `push()` returns `PushStatus::BUFFER_FULL`; callers must back off and retry.

### Gorilla decode errors

- `GorillaDecoder::hasError()` returns `true` for truncated or corrupt chunks. Check for storage corruption.
- Legacy chunks (written before v1 header) are decoded transparently; no migration needed.
- `GorillaSIMDDecoder` validates the 3-byte magic header (`kGorillaMagic0`/`kGorillaMagic1`/`kGorillaCurrentVersion`) before decoding; unsupported version returns 0 points with `hasError() = true`.

### Continuous aggregate is not refreshing

- Verify the aggregate is registered via `ContinuousAggMaterializationEngine::listAggregates()`.
- Check status with `getAggregateStatus()`: INACTIVE aggregates are skipped by `refreshAll()`.
- Inspect watermark via `ContinuousAggWatermarkStore::getWatermark(agg_id)`; 0 means no refresh has occurred yet.

### Out-of-order writes being rejected

- `ERR_TIMESERIES_LATE_ARRIVAL` is returned when `timestamp_ms` is older than `(high_watermark - late_arrival_window_ms)`.
- Increase `Config::late_arrival_window_ms` or set to 0 to accept all timestamps.
- Check `TSStore::getOutOfOrderStats()` for counters.

### Retention not cleaning up data

- `RetentionManager::apply()` is synchronous; confirm `startAsync()` was called for background cleanup.
- Verify per-metric policy is set: `policy.per_metric["my_metric"] = std::chrono::days(N)`.
- Check `RetentionStats` (`getStats()`) for `apply_count` and `total_deleted`.

### Prometheus remote-write failures

- Returns HTTP 400 for malformed protobuf/snappy payloads, 400 for unsupported encoding, 501 when the timeseries feature is disabled.
- Headers required: `Content-Encoding: snappy`, `Content-Type: application/x-protobuf`.
- Label mapping: `__name__` → metric, `instance` → entity, all others → tags JSON.

## Scientific References

1. Pelkonen, T., Franklin, S., Teller, J., Cavallaro, P., Huang, Q., Meza, J., & Veeraraghavan, K. (2015). **Gorilla: A Fast, Scalable, In-Memory Time Series Database**. *Proceedings of the VLDB Endowment*, 8(12), 1816–1827. https://doi.org/10.14778/2824032.2824078

2. Elias, P. (1975). **Universal Codeword Sets and Representations of the Integers**. *IEEE Transactions on Information Theory*, 21(2), 194–203. https://doi.org/10.1109/TIT.1975.1055349

3. Ding, R., Wang, Q., Dang, Y., Fu, Q., Zhang, H., & Zhang, D. (2015). **YADING: Fast Clustering of Large-Scale Time Series Data**. *Proceedings of the VLDB Endowment*, 8(5), 473–484. https://doi.org/10.14778/2735479.2735481

4. Keogh, E., & Ratanamahatana, C. A. (2005). **Exact Indexing of Dynamic Time Warping**. *Knowledge and Information Systems*, 7(3), 358–386. https://doi.org/10.1007/s10115-004-0154-9

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/timeseries/README.md`](../../include/timeseries/README.md) for the public API.
