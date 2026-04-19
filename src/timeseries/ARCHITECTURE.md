# Timeseries Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/timeseries/`

---

## 1. Overview

The Timeseries module provides ThemisDB's high-frequency time-series data management:
efficient TSStore storage, Gorilla delta-delta compression (Facebook, 2015), continuous
aggregation and downsampling, time-based retention, and TSAutoBuffer for transparent
batching of single-point inserts. It is optimized for metric streams, IoT sensor data,
financial tick data, and any monotonically advancing timestamp workload.

---

## 2. Design Principles

- **Gorilla Compression** – delta-of-deltas encoding for timestamps and XOR-based
  encoding for floating-point values achieves 10-20× compression for smooth metrics.
- **Auto-Batching** – `ts_auto_buffer.cpp` transparently aggregates high-frequency
  single-point inserts into chunks before writing to storage, avoiding write amplification.
- **Continuous Aggregation** – pre-computed downsampled views (1m → 5m → 1h → 1d) are
  maintained by the aggregate scheduler so time-range queries at any resolution are fast.
- **Retention by Policy** – `retention.cpp` enforces configurable retention periods per
  series; expired data is deleted in background sweeps.
- **Hypertable Layout** – `hypertable.cpp` partitions series by time chunk (analogous to
  TimescaleDB hypertables) for efficient range scans and parallel pruning.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `tsstore.cpp` | Time-series storage backend: read, write, range scan |
| `gorilla.cpp` | Gorilla delta-delta + XOR compression codec |
| `continuous_agg.cpp` | Continuous aggregation: pre-compute rollups |
| `aggregate_scheduler.cpp` / `aggregate_scheduler_helper.cpp` | Background aggregate refresh |
| `aggregates.cpp` | Aggregate function implementations (min, max, avg, sum, p50/p99) |
| `retention.cpp` | Retention policy: expire old data in background |
| `ts_auto_buffer.cpp` | Auto-batching buffer for high-frequency point inserts |
| `timeseries.cpp` | Public API: insert, query, range scan |
| `hypertable.cpp` | Time-chunk-based horizontal partitioning |
| `query_optimizer.cpp` | Timeseries-specific query optimization (chunk pruning) |
| `timeseries_metrics.cpp` | Prometheus metrics: ingest rate, compression ratio, lag |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│         High-frequency metric writer (1000s/sec per series)     │
│   tsstore.insert("cpu_usage", {timestamp, value})                │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    TSAutoBuffer                                  │
│   buffer single points → flush to chunk when buffer full        │
│   or max_buffer_duration exceeded                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ chunk
┌──────────────────────────▼──────────────────────────────────────┐
│                  Gorilla Codec                                   │
│   timestamps: delta-of-deltas + variable-length encoding        │
│   values: XOR of consecutive floats + leading/trailing zeros    │
│   → 10-20× compression for smooth time series                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │ compressed chunk
┌──────────────────────────▼──────────────────────────────────────┐
│                   TSStore (RocksDB via storage)                  │
│   key: ts:{series}:{chunk_start_time}                           │
│   value: compressed Gorilla chunk                               │
└──────────────────────────────────────────────────────────────────┘
         │
         │ background jobs
┌────────┴──────────────────────────────────────────────────────┐
│  AggregateScheduler: maintain 1m/5m/1h/1d rollup views         │
│  Retention: expire chunks older than retention_days            │
└────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 High-Frequency Insert

```
for each sensor reading (1000/sec):
    ts_auto_buffer.insert("temp_sensor_42", {ts: now(), val: 23.4})
        │
        ├─ buffer size < chunk_size → buffer in memory
        └─ buffer full or flush timer expired →
               gorilla.compress(buffer) → compressed chunk
               tsstore.write(series, chunk_start, compressed_chunk)
```

### 4.2 Range Query

```
timeseries.rangeQuery("cpu_usage", from=t1, to=t2, resolution="5m")
    │
    ├─ query_optimizer: chunk pruning → only chunks overlapping [t1, t2]
    │
    ├─ continuous_agg view available for 5m resolution? → read pre-computed
    │       → return rollup directly (no decompression)
    │
    └─ no rollup → fetch raw compressed chunks → gorilla.decompress →
               aggregate on the fly
```

### 4.3 Retention

```
retention.sweep("cpu_usage", retention_days=7)
    │
    ├─ scan chunks with chunk_start < now - 7 days
    └─ delete from storage (RocksDB range delete)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/storage/` | RocksDB storage for compressed chunks |
| **Uses** | `src/scheduler/` | Background aggregate refresh and retention |
| **Called by** | `src/query/` | Timeseries-specific AQL functions |
| **Called by** | `src/server/` | Timeseries API endpoints |
| **Provides to** | `src/observability/` | Ingest rate and compression metrics |

---

## 6. Threading & Concurrency Model

- `TSAutoBuffer` is per-series; each series has its own buffer (no cross-series contention).
- `TSStore` writes use RocksDB's internal concurrency for parallel series.
- `AggregateScheduler` runs background jobs on a dedicated thread pool.
- `Retention` sweep runs as a low-priority background job.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Gorilla compression | 10-20× compression for smooth metrics; reduces I/O and storage |
| Auto-batching | Reduces RocksDB write amplification for high-frequency point inserts |
| Chunk pruning | Query optimizer skips chunks outside query time range |
| Continuous aggregation | Pre-computed rollups avoid re-scanning raw data |
| Hypertable partitioning | Time-chunk layout enables parallel pruning and scans |

---

## 8. Security Considerations

- Time-series data is accessed under RBAC; per-series read permissions are enforced.
- Gorilla chunk encryption is planned (currently unencrypted at rest; use storage-level
  encryption for data at rest protection).

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `timeseries.chunk_duration_s` | 3600 | Chunk time window (1 hour) |
| `timeseries.auto_buffer.max_points` | 1000 | Points before auto-flush |
| `timeseries.auto_buffer.max_duration_s` | 1 | Max buffering time before flush |
| `timeseries.retention.default_days` | 90 | Default retention period |
| `timeseries.continuous_agg.resolutions` | "1m,5m,1h,1d" | Pre-computed rollup resolutions |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Gorilla decode error (corrupt chunk) | Return structured error; do not serve partial data |
| Buffer overflow | Flush immediately; log warning |
| Retention failure | Log error; skip affected chunks; retry next sweep |
| Aggregate refresh failure | Log error; stale rollup served until next successful refresh |

---

## 11. Known Limitations & Future Work

- SIMD-accelerated Gorilla decoder is planned.
- Chunk-level encryption is planned.
- Columnar format for timeseries (Arrow IPC) is planned for analytical exports.
- Adaptive chunk sizing (based on data rate) is planned.

---

## 12. References

- `src/timeseries/README.md` — module overview
- `docs/timeseries/` — timeseries documentation
- Pelkonen et al. (2015): *Gorilla: A Fast, Scalable, In-Memory Time Series Database* (VLDB)
- `ARCHITECTURE.md` (root) — full system architecture
