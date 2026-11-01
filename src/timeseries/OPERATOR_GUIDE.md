---
Author: "ThemisDB Team"
Created: "2026-08-07"
Last Updated: "2026-08-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "src/timeseries/OPERATIONS.md; src/timeseries/RUNBOOK.md; include/timeseries/tsstore.h; include/timeseries/ts_auto_buffer.h; include/timeseries/retention.h; include/timeseries/encrypted_chunk_store.h; include/timeseries/timeseries_metrics.h; src/timeseries/timeseries_metrics.cpp; include/core/production_mode.h"
---

> Exact model backend not verifiable from final document text alone.

# Timeseries operator guide

This guide covers the timeseries module's C++ configuration, observability,
tuning signals, and operational lifecycle. It is not a standalone service
manual. The module does not define HTTP routes, a deployment YAML schema, a
service CLI, scrape endpoint, or alert policy; those must be taken from the
owning server and platform sources.

The previous guide included unsupported module-specific environment settings,
example HTTP routes, unverified metric names, nanosecond-looking timestamps, and
performance/SLA figures that could not be tied to the module implementation.
Those are not module configuration or verified deployment facts and are
intentionally not repeated here.

## Direct API contract

`TSStore::DataPoint::timestamp_ms` and `QueryOptions` time bounds are signed
Unix epoch **milliseconds**. `TSStore::Config::late_arrival_window_ms` is also
in milliseconds. Any HTTP request field and its conversion to this API are
server-owned; do not copy nanosecond values from an integration example into
these C++ fields. See [`tsstore.h`](../../include/timeseries/tsstore.h).

## Module configuration

These are C++ struct fields, not environment-variable names. Defaults shown in
the public headers are code defaults, not workload sizing recommendations.

| Component | Source-backed settings and units |
|---|---|
| `TSStore::Config` | `compression` (`None` or `Gorilla`), `chunk_size_hours`, and `late_arrival_window_ms`. |
| `TSAutoBufferConfig` | Point limits (`max_points_per_buffer`, `max_total_points`), `flush_interval` in milliseconds, memory limits in bytes, flush and Gorilla batch sizes, optional deduplication, and optional adaptive flush/backpressure settings. `max_buffer_bytes` controls the non-blocking `push()` full condition; zero disables that threshold. |
| `RetentionPolicy` | Per-metric retention periods in seconds. Background cleanup is opt-in through `RetentionManager::startAsync(interval)`; constructing a policy alone does not start it. |
| `EncryptedChunkStore` | Caller-provided current-key and historical-key lookup callbacks. Retain historical keys while stored chunks may still refer to them; supply audit dependencies as required by the deployment. |

See [`ts_auto_buffer.h`](../../include/timeseries/ts_auto_buffer.h),
[`retention.h`](../../include/timeseries/retention.h), and
[`encrypted_chunk_store.h`](../../include/timeseries/encrypted_chunk_store.h)
for complete fields and API contracts. The repository-wide `ProductionMode`
helper reads `THEMIS_PRODUCTION_MODE` and `THEMIS_ENVIRONMENT`; these are
generic core settings, not timeseries-specific controls. Their implementation
and accepted values are in [`production_mode.h`](../../include/core/production_mode.h).
They do not replace the server's authentication, authorization, audit, or
deployment configuration.

## Buffer operation and tuning

Configure limits for the observed workload, then inspect
`TSAutoBuffer::getStats()` before changing them. Relevant statistics include
buffered/flushed points, current point count and memory, flush counts and
triggers, overflow and per-metric memory rejections, deduplicated points,
backpressure events, EWMA latency, and adaptive batch size.

For delayed or stalled writes, compare those statistics with the active
`TSAutoBufferConfig` and the underlying `TSStore` write result. `push()` is
non-blocking and can return `BUFFER_FULL`; the caller owns its retry/backoff
policy. A confirmed healthy store can be drained with `flush()` or
`flushFor(metric, entity)`. `stop()` joins the background thread and flushes
remaining points. Do not destroy the store or RocksDB dependency while the
buffer can still use it.

Tune point/memory limits, flush interval, and batch sizes from measured
backlog, memory, flush latency, and write behavior. Adaptive flush is optional.
The header defaults do not establish safe capacity for a particular deployment,
and this guide does not claim a throughput target or latency SLA. Benchmark
results and scope are summarized in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Metrics and monitoring

`TimeSeriesMetrics::exportPrometheus()` produces module metric text and
`exportJson()` produces a JSON snapshot. The application must wire the metrics
object and decide how to expose/export it; these methods do not define an HTTP
endpoint or scrape configuration. Metric names below are taken from the
implementation in [`timeseries_metrics.cpp`](timeseries_metrics.cpp):

| Area | Exported examples |
|---|---|
| Ingestion | `themis_timeseries_data_points_written_total`, `themis_timeseries_batches_written_total`, `themis_timeseries_write_errors_total`, `themis_timeseries_late_arrival_rejected_total` |
| Buffer | `themis_timeseries_autobuffer_backpressure_total`, `themis_timeseries_autobuffer_overdue_flush_total` |
| Queries | `themis_timeseries_queries_executed_total`, `themis_timeseries_query_latency_ms_avg`, `themis_timeseries_data_points_returned_total` |
| Storage and retention | `themis_timeseries_current_data_points`, `themis_timeseries_current_storage_bytes`, `themis_timeseries_retention_runs_total`, `themis_timeseries_data_points_deleted_total` |
| Compression and aggregates | `themis_timeseries_compression_ratio_avg`, `themis_timeseries_continuous_agg_refreshes_total`, `themis_cagg_refresh_latency_ms_avg`, `themis_cagg_refresh_lag_ms` |

Additional counters and gauges are listed in `exportPrometheus()` and
`exportJson()`. Some aggregate metrics appear only after values have been
recorded. Define scrape settings, labels/cardinality policy, dashboards, and
alert thresholds in the integrating server/platform documentation; do not infer
an alert threshold from a metric name.

## Production and recovery checks

- Keep the non-owned RocksDB database and store alive until module components
  have stopped. Stop the auto-buffer before destroying its store.
- Retention deletes expired data; the module has no undelete operation. Check
  the policy and audit evidence before applying cleanup, and use the deployment's
  tested backup/restore procedure for recovery.
- For encrypted chunks, verify historical-key lookup before recovery and retain
  required keys. Do not replace original ciphertext during investigation.
- Preserve `TSAutoBufferStats`, `TimeSeriesMetrics`, and diagnostics before
  restarting or clearing in-memory state when investigating an incident.

For startup/shutdown ordering, backup boundaries, and dependency ownership, use
[`OPERATIONS.md`](OPERATIONS.md). For symptom-based triage—including retention,
buffer, encrypted chunk, and remote-write parser errors—use [`RUNBOOK.md`](RUNBOOK.md).
The module's remote-write parser decodes protobuf payloads; HTTP transport,
authorization, endpoint health, and retry policy belong to the server/integration.
For security boundaries and production requirements, see [`TRUST.md`](TRUST.md)
and [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).
