# ThemisDB Observability Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The observability module provides monitoring, metrics, tracing, profiling, alerting, and operational diagnostics infrastructure for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| metrics_collector.cpp | metrics capture and export surfaces |
| alertmanager.cpp | alertmanager integration and routing |
| alerting_engine.cpp | rule-based alert evaluation and notifications |
| tracer.cpp | trace/span lifecycle and propagation support |
| opentelemetry_tracer.cpp | OpenTelemetry integration surfaces |
| continuous_profiler.cpp | continuous profiling behavior |
| query_profiler.cpp | query profiling instrumentation |
| storage_profiler.cpp | storage profiling instrumentation |
| performance_analyzer.cpp | performance diagnostics and recommendations |
| metrics_stream_server.cpp | live metric streaming behavior |
| metric_aggregator.cpp | metric aggregation/rollup semantics |
| advanced_metrics.cpp | advanced metric primitive support |
| metric_anomaly_detector.cpp | anomaly detection integration |
| ml_anomaly_detector.cpp | model-driven anomaly detection support |
| distributed_flame_graph.cpp | distributed flame graph processing |
| root_cause_analyzer.cpp | root-cause analysis logic |
| log_aggregator.cpp | structured log aggregation |
| log_search_engine.cpp | structured log search/filter behavior |
| tenant_metrics_namespace.cpp | tenant-scoped metrics namespacing |
| ebpf_tracer.cpp | eBPF-based observability probes |
| slo_reporter.cpp | SLO/SLA burn-rate reporting |

## Scope

In scope:
- metrics/tracing/profiling/alerting runtime behavior
- observability diagnostics and analysis surfaces
- metrics export and stream/aggregation behavior

Out of scope:
- external dashboard/alert backend ownership outside integration boundaries
- non-observability business-domain logic
- long-term storage ownership of external observability backends

## Runtime Behavior and Limits

- behavior depends on enabled observability components and runtime configuration.
- unsupported integration paths degrade deterministically with explicit outcomes.
- profiling/tracing/metrics behavior remains bounded by module-local controls.
- metrics collector input now fail-closes malformed label sets and emits explicit rejection
  diagnostics instead of creating unbounded or invalid series.
- tracing propagation is verified against real `OpenTelemetryTracer` behavior for
  upstream `traceparent`, outbound baggage injection, and exception-to-span
  failure diagnostics.
- exporter incidents are surfaced through failure/recovery counters plus
  `exporter_health_status{exporter=...}` for operator-facing status checks.

## Sourcecode Verification (Module: observability/readme)

- Verified files:
  - src/observability/metrics_collector.cpp
  - src/observability/alertmanager.cpp
  - src/observability/alerting_engine.cpp
  - src/observability/tracer.cpp
  - src/observability/opentelemetry_tracer.cpp
  - src/observability/continuous_profiler.cpp
  - src/observability/query_profiler.cpp
  - src/observability/storage_profiler.cpp
  - src/observability/performance_analyzer.cpp
  - src/observability/metrics_stream_server.cpp
  - src/observability/metric_aggregator.cpp
  - src/observability/advanced_metrics.cpp
  - src/observability/metric_anomaly_detector.cpp
  - src/observability/ml_anomaly_detector.cpp
  - src/observability/distributed_flame_graph.cpp
  - src/observability/root_cause_analyzer.cpp
  - src/observability/log_aggregator.cpp
  - src/observability/log_search_engine.cpp
  - src/observability/tenant_metrics_namespace.cpp
  - src/observability/ebpf_tracer.cpp
  - src/observability/slo_reporter.cpp
- Verified behavior surfaces:
  - metrics, tracing, profiling, alerting, and diagnostics paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md