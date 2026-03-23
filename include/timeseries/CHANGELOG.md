<!-- Status: current | validated: 2026-03-22 -->

# Changelog — include/timeseries/

All notable changes to the **public headers** of the `timeseries` module are
documented here. Follow [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
conventions.

For implementation-level changes see `../../src/timeseries/CHANGELOG.md`.

---

## [Unreleased]

### Planned
- `ts_stream_cursor.h` — streaming cursor API for large result sets
  (Target: Q3 2026)
- `ts_columnar_store.h` — columnar layout option for analytical workloads
  (Target: Q4 2026)

---

## [1.5.0] — 2026-03-12

### Added
- `ts_encrypted_key_rotation.h` — `KeyRotationManager` and `RotationPolicy` for
  online key rotation without chunk-level downtime.
- `ts_auto_buffer_adaptive.h` — `TSAutoBufferAdaptive` extends `TSAutoBuffer`
  with throughput-aware dynamic capacity adjustment.
- `timeseries_metrics.h` — `TSMetrics` and `MetricSnapshot` expose internal
  performance counters as a stable public interface.
- `gorilla_simd.h` — AVX2 and ARM NEON accelerated encoder/decoder paths;
  feature-detected at runtime with automatic scalar fallback.
- `TSConfig` gains `max_chunk_size_bytes` (default: 64 MiB) and
  `key_rotation_interval_s` (default: 86400) fields — fully backward-compatible.

### Changed
- `prometheus_remote_write.h` — `RemoteWriteConfig` adds `snappy_compression`
  flag (default `true`); existing callers unaffected.
- `query_optimizer.h` — `QueryPlan` is now move-constructible to reduce copy
  overhead in plan-cache scenarios.

### Fixed
- `encrypted_chunk_store.h` — `ChunkKey` copy constructor now performs deep copy
  of key material (was shallow in v1.4.x).

---

## [1.4.0] — 2025-12-01

### Added
- `ts_auto_buffer.h` — fixed-capacity SPSC ring buffer for ingest staging.
- `retention.h` — `RetentionPolicy` and `RetentionExecutor` for time-based and
  size-based chunk expiry with configurable grace periods.
- `prometheus_remote_write.h` — initial Prometheus remote-write protocol support.

### Changed
- `hypertable.h` — `HypertablePartition` now exposes `chunk_count()` as
  `[[nodiscard]]`.
- `aggregates.h` — `AggregateResult` moves to value semantics (was pointer-based).

---

## [1.3.0] — 2025-09-15

### Added
- `continuous_agg.h` — `ContinuousAggregate` and `MaterialisedView` for
  incremental rollup materialisation.
- `aggregate_scheduler.h` — background scheduler for aggregate refresh jobs.
- `downsampling.h` — LTTB and time-bucket downsampling strategies.

### Changed
- `gorilla.h` — `GorillaEncoder` is now reusable across multiple streams via
  `reset()`.

---

## [1.2.0] — 2025-06-20

### Added
- `encrypted_chunk_store.h` — AES-256-GCM per-chunk encryption with `ChunkKey`.
- `query_optimizer.h` — initial predicate push-down and chunk skip-list optimiser.

---

## [1.1.0] — 2025-03-10

### Added
- `gorilla.h` — Gorilla XOR compression encoder/decoder.
- `tsstore.h` — low-level chunk read/write and reference-counted `ChunkRef`.
- `timeseries_metrics.h` (internal preview; promoted to public API in v1.5.0).

---

## [1.0.0] — 2024-12-01

### Added
- `timeseries.h` — `TimeSeriesEngine` root object and `TSConfig`.
- `hypertable.h` — `Hypertable` and `HypertablePartition` for time-range
  partitioning.
- `aggregates.h` — scalar aggregate primitives.

---

> Full implementation changelog: `../../src/timeseries/CHANGELOG.md`
