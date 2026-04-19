# Observability Roadmap

**Module:** `src/observability`  
**Status:** Q1 Complete ✅ | Q2–Q4 Planned  
**Last Updated:** April 2026

This document maps the planned observability improvements for ThemisDB across four
quarters, tracking the gap-analysis from
[issue: Observability-Modul – Production Readiness](../../issues) and the detailed
enhancement catalogue in
[`src/observability/FUTURE_ENHANCEMENTS.md`](../src/observability/FUTURE_ENHANCEMENTS.md).

---

## Q1 – Alertmanager Production Integration & Live Metrics  ✅ COMPLETED

### Alertmanager – Prometheus Alertmanager v2 HTTP API  ✅
- ✅ Replaced stub `sendAlert` / `resolveAlert` / `silenceAlert` / `testConnection` with
  real HTTP calls via `HTTPClientPool` (Boost.Beast).
- ✅ POST `/api/v2/alerts` for firing and resolving alerts (ISO-8601 `endsAt`).
- ✅ POST `/api/v2/silences` with matcher payload and time window.
- ✅ GET `/api/v2/status` for health-check on `initialize()`.
- ✅ Configurable retry (`retry_count`, `retry_delay_ms`) in `AlertmanagerConfig`.
- ✅ Auth-token (`Authorization: Bearer`) header support.
- ✅ Failover: log-and-continue when Alertmanager is unreachable; return `Result<void>`
  error to allow callers to decide on fallback behaviour.
- ✅ Auto-configured from `THEMIS_ALERTMANAGER_URL` / `THEMIS_ALERTMANAGER_TOKEN` env vars.

### OTel / Distributed Tracing – Span & Trace Export  ✅
- ✅ `Tracer::initialize()` OTLP HTTP exporter path (no-op mode via `#ifdef THEMIS_ENABLE_TRACING`).
- ✅ W3C TraceContext (`traceparent` / `tracestate`) propagation via `Tracer::startSpanFromHeaders()`.
- ✅ HTTP server wires `startSpanFromHeaders` for every incoming request (all child spans
  participate in the caller's distributed trace automatically).
- ✅ `ScopedSpan` / `TracedSpan` used across query, storage, and shard-RPC paths (191+ call sites).

### Live Metrics Dashboard (HTML / CLI)  ✅
- ✅ `MetricsCollector::getPrometheusMetrics()` output appended to `/metrics` endpoint.
- ✅ `/metrics/html` HTTP endpoint renders a lightweight dark-mode HTML dashboard.
- ✅ `themis_build_info{version, build_type, compiler, edition}` metric added.
- ⬜ `themis-metrics` CLI command (deferred to Q2 tooling work).

### Operator API  ✅
- ✅ `GET /api/v1/observability/alerts` – list active alerts as JSON.
- ✅ `POST /api/v1/observability/alerts/{id}/silence` – silence via REST API.
- ✅ `GET /api/v1/observability/health` – aggregate health status (alertmanager, tracing, MetricsCollector).
- ✅ All new endpoints documented in OpenAPI spec (`GET /api/openapi.json`).

---

## Q2 – Anomaly Detection, Baselines & SLA / Deadman Switches

### Statistical Anomaly Detection
- Implement `StatisticalAnomalyDetector` with Z-score, modified Z-score, and IQR
  methods (see `FUTURE_ENHANCEMENTS.md`).
- Integrate with `MetricsCollector` so detectors are evaluated on every metric flush.
- On anomaly: fire an alert via `DefaultAlertmanager::sendAlert()`.

### Baseline Comparison
- Implement `BaselineComparator` for comparing current metric windows against a
  rolling baseline (default 7-day window).
- REST endpoint `GET /api/v1/observability/baseline/{metric}` to retrieve baseline
  statistics.

### Adaptive SLO / SLA Alerter
- Define SLO targets per metric (e.g., `query_latency_ms p99 < 500`).
- `SLOEvaluator` checks targets every N seconds; fires CRITICAL alert on breach.
- Deadman switch: alert if no data arrives within a configurable window
  (detects silent failures / stalled exporters).

### Stream Metrics API
- WebSocket endpoint `/metrics/stream` – push metric deltas to subscribed clients at
  configurable interval.
- Server-Sent Events alternative `/metrics/events` for simpler browser integration.

---

## Q3 – External Exporters & Real-Time Heatmaps

### Grafana Loki Log Exporter
- Implement `LokiLogExporter : ILogger` that batches log lines and posts them to
  the Loki push API (`/loki/api/v1/push`).
- Static labels: `component`, `environment`, `instance_id`.
- Flush on buffer full or configurable time interval.

### DataDog / StatsD Exporter
- Implement a StatsD UDP exporter for compatibility with DataDog agent.
- Map ThemisDB counters → StatsD counters, gauges → gauges, histograms → timers.

### AWS CloudWatch Metrics Exporter
- Publish selected metrics to CloudWatch via `PutMetricData` API using the AWS SDK
  (or lightweight HTTP calls with SigV4 signing).

### Elastic APM Exporter
- Export `TracedSpan` data to Elastic APM intake API.

### Real-Time Heatmaps (HTML)
- `GET /api/v1/observability/heatmap/{metric}` – returns an SVG or JSON heatmap of
  latency buckets over time.

### Operator Reporting Tools
- NDJSON audit report: `GET /api/v1/observability/audit?from=&to=&format=ndjson`.
- CSV report: `GET /api/v1/observability/report?format=csv`.

---

## Q4 – Continuous Profiling, Predictive Alerts & Runbooks

### Continuous Profiling (CPU / Memory / Lock)
- Integrate `gperftools` or `perf`-based sampler for always-on CPU profiling
  at < 1% overhead.
- Implement `ContinuousProfiler` with configurable sampling rate and snapshot
  interval.
- Expose flamegraph data at `GET /api/v1/observability/profile/cpu`.
- Memory leak detector: track heap growth rate; alert if > configurable threshold.
- Lock-contention analyser: report hot mutexes via `LOCK_CONTENTION` alert.

### Predictive Alerts
- `CapacityPlanner` implementation: linear / exponential growth regression on
  storage, QPS, and memory metrics.
- Fire WARN alert when projected exhaustion is within 30 days.

### Audit API
- `GET /api/v1/observability/audit` – paged, filterable audit log with NDJSON
  and CSV export.
- Retention policy: configurable max age and max rows.

### Runbooks & Playbooks
- Machine-readable runbook index at `GET /api/v1/observability/runbooks`.
- Each runbook linked from alert annotations (`runbook_url` label).
- Initial runbooks for common alerts: `HighMemoryUsage`, `HighWriteAmplification`,
  `AlertmanagerDown`, `SLOBreach`.

### Operator Feedback Channels
- POST `/api/v1/observability/feedback` – structured feedback from operators
  (acknowledge alert, add comment, tag false-positive).
- Feedback stored alongside alert history for ML training (Q2+ anomaly model).

---

## Related Documents

| Document | Description |
|---|---|
| [`src/observability/FUTURE_ENHANCEMENTS.md`](../src/observability/FUTURE_ENHANCEMENTS.md) | Detailed design & code snippets |
| [`docs/observability/README.md`](observability/README.md) | Current observability overview |
| [`docs/observability/llm_metrics_schema.md`](observability/llm_metrics_schema.md) | LLM-specific metric schema |
| [`docs/PROMETHEUS_INTEGRATION_COMPLETE.md`](PROMETHEUS_INTEGRATION_COMPLETE.md) | Prometheus integration status |
| [`docs/GRAFANA_METRICS_COMPLETE.md`](GRAFANA_METRICS_COMPLETE.md) | Grafana dashboard status |
| [`roadmap.md`](../roadmap.md) | Top-level product roadmap |
