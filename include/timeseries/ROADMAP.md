---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/timeseries/; src/timeseries/ROADMAP.md; tests/timeseries/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

<!-- Planning inventory checked against current headers: 2026-10-31 -->
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

All 26 headers listed in [`README.md`](README.md) are present. This records
header inventory only; it does not assert runtime integration, API stability,
or successful build/test execution. The public declarations include store,
partitioning, compression, downsampling, gap-fill, continuous aggregation,
anomaly detection, buffering, encryption, remote-write decoding, cursors,
metrics, and diagnostics.

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

- [ ] Reconcile continuous-aggregate refresh-lag and staleness documentation with the current declarations in `continuous_agg.h` and `aggregate_scheduler.h` (planning item; completion not established by this inventory)
- [ ] Clarify back-pressure semantics and flush-rate contract in `adaptive_flush_controller.h` (planning item; completion not established by this inventory)

---

## Planned

- [ ] `timeseries_policy.h` — unified retention, downsampling, and compression policy contract (Target: 2026-Q4)
- [ ] Add SIMD availability guards and fallback notes to `gorilla_simd.h` (Target: 2026-Q4)
- [ ] Expose benchmark throughput targets for Prometheus remote-write payload decoding (Target: 2026-Q4)

---

## Breaking Change History

No breaking-change history was established by this header-inventory review.
Do not treat the presence of the current declarations as a backward
compatibility guarantee; use the repository's release and versioning sources
for supported compatibility policy.
