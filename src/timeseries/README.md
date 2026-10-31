---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/timeseries/tsstore.h; include/timeseries/ts_auto_buffer.h; src/timeseries/tsstore.cpp; tests/timeseries/CMakeLists.txt; benchmarks/timeseries/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries module

This directory contains the implementation of the timeseries storage and
processing APIs declared in [`../../include/timeseries/`](../../include/timeseries/).
This overview describes source organization; it does not establish that a
particular build, test suite, benchmark, or production deployment has passed.

## Runtime shape

- `TSStore` stores individual points under `ts:` keys. With Gorilla configured,
  `putDataPoints()` and `putBatch()` group and encode batches under `tsc:` keys.
  `putDataPoint()` is an individual write unless Gorilla is configured and an
  attached auto-buffer accepts the point.
- `TSAutoBuffer` is optional. Its limits and flush behavior are configured by
  `TSAutoBufferConfig`; it holds a non-owning pointer to `TSStore`.
- The module also contains query/aggregation, downsampling, retention,
  encryption/key rotation, Prometheus remote-write decoding, metrics, and
  diagnostics components. Their lifecycles and thread-safety requirements are
  defined by the corresponding public headers.
- `TSStore` timestamps and query bounds use signed Unix epoch milliseconds.
  Ordering validation is disabled for non-positive
  `late_arrival_window_ms`; a positive value rejects timestamps strictly below
  the per-series watermark minus that window.

## Implementation inventory

| Area | Source files |
|---|---|
| Store and entry points | `timeseries.cpp`, `tsstore.cpp`, `hypertable.cpp` |
| Buffering and flush | `ts_auto_buffer.cpp`, `ts_auto_buffer_adaptive.cpp`, `adaptive_flush_controller.cpp` |
| Compression | `gorilla.cpp`, `gorilla_simd.cpp`, `compression_selector.cpp` |
| Query and derived data | `aggregates.cpp`, `aggregate_scheduler.cpp`, `aggregate_scheduler_helper.cpp`, `continuous_agg.cpp`, `downsampling.cpp`, `query_optimizer.cpp`, `gap_fill.cpp`, `ts_stream_cursor.cpp` |
| Retention and encryption | `retention.cpp`, `encrypted_chunk_store.cpp`, `ts_encrypted_key_rotation.cpp` |
| Integrations and diagnostics | `prometheus_remote_write.cpp`, `anomaly_detection.cpp`, `timeseries_metrics.cpp`, `timeseries_incident_taxonomy.cpp`, `ts_edge_case_handler.cpp`, `ts_operator_diagnostics.cpp` |

This list reflects the source files present in `src/timeseries/`; it is not a
claim that every component is wired into every application path.

## Validation and registration

- Focused module tests are source files named `test_*.cpp` in
  [`../../tests/timeseries/`](../../tests/timeseries/). The directory's
  `CMakeLists.txt` creates focused targets from that glob. Storage-backed
  timeseries tests also exist elsewhere under `tests/`.
- Benchmark sources named `bench_*.cpp` in
  [`../../benchmarks/timeseries/`](../../benchmarks/timeseries/) are registered
  when `THEMIS_BUILD_BENCHMARKS` is enabled. Some benchmark sources use
  in-process models/stubs and do not measure the complete TSStore runtime.
- For target names and configuration, consult those CMake files and the
  configured build tree. No build, test, or benchmark execution is asserted by
  this document.

## Developer references

- [API guide](API.md) and [implementation guide](IMPLEMENTATION.md)
- [Architecture](ARCHITECTURE.md) and [data model](MODELING.md)
- [Focused developer guide](MODULE_GUIDE.md)
- [Operations](OPERATIONS.md), [runbook](RUNBOOK.md), and [trust boundaries](TRUST.md)
- [Roadmap](ROADMAP.md) and [future enhancements](FUTURE_ENHANCEMENTS.md)
