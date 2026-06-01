> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/timeseries/ARCHITECTURE.md -->

# Timeseries Module — Public Header Architecture

**Module Path:** `include/timeseries/`
**Implementation:** `../../src/timeseries/`
**Canonical architecture doc:** [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md)

---

## 1. Overview

`include/timeseries/` defines the **public time-series ingestion, storage, query, and management API contract** for ThemisDB. The 22 headers cover hypertable management, Gorilla/SIMD compression, downsampling, gap-fill, continuous aggregation, anomaly detection, adaptive flushing, encrypted chunk storage, Prometheus remote-write, streaming cursors, and query optimisation.

For runtime composition — chunk lifecycle, compaction, WAL integration, and scheduler internals — see:
→ [`../../src/timeseries/ARCHITECTURE.md`](../../src/timeseries/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Hypertable and Storage

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hypertable.h` | `Hypertable` | Time-partitioned hypertable definition and chunk management |
| `timeseries.h` | `TimeseriesStore` | Top-level time-series store entry point |
| `tsstore.h` | `TSStore` | Low-level time-series chunk storage backend |
| `ts_auto_buffer.h` / `ts_auto_buffer_adaptive.h` | `TSAutoBuffer`, `TSAutoBufferAdaptive` | Write-buffer with fixed and adaptive sizing |

### 2.2 Compression

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gorilla.h` | `GorillaEncoder` / `GorillaDecoder` | Facebook Gorilla XOR floating-point compression |
| `gorilla_simd.h` | `GorillaSIMDEncoder` | SIMD-accelerated Gorilla encoding |
| `compression_selector.h` | `CompressionSelector` | Adaptive compression codec selection |

### 2.3 Query and Aggregation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `aggregates.h` | `TimeseriesAggregates` | Built-in time-bucket aggregation functions |
| `continuous_agg.h` | `ContinuousAggregate` | Background-refreshed continuous aggregation |
| `aggregate_scheduler.h` | `AggregateScheduler` | Continuous-aggregate refresh scheduling |
| `query_optimizer.h` | `TSQueryOptimizer` | Time-series query plan optimisation |
| `gap_fill.h` | `GapFill` | Missing-value interpolation and gap-fill |

### 2.4 Downsampling and Retention

| Header | Public Type | Purpose |
|--------|------------|---------|
| `downsampling.h` | `DownsamplingPolicy` | Resolution-reduction rules for aged data |
| `retention.h` | `RetentionPolicy` | Age-based chunk deletion policy |

### 2.5 Anomaly Detection and Metrics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `anomaly_detection.h` | `TSAnomalyDetector` | Real-time anomaly detection on ingested series |
| `timeseries_metrics.h` | `TimeseriesMetrics` | Prometheus-compatible metrics emission |

### 2.6 Streaming and Encryption

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ts_stream_cursor.h` | `TSStreamCursor` | Low-latency streaming cursor over live and historical data |
| `encrypted_chunk_store.h` | `EncryptedChunkStore` | AES-GCM encrypted chunk storage backend |
| `ts_encrypted_key_rotation.h` | `TSEncryptedKeyRotation` | Live key rotation for encrypted chunk stores |

### 2.7 External Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `prometheus_remote_write.h` | `PrometheusRemoteWrite` | Prometheus Remote Write ingest endpoint |
| `adaptive_flush_controller.h` | `AdaptiveFlushController` | Back-pressure-aware flush rate controller |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::timeseries` | All hypertable, storage, query, and compression types |

---

## 4. Public Contract Notes

- `hypertable.h` and `timeseries.h` form the primary ingestion and query entry points; callers must not assume internal chunk layout.
- Compression headers expose codec selection and encode/decode contracts; codec internals are opaque.
- Continuous-aggregate and scheduler headers define refresh-semantics contracts; lag and staleness bounds are configurable.
- Encrypted-chunk and key-rotation headers define stable contracts for at-rest encryption; key management integrates with `include/security/`.
- Prometheus remote-write header provides a stable ingestion contract compatible with the standard remote-write specification.
