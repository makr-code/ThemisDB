<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/timeseries/ -->

# Architecture — include/timeseries/

This document describes the public-header architecture of the **timeseries** module.
It covers design principles, the interface inventory, and cross-references to the
corresponding implementation in `../../src/timeseries/`.

---

## Overview

The `timeseries` module provides a high-performance, encrypted time-series engine
for ThemisDB.  It is responsible for:

- **Ingestion & Storage** — hypertable-based partitioning, auto-buffering, and
  encrypted chunk persistence.
- **Compression** — Gorilla XOR encoding with optional SIMD acceleration.
- **Continuous Aggregation** — materialised rollups with configurable refresh
  schedules.
- **Query Optimisation** — predicate push-down, chunk pruning, and adaptive
  downsampling.
- **Export** — Prometheus remote-write protocol for interoperability with
  observability stacks.
- **Security** — at-rest encryption with online key rotation per chunk.

All public contracts are expressed through the headers listed below.  Callers must
link against the compiled `timeseries` library but are otherwise insulated from
internal implementation details.

---

## Design Principles

- **Immutable chunk semantics** — each written chunk is sealed and
  content-addressed; mutations create a new chunk revision, preserving audit
  integrity.
- **Compression before encryption** — Gorilla XOR is applied first to maximise
  entropy reduction, then AES-256-GCM encryption is layered on each chunk block.
- **Zero-copy query path** — query results are materialized directly into
  caller-owned buffers to avoid heap fragmentation under high concurrency.
- **Adaptive resource management** — `ts_auto_buffer_adaptive.h` monitors write
  throughput and resident-set size, dynamically resizing ring buffers to prevent
  OOM under burst loads.
- **Open telemetry interoperability** — Prometheus remote-write (protobuf + snappy)
  is a first-class export path, requiring no external adapter layer.

---

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `timeseries.h` | `TimeSeriesEngine`, `TSConfig` | Root engine initialisation and lifecycle |
| `tsstore.h` | `TSStore`, `ChunkRef` | Low-level chunk read/write and iteration |
| `hypertable.h` | `Hypertable`, `HypertablePartition` | Time-range partitioning of metric streams |
| `aggregates.h` | `AggregateFunction`, `AggregateResult` | Scalar aggregate primitives (min/max/avg/sum/count) |
| `aggregate_scheduler.h` | `AggregateScheduler`, `ScheduleEntry` | Background scheduling of aggregate refresh jobs |
| `continuous_agg.h` | `ContinuousAggregate`, `MaterialisedView` | Incremental materialisation of rollup views |
| `downsampling.h` | `DownsamplePolicy`, `DownsampleResult` | LTTB and time-bucket downsampling strategies |
| `gorilla.h` | `GorillaEncoder`, `GorillaDecoder` | Gorilla XOR bit-stream compression |
| `gorilla_simd.h` | `GorillaSIMDEncoder`, `GorillaSIMDDecoder` | AVX2/NEON-accelerated Gorilla paths |
| `encrypted_chunk_store.h` | `EncryptedChunkStore`, `ChunkKey` | AES-256-GCM per-chunk encryption and decryption |
| `ts_encrypted_key_rotation.h` | `KeyRotationManager`, `RotationPolicy` | Online key rotation without service interruption |
| `ts_auto_buffer.h` | `TSAutoBuffer` | Fixed-capacity ring buffer for ingest staging |
| `ts_auto_buffer_adaptive.h` | `TSAutoBufferAdaptive` | Throughput-aware adaptive ring-buffer sizing |
| `retention.h` | `RetentionPolicy`, `RetentionExecutor` | Time-based and size-based chunk expiry |
| `query_optimizer.h` | `QueryOptimizer`, `QueryPlan` | Chunk-pruning and predicate push-down |
| `prometheus_remote_write.h` | `PrometheusRemoteWriter`, `RemoteWriteConfig` | Prometheus remote-write (protobuf/snappy) export |
| `timeseries_metrics.h` | `TSMetrics`, `MetricSnapshot` | Internal instrumentation counters and gauges |

---

## Component Diagram

```
+---------------------------------------------------------+
|                   TimeSeriesEngine                      |
|  +-------------+  +--------------+  +---------------+  |
|  |  Hypertable |  | TSAutoBuffer |  | QueryOptimizer|  |
|  |  Partition  |  |  (Adaptive)  |  | + Plan        |  |
|  +------+------+  +------+-------+  +-------+-------+  |
|         |                |                  |           |
|  +------+----------------+------------------+------+   |
|  |              TSStore / ChunkRef                  |   |
|  +------+-------------------------------------------+   |
|         |                                               |
|  +------+-----------+  +----------------------------+  |
|  | GorillaEncoder   |  | EncryptedChunkStore        |  |
|  | (+ SIMD)         |  | + KeyRotationManager       |  |
|  +------------------+  +----------------------------+  |
|                                                         |
|  +------------------+  +----------------------------+  |
|  | ContinuousAgg    |  | PrometheusRemoteWriter     |  |
|  | + Scheduler      |  | + RetentionExecutor        |  |
|  +------------------+  +----------------------------+  |
+---------------------------------------------------------+
```

---

## Related Documents

- `README.md` — module overview and quick-start
- `ROADMAP.md` — planned enhancements and milestones
- `FUTURE_ENHANCEMENTS.md` — design sketches for future capabilities
- `SECURITY.md` — threat model and encryption controls
- `AUDIT.md` — header audit results

---

> Implementation in `../../src/timeseries/`
