> **Build:** `cmake --preset release && cmake --build build/release`

# Timeseries Module Headers

This directory contains public header files for the ThemisDB timeseries module.

## Purpose

Public interfaces and declarations for time-series storage, hypertables, continuous aggregates, downsampling, gap-filling, compression, anomaly detection, and metrics ingestion.

## Headers

### Core

- `timeseries.h` — Primary timeseries API
- `tsstore.h` — Low-level time-series store interface
- `hypertable.h` — Hypertable partitioning and chunk management

### Compression

- `gorilla.h` — Gorilla timestamp/value compression
- `gorilla_simd.h` — SIMD-accelerated Gorilla compression
- `compression_selector.h` — Automatic per-column compression selection

### Query & Aggregation

- `query_optimizer.h` — Timeseries query optimizer
- `aggregates.h` — Built-in aggregate functions (sum, avg, first, last, etc.)
- `aggregate_scheduler.h` — Background aggregate refresh scheduler
- `continuous_agg.h` — Continuous aggregate (materialized view) management
- `downsampling.h` — Time-based downsampling policies
- `gap_fill.h` — Gap-fill interpolation for sparse series
- `adaptive_flush_controller.h` — Adaptive write-buffer flush control

### Retention & Lifecycle

- `retention.h` — Retention policy enforcement
- `ts_auto_buffer.h` — Automatic write buffer management
- `ts_auto_buffer_adaptive.h` — Adaptive auto-buffer tuning
- `ts_stream_cursor.h` — Streaming cursor for live ingestion reads

### Encryption

- `encrypted_chunk_store.h` — Encrypted chunk storage backend
- `ts_encrypted_key_rotation.h` — Key rotation for encrypted timeseries chunks

### Monitoring

- `anomaly_detection.h` — Statistical anomaly detection
- `timeseries_metrics.h` — Internal module metrics
- `prometheus_remote_write.h` — Prometheus remote-write ingestion endpoint

## Implementation

See `../../src/timeseries/` for the implementation code.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
