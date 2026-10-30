---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/observability/metrics_collector.h"
  - "../../include/observability/tracer.h"
  - "../../include/observability/opentelemetry_tracer.h"
  - "../../include/observability/alerting_engine.h"
  - "../../include/observability/query_profiler.h"
  - "metrics_collector.cpp"
  - "tracer.cpp"
  - "opentelemetry_tracer.cpp"
  - "alerting_engine.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Observability Module Architecture

## Scope, boundaries, and ownership

`src/observability/` owns in-process telemetry collection and observability processing: metrics, spans/trace propagation, query/storage profiling, aggregation, alert rules, log analysis, and diagnostics. External telemetry backends and notification services remain integrations; this module does not own application request semantics.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Metrics | [`MetricsCollector`](../../include/observability/metrics_collector.h), [`AdvancedMetrics`](../../include/observability/advanced_metrics.h), [`TenantMetricsNamespace`](../../include/observability/tenant_metrics_namespace.h) | Record/aggregate metrics, enforce metric/label constraints, and expose text/health data. |
| Tracing | [`ObservabilityTracer`](../../include/observability/tracer.h), [`OpenTelemetryTracer`](../../include/observability/opentelemetry_tracer.h), [`DistributedTracingSDK`](../../include/observability/distributed_tracing_sdk.h) | Span lifecycle and propagation/export integration. |
| Profiling and diagnostics | [`QueryProfiler`](../../include/observability/query_profiler.h), [`StorageProfiler`](../../include/observability/storage_profiler.h), [`PerformanceAnalyzer`](../../include/observability/performance_analyzer.h) | Collect query/storage profiling data and derive diagnostics. |
| Alerts and analysis | [`AlertingEngine`](../../include/observability/alerting_engine.h), [`Alertmanager`](../../include/observability/alertmanager.h), [`RootCauseAnalyzer`](../../include/observability/root_cause_analyzer.h) | Evaluate configured rules, route notifications, and analyze incidents. |
| Provenance | [`ProvenanceStore`](../../include/observability/provenance_store.h) | Persist and query retrieval-step provenance records through the implementation's RocksDB-backed store. |

## Data and event flow

1. Instrumented modules call metric and tracing APIs with measurements, labels, and span context.
2. Collectors validate bounded inputs and aggregate/export data; trace export may use the API module's OTLP exporter.
3. Profilers and log/metric analysis components derive diagnostics; the alerting engine evaluates configured rules and invokes notification channels.
4. Retrieval provenance records are stored through a separate provenance-store interface.

See [`metrics_collector.cpp`](metrics_collector.cpp), [`tracer.cpp`](tracer.cpp), [`opentelemetry_tracer.cpp`](opentelemetry_tracer.cpp), and [`provenance_store.cpp`](provenance_store.cpp).

## Failure domains and edge cases

- Invalid metric labels/cardinality limits can reject or constrain metric series; metric input problems must not be interpreted as application success evidence.
- Exporter unavailability and notification-channel failures are downstream integration failures; they can affect telemetry delivery without changing the instrumented operation's result.
- Profiling and tracing may be disabled or sampled; absence of a span/sample is not proof that no operation occurred.
- Provenance persistence has its own RocksDB/open/write failure domain, separate from in-memory metric or tracing work.

## Explicit non-goals

- Owning the external Prometheus, OTLP, Jaeger, Zipkin, or notification backend.
- Owning query/storage/LLM behavior merely because it is instrumented.
- Guaranteeing every telemetry event is exported or that telemetry is a transactionally complete audit record.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| API exporter | `opentelemetry_tracer.cpp` includes `api/otlp_exporter.h`; see [API architecture](../api/ARCHITECTURE.md). |
| Query profiling | Query module exposes a query-profiler path; see [query architecture](../query/ARCHITECTURE.md). |
| Security/redaction | `metrics_collector.cpp` includes the PII redaction policy; see [security architecture](../security/ARCHITECTURE.md). |
| Retrieval provenance | The provenance record names retrieval steps; this is a telemetry contract, not evidence that every retrieval path writes a record. |

## References

- [Module README](README.md) · [API contract](API.md) · [module roadmap](ROADMAP.md)
- [`observability_api_contract.h`](../../include/observability/observability_api_contract.h) · [`metrics_collector.h`](../../include/observability/metrics_collector.h) · [`opentelemetry_tracer.h`](../../include/observability/opentelemetry_tracer.h)
