> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/timeseries/ROADMAP.md -->

# Timeseries Module — Public Header Roadmap

**Module Path:** `include/timeseries/`
**Canonical implementation roadmap:** [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)

---

## Overview

Tracks public time-series API contract stability, header coverage, and future public entry points. Runtime chunk lifecycle, compaction, and WAL work remain in:

→ [`../../src/timeseries/ROADMAP.md`](../../src/timeseries/ROADMAP.md)

---

## Current Status

All 22 timeseries headers are present. Public entry points exist for hypertable management, Gorilla and SIMD compression, downsampling, gap-fill, continuous aggregation, anomaly detection, adaptive flushing, encrypted chunk storage, Prometheus remote-write, streaming cursors, and query optimisation.

---

## Completed ✅

- [x] `hypertable.h`, `timeseries.h`, `tsstore.h`, `ts_auto_buffer.h`, `ts_auto_buffer_adaptive.h` — core hypertable and storage contract
- [x] `gorilla.h`, `gorilla_simd.h`, `compression_selector.h` — Gorilla and adaptive compression
- [x] `aggregates.h`, `continuous_agg.h`, `aggregate_scheduler.h`, `query_optimizer.h`, `gap_fill.h` — query, aggregation, and gap-fill
- [x] `downsampling.h`, `retention.h` — downsampling and retention policies
- [x] `anomaly_detection.h`, `timeseries_metrics.h` — anomaly detection and metrics
- [x] `ts_stream_cursor.h`, `encrypted_chunk_store.h`, `ts_encrypted_key_rotation.h` — streaming and encrypted storage
- [x] `prometheus_remote_write.h`, `adaptive_flush_controller.h` — external integration

---

## In Progress

- [ ] Document continuous-aggregate refresh-lag and staleness bounds in `continuous_agg.h` and `aggregate_scheduler.h` (Target: 2026-Q3)
- [ ] Clarify back-pressure semantics and flush-rate contract in `adaptive_flush_controller.h` (Target: 2026-Q3)

---

## Planned

- [ ] `timeseries_policy.h` — unified retention, downsampling, and compression policy contract (Target: 2026-Q4)
- [ ] Add SIMD availability guards and fallback notes to `gorilla_simd.h` (Target: 2026-Q4)
- [ ] Expose benchmark throughput targets for Prometheus remote-write ingest hot path (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Timeseries headers maintain backward compatibility within the active major line; compression-format and partition-schema changes require migration notes and changelog updates.
