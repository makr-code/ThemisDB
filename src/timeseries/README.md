# ThemisDB Timeseries Module

<!-- Status: current | validated: 2026-08-07 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PHASE_6_ACCEPTANCE_CHECKLIST.md · OPERATOR_GUIDE.md -->

## Module Purpose

The timeseries module provides ingestion, storage, compression, retention, aggregation, and query behavior for high-frequency time-series workloads in ThemisDB.

## Production Status

✅ **PRODUCTION-READY** (Phase 1–6 complete, 2026-08-07)
- Core API contract frozen (Phase 1)
- Comprehensive implementation (Phase 2–3)
- Contract test suite complete: TSCH-01..16 (Phase 4)
- Release gates locked: TSRG-01..06 (Phase 5)
- Documentation and acceptance complete (Phase 6)

## Quick Links for Operators and Developers

- **Operators:** See [OPERATOR_GUIDE.md](OPERATOR_GUIDE.md) for deployment, tuning, monitoring, and incident response
- **Performance:** See [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md) for release gate baselines and regression detection
- **Production Requirements:** See [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) for mandatory constraints
- **Phase 6 Acceptance:** See [PHASE_6_ACCEPTANCE_CHECKLIST.md](PHASE_6_ACCEPTANCE_CHECKLIST.md) for completion evidence
- **Architecture:** See [ARCHITECTURE.md](ARCHITECTURE.md) for design overview
- **Roadmap:** See [ROADMAP.md](ROADMAP.md) for delivery phases and status

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| timeseries.cpp | public timeseries API behavior |
| tsstore.cpp | core timeseries storage behavior |
| gorilla.cpp | Gorilla compression/decompression behavior |
| gorilla_simd.cpp | SIMD Gorilla decode behavior |
| ts_auto_buffer.cpp | auto-buffer ingest behavior |
| ts_auto_buffer_adaptive.cpp | adaptive flush behavior |
| adaptive_flush_controller.cpp | standalone flush-control behavior |
| continuous_agg.cpp | continuous aggregation behavior |
| aggregate_scheduler.cpp | aggregate refresh scheduling behavior |
| downsampling.cpp | timeseries downsampling behavior |
| query_optimizer.cpp | timeseries query optimization behavior |
| retention.cpp | retention lifecycle behavior |
| hypertable.cpp | hypertable partition behavior |
| anomaly_detection.cpp | anomaly detection behavior |
| gap_fill.cpp | gap-fill behavior |
| ts_stream_cursor.cpp | streaming cursor behavior |
| prometheus_remote_write.cpp | Prometheus remote-write ingest behavior |
| encrypted_chunk_store.cpp | encrypted chunk storage behavior |
| ts_encrypted_key_rotation.cpp | encrypted chunk key-rotation behavior |
| timeseries_metrics.cpp | metrics/observability behavior |

## Scope

In scope:
- high-frequency time-series ingest, compression, query, retention, and downsampling behavior
- adaptive flush, aggregation scheduling, and monitoring behavior
- encrypted chunk and remote-write timeseries paths

Out of scope:
- generic temporal lifecycle semantics owned by temporal module
- non-timeseries business logic outside module boundaries

## Runtime Behavior and Limits

- ingest and flush behavior is bounded by buffer, chunk, and storage configuration.
- query and range behavior expose explicit results under chunk and aggregation constraints.
- retention, downsampling, and encryption lifecycle behavior remains observable.
- remote-write and anomaly/gap-fill paths remain explicit and diagnosable.

## Sourcecode Verification (Module: timeseries/readme)

- Verified files:
  - src/timeseries/timeseries.cpp
  - src/timeseries/tsstore.cpp
  - src/timeseries/gorilla.cpp
  - src/timeseries/gorilla_simd.cpp
  - src/timeseries/ts_auto_buffer.cpp
  - src/timeseries/ts_auto_buffer_adaptive.cpp
  - src/timeseries/adaptive_flush_controller.cpp
  - src/timeseries/continuous_agg.cpp
  - src/timeseries/aggregate_scheduler.cpp
  - src/timeseries/downsampling.cpp
  - src/timeseries/query_optimizer.cpp
  - src/timeseries/retention.cpp
  - src/timeseries/hypertable.cpp
  - src/timeseries/anomaly_detection.cpp
  - src/timeseries/gap_fill.cpp
  - src/timeseries/ts_stream_cursor.cpp
  - src/timeseries/prometheus_remote_write.cpp
  - src/timeseries/encrypted_chunk_store.cpp
  - src/timeseries/ts_encrypted_key_rotation.cpp
  - src/timeseries/timeseries_metrics.cpp
- Verified behavior surfaces:
  - ingest/compression/query/retention/downsampling/encryption/remote-write paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md