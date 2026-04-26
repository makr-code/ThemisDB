> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Observability Module — Architecture Guide

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/observability/README.md -->

**Version:** 1.1
**Last Updated:** 2026-04-06
**Module Path:** `src/observability/`

---

## 1. Overview

The Observability module provides ThemisDB's production monitoring infrastructure:
Prometheus-compatible metrics collection and exposition, OpenTelemetry distributed tracing,
query profiling, storage profiling, automated performance analysis, and alerting integration.

It is the central hub for all runtime instrumentation; every other module that records
metrics or emits traces does so through the interfaces provided here or through the
`ConcernsContext` from `src/core/`.

---

## 2. Design Principles

- **Prometheus-Native** – metrics are exposed in Prometheus text format on `/metrics`
  without requiring an external agent.
- **Non-Invasive Profiling** – `query_profiler.cpp` and `storage_profiler.cpp` add
  structured timing without adding locks to the query or storage hot paths.
- **Automated Analysis** – `performance_analyzer.cpp` correlates metrics to identify
  issues (high P99 latency, write amplification, memory pressure) and generates
  recommendations automatically.
- **External Sinks** – log storage, dashboards, and alerting backends are external
  (Loki, Grafana, Alertmanager, PagerDuty); this module only produces the data.
- **Health Endpoints** – readiness and liveness probes for Kubernetes orchestration.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `metrics_collector.cpp` | Prometheus metric registry: counters, gauges, histograms, summaries |
| `query_profiler.cpp` | Per-query execution plan timing: phases, operators, index usage |
| `storage_profiler.cpp` | RocksDB performance metrics: compaction, amplification, cache |
| `performance_analyzer.cpp` | Automated issue detection and recommendations |
| `alertmanager.cpp` | Alert threshold monitoring and Alertmanager webhook integration |
| `alerting_engine.cpp` | Rule-based alerting with pluggable notification channels |
| `continuous_profiler.cpp` | Continuous profiling (pprof-compatible CPU/memory/mutex/block profiles) |
| `tracer.cpp` | `ObservabilityTracer` — W3C Trace Context propagation, span ring buffer, MetricsCollector integration |
| `opentelemetry_tracer.cpp` | `OpenTelemetryTracer` — OTLP gRPC/HTTP, Jaeger, Zipkin exporters, W3C Baggage |
| `log_aggregator.cpp` | `LogAggregator` — structured JSON log collection, trace-context correlation, ring buffer |
| `log_search_engine.cpp` | `LogSearchEngine` — query structured logs by field, level, time range, message |
| `ebpf_tracer.cpp` | eBPF-based kernel tracing (perf counters; guarded by `THEMIS_ENABLE_EBPF`) |
| `distributed_flame_graph.cpp` | Distributed flame graph across cluster nodes |
| `metric_aggregator.cpp` | Rate, histogram aggregation, cross-shard pre-aggregation, cardinality rollup |
| `metric_anomaly_detector.cpp` | ML-based anomaly detection bridging analytics `StreamingAnomalyDetector` |
| `ml_anomaly_detector.cpp` | ML anomaly detector with ARIMA/Holt-Winters forecast-based detection |
| `metrics_stream_server.cpp` | Real-time metric streaming via WebSocket/SSE with `SendFn` callback |
| `advanced_metrics.cpp` | Summary, ExponentialHistogram, Cardinality, TimeWeightedAverage, Rate |
| `slo_reporter.cpp` | SLO/SLA burn-rate alerting (FAST 14.4×, MEDIUM 6×, SLOW 3× windows) |
| `root_cause_analyzer.cpp` | Automated root cause analysis via correlation and causal graph inference |
| `tenant_metrics_namespace.cpp` | Per-tenant metric namespacing with independent cardinality budgets |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    All ThemisDB Modules                         │
│   metrics.incrementCounter("query_total") / tracer.startSpan()  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  MetricsCollector (singleton)                    │
│                                                                  │
│  Prometheus registry: counters, gauges, histograms, summaries    │
│  /metrics HTTP endpoint (Prometheus scrape)                     │
└──────────────┬──────────────────────────────────────────────────┘
               │
  ┌────────────┴───────────────────────────────┐
  │                                            │
┌─▼─────────────────┐           ┌──────────────▼────────────────┐
│  QueryProfiler    │           │  StorageProfiler               │
│  plan phases:     │           │  RocksDB stats, amplification, │
│  parse/optimize/  │           │  block cache hit rate, SST     │
│  execute timings  │           │  file counts, compaction       │
└─┬─────────────────┘           └──────────────┬────────────────┘
  │                                            │
  └─────────────────┬──────────────────────────┘
                    │
       ┌────────────▼────────────────────────┐
       │       PerformanceAnalyzer           │
       │  correlate metrics → detect issues  │
       │  generate recommendations           │
       └────────────┬────────────────────────┘
                    │ alert threshold exceeded
       ┌────────────▼────────────────────────┐
       │          Alertmanager               │
       │  POST webhook → Alertmanager/PagerDuty│
       └─────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Metrics Scrape (Prometheus)

```
Prometheus: GET /metrics
    │
    ▼
MetricsCollector::exportPrometheusText()
    │
    ├─ collect all registered metrics (counters, gauges, histograms)
    └─ serialize to Prometheus text format
```

### 4.2 Query Profiling

```
QueryEngine: QueryProfiler::beginQuery(query_id)
    │
    ├─ phase("parse") → end("parse") → record timing
    ├─ phase("optimize") → end("optimize") → record timing
    ├─ phase("execute") → end("execute") → record timing
    │
    ▼
QueryProfiler::endQuery(query_id)
    │
    ├─ update histogram: query_duration_seconds
    ├─ update histogram: query_plan_stages
    └─ store profile for /query/profile endpoint
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | All modules | `MetricsCollector` singleton for metrics recording |
| **Uses** | `src/core/` | `ConcernsContext::metrics()` integration |
| **Provides to** | Prometheus | `/metrics` HTTP endpoint |
| **Provides to** | `src/server/` | `/health/ready`, `/health/live` endpoints |
| **Provides to** | Alertmanager | Alert webhook notifications |
| **Provides to** | pprof tooling | CPU and memory profiles |

---

## 6. Threading & Concurrency Model

- `MetricsCollector` singleton uses lock-free atomic counters for hot-path metrics.
- Histogram updates use CAS (compare-and-swap) on bucket counters.
- `QueryProfiler` uses per-query context objects (no shared state between queries).
- `ContinuousProfiler` runs on a dedicated background thread at configurable intervals.
- `Alertmanager` webhook calls are fire-and-forget on a separate I/O thread.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Lock-free counters | Atomic increments; no mutex on metrics hot path |
| Lazy histogram | Histogram buckets computed on scrape, not on record |
| Profile sampling | Continuous profiler samples at 99 Hz to minimize overhead |
| Async alerts | Alert webhook calls are non-blocking |

---

## 8. Security Considerations

- `/metrics` endpoint should be restricted to the internal monitoring network; exposing
  it publicly leaks performance and capacity information.
- `/health` endpoints are safe to expose publicly (return only OK/not-OK).
- Distributed trace data may contain query structure; sampling should be tuned to avoid
  capturing sensitive query parameters.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `observability.metrics.port` | 9091 | Prometheus metrics port |
| `observability.tracing.enabled` | false | Enable OpenTelemetry tracing |
| `observability.tracing.sample_rate` | 0.1 | Trace sampling rate |
| `observability.profiler.enabled` | false | Enable continuous profiler |
| `observability.profiler.interval_s` | 30 | Profiling interval |
| `observability.alertmanager.url` | "" | Alertmanager webhook URL |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Metrics registry full | Drop new metrics; log warning |
| Alertmanager unreachable | Buffer alerts locally; retry; drop on overflow |
| Profiler overhead too high | Auto-disable if CPU usage exceeds threshold |

---

## 11. Known Limitations & Future Work

- Telemetry aggregation across shards is eventually consistent.
- OTLP exemplar linking (attaching trace context to individual histogram observations) is a planned interface (`otlp_exemplar.h`, target Q3 2026) and not yet integrated in the export pipeline.
- Log aggregation to external Loki requires the Grafana agent (not included).
- Distributed trace data may contain query structure; sampling should be tuned to avoid capturing sensitive query parameters.

---

## 12. References

- `src/observability/README.md` — module overview
- `docs/monitoring.md` — monitoring guide
- `docs/prometheus_metrics.md` — metrics reference
- `ARCHITECTURE.md` (root) — full system architecture
