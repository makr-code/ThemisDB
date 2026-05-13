> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Timeseries Module — Public API

<!-- Status: current | validated: 2026-05-13 -->
<!-- Primary: src/timeseries/ | Docs: docs/timeseries/ docs/de/timeseries/ -->

This directory contains public header files for the ThemisDB timeseries module.

## Purpose

Public interfaces and declarations for time-series storage, hypertable partitioning, continuous aggregates, downsampling, gap-filling, compression, anomaly detection, streaming cursors, and metrics ingestion.

## Quick Start

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"

// Create TSStore with Gorilla compression
TSStore::Config cfg;
cfg.compression = TSStore::CompressionType::Gorilla;
TSStore store(db, cf, cfg);

// Attach auto-buffer so single-point inserts are Gorilla-encoded
TSAutoBufferConfig buf_cfg;
TSAutoBuffer buffer(&store, buf_cfg);
store.setAutoBuffer(&buffer);
buffer.start();

// Write and query
store.putDataPoint({"cpu_usage", "srv-01", now_ms(), 0.72, {}, {}});
auto res = store.query({.metric = "cpu_usage", .from_timestamp_ms = t0, .to_timestamp_ms = t1});
// res.value() is std::vector<TSStore::DataPoint>

buffer.stop();
```

---

## Headers

### Core

#### `timeseries.h` — Simplified TimeSeriesStore API

Entry point for basic time-series use cases (no auto-buffer configuration required).

**Key class:** `themis::TimeSeriesStore`

```cpp
#include "timeseries/timeseries.h"

themis::TimeSeriesStore ts(db, cf);

// Write
ts.put("cpu_usage", "server-1", {now_ms(), 75.5, {}});

// Range query
auto pts = ts.query("cpu_usage", "server-1", {.from_ms = t0, .to_ms = t1, .limit = 1000});

// Aggregation
auto agg = ts.aggregate("cpu_usage", "server-1", {.from_ms = t0, .to_ms = t1});
// agg.avg, agg.min, agg.max, agg.sum, agg.count

// Latest value
auto latest = ts.getLatest("cpu_usage", "server-1");
if (latest) { /* use *latest */ }

// Retention: delete points older than 30 days
ts.deleteOldPoints("cpu_usage", "server-1", now_ms() - 30LL * 86400 * 1000);
```

**Key schema:** `ts:{metric}:{entity}:{timestamp_ms}`

---

#### `tsstore.h` — Full TSStore API (primary entry point)

The main time-series storage engine with Gorilla compression, batch write, tag filtering, and metadata.

**Key class:** `themis::TSStore`

**Configuration:**

```cpp
TSStore::Config config;
config.compression            = TSStore::CompressionType::Gorilla; // or None
config.chunk_size_hours       = 24;   // Gorilla chunk size in hours
config.late_arrival_window_ms = 5000; // 0 = accept all out-of-order; >0 = reject too-old
```

**Write APIs:**

```cpp
// Single-point insert (routes through TSAutoBuffer when attached)
Result<void> res = store.putDataPoint(point);

// Batch insert with Gorilla compression
std::vector<TSStore::DataPoint> batch = { /*...*/ };
store.putDataPoints(batch);

// Zero-copy batch write — single RocksDB WriteBatch, ≥ 1 M rows/s target
std::vector<TSStore::TSRow> rows = {
    {"cpu_usage", "srv-01", now_ms(),     0.72},
    {"cpu_usage", "srv-01", now_ms() + 1, 0.74},
};
Result<TSStore::BatchWriteResult> bwr = store.putBatch(rows);
// bwr.value().ok_count, bwr.value().failed_count, bwr.value().row_errors
```

**Query API:**

```cpp
TSStore::QueryOptions opts;
opts.metric           = "cpu_usage";
opts.entity           = "srv-01";              // optional
opts.from_timestamp_ms = t0;
opts.to_timestamp_ms   = t1;
opts.limit            = 5000;
opts.tag_filter       = {{"env", "prod"}};     // optional exact-match filter

Result<std::vector<TSStore::DataPoint>> pts = store.query(opts);
Result<TSStore::AggregationResult>      agg = store.aggregate(opts);
```

**Error handling:**

```cpp
auto res = store.putDataPoint(point);
if (!res.ok()) {
    // res.error() contains a detailed error string
    LOG_ERROR("write failed: {}", res.error());
}
```

**Out-of-order writes:**

```cpp
TSStore::OutOfOrderStats ooo = store.getOutOfOrderStats();
// ooo.out_of_order_accepted — within late_arrival_window_ms
// ooo.late_arrival_rejected — too old, rejected with ERR_TIMESERIES_LATE_ARRIVAL
```

**Encryption attachment:**

```cpp
auto enc = std::make_shared<EncryptedChunkStore>(hkdf, lek_mgr, audit_logger);
store.setEncryptedChunkStore(enc);
// All Gorilla chunks are now encrypted with AES-256-GCM (compress-then-encrypt)
```

---

#### `hypertable.h` — Time-Chunk Partitioning

TimescaleDB-style horizontal partitioning via RocksDB Column Families.

**Key class:** `themis::Hypertable`

```cpp
#include "timeseries/hypertable.h"

Hypertable::Config htcfg;
htcfg.table_name = "metrics";
htcfg.chunk_interval = std::chrono::hours(24); // 1 chunk per day

Hypertable ht(rocksdb_wrapper, htcfg);
auto* cf = ht.getOrCreateChunk(timestamp_ms); // returns CF handle
```

> **Note:** `listChunks()` currently returns an empty list (stub — see FUTURE_ENHANCEMENTS.md).

---

### Compression

#### `gorilla.h` — Gorilla Codec

Delta-of-delta timestamp encoding + XOR float value encoding.

**Key classes:** `themis::GorillaEncoder`, `themis::GorillaDecoder`, `themis::BitWriter`, `themis::BitReader`

```cpp
#include "timeseries/gorilla.h"

// Encode
GorillaEncoder enc;
enc.add(t0, v0);
enc.add(t1, v1);
std::vector<uint8_t> bytes = enc.finish(); // includes 3-byte header (v1+)

// Decode
GorillaDecoder dec(bytes);
while (auto pt = dec.next()) {
    auto [ts, val] = *pt;
}
if (dec.hasError()) {
    // truncated or corrupt chunk; dec.decodedCount() successful points
}
```

**Chunk header:** Chunks produced by `finish()` carry a 3-byte prefix:
`kGorillaMagic0` (0x47 'G'), `kGorillaMagic1` (0x4F 'O'), `kGorillaCurrentVersion` (0x01).
Legacy chunks (no header) are decoded transparently.

**Compression ratio:** 10–20× for smooth sensor/metric workloads, ~4× for noisy data.

---

#### `gorilla_simd.h` — SIMD-Accelerated Gorilla Decoder

AVX2 (x86-64) or NEON (ARM) accelerated bulk decompression.

**Key class:** `themis::GorillaSIMDDecoder`

```cpp
#include "timeseries/gorilla_simd.h"

GorillaSIMDDecoder dec(bytes);
auto pts = dec.decodeAll(); // returns std::vector<std::pair<int64_t,double>>
if (dec.hasError()) { /* corrupt chunk */ }
```

Runtime CPUID detection selects AVX2/NEON or falls back to scalar `GorillaDecoder`.
**Target:** >2 GB/s decoded data per core (vs. ~400 MB/s scalar).

---

#### `compression_selector.h` — Automatic Compression Selection

Selects Gorilla vs. Delta-of-delta vs. RLE per series based on heuristics.

**Key classes:** `themis::HeuristicCompressionSelector`, `themis::PerSeriesCompressionRegistry`

---

### Query & Aggregation

#### `query_optimizer.h` — Timeseries Query Optimizer

Chunk pruning and tier selection for efficient range queries.

---

#### `aggregates.h` — Aggregate Functions

Built-in aggregate functions: min, max, avg, sum, count, first, last, p50, p99.

---

#### `aggregate_scheduler.h` — Background Aggregate Scheduler

Drives periodic incremental refresh of continuous aggregates.

---

#### `continuous_agg.h` — Continuous Aggregates

TimescaleDB-style materialized views with watermark-driven incremental refresh.

**Key classes:** `themis::ContinuousAggMaterializationEngine`, `themis::ContinuousAggregateManager`, `themis::ContinuousAggWatermarkStore`, `themis::DistributedAggregateCoordinator`

```cpp
#include "timeseries/continuous_agg.h"

ContinuousAggMaterializationEngine engine(&store);

// 1. Register (like CREATE MATERIALIZED VIEW)
ContinuousAggDefinition def;
def.name   = "cpu_5min";
def.config = { "cpu_usage", "srv-01", AggWindow{std::chrono::minutes(5)} };
engine.createAggregate(def);

// 2. Incremental refresh (called by scheduler or on-demand)
engine.refreshAggregate("cpu_5min", now_ms());
// or refresh all active aggregates
engine.refreshAll(now_ms());

// 3. Query materialized results (no refresh triggered)
auto pts = engine.queryMaterialized("cpu_5min", from_ms, to_ms);

// 4. Inspect status
auto status = engine.getAggregateStatus("cpu_5min");
// status->watermark_ms, status->status (ACTIVE/STALE/INACTIVE), status->windows_written

// 5. Drop
engine.dropAggregate("cpu_5min");
```

**Thread safety:** Individual methods are NOT thread-safe; external synchronization required for concurrent callers.

**Derived metric name:** `<metric>__agg_<window_ms>` (see `ContinuousAggregateManager::derivedMetricName()`).

---

#### `downsampling.h` — Multi-Tier Downsampling Pipeline

Configurable pipeline: raw → 1 min → 1 hour → 1 day.

**Key classes:** `themis::DownsamplingPipeline`, `themis::TierSelector`, `themis::DownsamplingTier`

---

#### `gap_fill.h` — Gap-Fill Interpolation

Fills missing data points in sparse time series.

**Key classes:** `themis::ForwardFillGapFiller`, `themis::LinearInterpolationGapFiller`, `themis::BackwardFillGapFiller`, `themis::GapFiller`

---

#### `adaptive_flush_controller.h` — Adaptive Flush Control

Adaptive write-buffer flush control via EWMA latency feedback (standalone helper).

---

### Retention & Lifecycle

#### `retention.h` — Retention Policy Manager

Per-metric TTL enforcement with background cleanup and compliance audit logging.

**Key classes:** `themis::RetentionManager`, `themis::RetentionPolicy`, `themis::StagedDeletionPolicy`, `themis::RetentionStats`, `themis::RetentionAuditEntry`

```cpp
#include "timeseries/retention.h"

RetentionPolicy policy;
policy.per_metric["cpu_usage"]    = std::chrono::days(30);
policy.per_metric["disk_io"]      = std::chrono::days(90);

RetentionManager rm(&store, policy);
rm.startAsync(std::chrono::hours(1)); // background cleanup every hour

// Optional: staged deletion (mark → soft → hard)
StagedDeletionPolicy staged;
staged.mark_after        = std::chrono::days(25);
staged.soft_delete_after = std::chrono::days(28);
staged.hard_delete_after = std::chrono::days(30);
rm.setStagedDeletion(staged);

// Optional: compliance audit callback
rm.setAuditCallback([](const RetentionAuditEntry& e) {
    LOG_AUDIT("retention: metric={} action={} records={}", e.metric, e.action, e.records_affected);
});

RetentionStats stats = rm.getStats();
// stats.total_deleted, stats.apply_count, stats.async_cycle_count
```

**Atomicity:** Deletion is at chunk boundary; partial chunk deletion is not permitted.

---

#### `ts_auto_buffer.h` — Auto-Batching Write Buffer

Buffers single-point inserts and flushes as Gorilla-compressed batches.

**Key classes:** `themis::TSAutoBuffer`, `themis::TSAutoBufferConfig`, `themis::TSAutoBufferStats`

```cpp
#include "timeseries/ts_auto_buffer.h"

TSAutoBufferConfig cfg;
cfg.max_points_per_buffer = 1000;
cfg.flush_interval        = std::chrono::seconds(5);
cfg.max_memory_bytes      = 100 * 1024 * 1024; // 100 MB
cfg.enable_adaptive_flush = true;
cfg.backpressure_slo_ms   = 50.0; // block producers if write latency > 50 ms

TSAutoBuffer buffer(&store, cfg);
buffer.start();

// Non-blocking push (use with setAutoBuffer)
auto status = buffer.push(point);
if (status == TSAutoBuffer::PushStatus::BUFFER_FULL) { /* back off */ }

// Blocking add (for direct use)
buffer.add(point);

// Manual flush
size_t flushed = buffer.flush();
size_t flushed_for = buffer.flushFor("cpu_usage", "srv-01");

// Statistics
TSAutoBufferStats stats = buffer.getStats();
// stats.points_buffered, stats.points_flushed, stats.backpressure_events

// WAL-based crash recovery
buffer.persistToWAL("/var/lib/themisdb/ts_autobuf.wal");
// On restart:
buffer.restoreFromWAL("/var/lib/themisdb/ts_autobuf.wal");
buffer.flush(); // replay to TSStore
TSAutoBuffer::removeWAL("/var/lib/themisdb/ts_autobuf.wal");

buffer.stop(); // flushes all remaining points
```

**Flush triggers:** size threshold (`max_points_per_buffer`), time threshold (`flush_interval`), memory threshold (`max_memory_bytes`), global threshold (`max_total_points`).

**PushStatus values:**
- `OK` — point buffered
- `BUFFER_FULL` — total buffer bytes exceed `max_buffer_bytes`; caller should retry later
- `INVALID_INPUT` — empty metric or entity; permanent error, do not retry

---

#### `ts_auto_buffer_adaptive.h` — Adaptive Buffer Extension

Extended adaptive flush controller integrated with `TSAutoBuffer`.

---

#### `ts_stream_cursor.h` — Streaming Cursor

Lazy paginated iterator over `TSStore::query()` results — avoids full in-memory materialization.

**Key class:** `themis::timeseries::TsStreamCursor`

```cpp
#include "timeseries/ts_stream_cursor.h"

TSStore::QueryOptions opts;
opts.metric = "cpu_usage";
opts.from_timestamp_ms = t0;
opts.to_timestamp_ms   = t1;

// Open cursor with default page_size=4096
auto cursor = themis::timeseries::TsStreamCursor::open(store, opts);

while (cursor->valid()) {
    const TSStore::DataPoint& pt = cursor->current();
    process(pt);
    cursor->advance();
}
// cursor->rowsConsumed(), cursor->pagesFetched() for diagnostics
```

**Constraints:**
- Zero-copy: caller owns result memory for the duration of the current page only.
- Invalidated if the underlying TSStore is destroyed or concurrent chunk rotation is detected.
- `advance()` returns `CursorInvalidated` error on stale cursor.

---

### Encryption

#### `encrypted_chunk_store.h` — Encrypted Chunk Storage

AES-256-GCM encryption wrapper for Gorilla-compressed chunks.

**Key class:** `themis::EncryptedChunkStore`

Attach to `TSStore` via `setEncryptedChunkStore()`; encryption is compress-then-encrypt.
All key accesses are audited via the configured `AuditLogger`.

---

#### `ts_encrypted_key_rotation.h` — Key Rotation

Background job that re-encrypts stale chunks without blocking reads.

---

### Monitoring

#### `anomaly_detection.h` — Statistical Anomaly Detection

Z-score and IQR-based anomaly detection for time-series values.

**Key classes:** `themis::AnomalyDetector`, `themis::ZScoreDetector`, `themis::IQRDetector`, `themis::AnomalyPoint`, `themis::AnomalyConfig`

```cpp
#include "timeseries/anomaly_detection.h"

AnomalyConfig cfg;
cfg.method      = AnomalyMethod::ZScore;
cfg.zscore_threshold = 3.0;

AnomalyDetector detector(cfg);
auto anomalies = detector.detect(data_points); // returns std::vector<AnomalyPoint>
for (const auto& a : anomalies) {
    // a.timestamp_ms, a.value, a.score, a.method
}
```

---

#### `timeseries_metrics.h` — Module Metrics

Internal Prometheus metrics for ingest rate, compression ratio, flush lag, backpressure events.

**Key class:** `themis::TimeSeriesMetrics`

```cpp
#include "timeseries/timeseries_metrics.h"

auto metrics = std::make_shared<TimeSeriesMetrics>();
store.setMetrics(metrics);     // wire to TSStore
// metrics expose recordBackpressure(), recordOverdueFlush(), etc.
```

---

#### `prometheus_remote_write.h` — Prometheus Remote-Write Endpoint

Handles `POST /api/v1/prom/write` (Prometheus remote-write 1.0 protocol).

- **Encoding:** `Content-Encoding: snappy` + `Content-Type: application/x-protobuf`
- **Label mapping:** `__name__` → metric, `instance` → entity, all others → tags JSON
- **Success:** HTTP 204 No Content
- **Errors:** HTTP 400 (malformed payload / unsupported encoding), HTTP 501 (feature disabled)

---

## Configuration Quick Reference

| Config struct | Header | Key fields |
|---------------|--------|------------|
| `TSStore::Config` | `tsstore.h` | `compression`, `chunk_size_hours`, `late_arrival_window_ms` |
| `TSAutoBufferConfig` | `ts_auto_buffer.h` | `max_points_per_buffer`, `flush_interval`, `max_memory_bytes`, `enable_adaptive_flush`, `backpressure_slo_ms` |
| `RetentionPolicy` | `retention.h` | `per_metric` (map of metric → `std::chrono::seconds`) |
| `StagedDeletionPolicy` | `retention.h` | `mark_after`, `soft_delete_after`, `hard_delete_after` |
| `AnomalyConfig` | `anomaly_detection.h` | `method` (ZScore/IQR/Both), `zscore_threshold`, `iqr_multiplier` |
| `Hypertable::Config` | `hypertable.h` | `table_name`, `chunk_interval` |

## Error Codes

| Error | Cause |
|-------|-------|
| `ERR_TIMESERIES_LATE_ARRIVAL` | Timestamp older than `(high_watermark - late_arrival_window_ms)` |
| `ERR_API_RESOURCE_EXHAUSTED` | Backpressure: buffer stopped while producer was waiting |
| `PushStatus::BUFFER_FULL` | Total buffer bytes exceed `max_buffer_bytes` |
| `PushStatus::INVALID_INPUT` | Empty metric or entity field |

## Troubleshooting

| Symptom | Check |
|---------|-------|
| High write latency | `TSAutoBufferStats::backpressure_events`; increase `max_memory_bytes` or enable `enable_adaptive_flush` |
| Points not appearing in queries | `TSAutoBuffer::flush()` pending; buffer may not have flushed yet |
| Gorilla decode errors | `GorillaDecoder::hasError()`; check for storage corruption or unsupported version |
| Continuous agg stale | `getAggregateStatus()` → STALE; call `refreshAggregate()` or `refreshAll()` |
| Retention not running | Confirm `RetentionManager::startAsync()` was called |
| Out-of-order rejected | Increase `Config::late_arrival_window_ms` or set to 0 |
| Prometheus write 400 | Check `Content-Encoding: snappy` and `Content-Type: application/x-protobuf` headers |

## Related Documentation

| Document | Path |
|----------|------|
| Implementation guide | [`../../src/timeseries/README.md`](../../src/timeseries/README.md) |
| Architecture guide | [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md) |
| Roadmap | [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md) |
| Future enhancements | [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md) |
| Changelog | [`../../src/timeseries/CHANGELOG.md`](../../src/timeseries/CHANGELOG.md) |
| Security | [`../../src/timeseries/SECURITY.md`](../../src/timeseries/SECURITY.md) |
| Storage methods (de) | [`../../docs/de/timeseries/STORAGE_METHODS.md`](../../docs/de/timeseries/STORAGE_METHODS.md) |
| Auto-buffer guide (de) | [`../../docs/de/timeseries/AUTO_BUFFER.md`](../../docs/de/timeseries/AUTO_BUFFER.md) |
| Configuration guide | [`../../docs/timeseries/CONFIG_GUIDE.md`](../../docs/timeseries/CONFIG_GUIDE.md) |
| Module overview | [`../../docs/timeseries/README.md`](../../docs/timeseries/README.md) |

## Implementation

See [`../../src/timeseries/`](../../src/timeseries/) for the implementation code.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
