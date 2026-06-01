> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/observability/ARCHITECTURE.md -->

# Observability Module — Public Header Architecture

**Module Path:** `include/observability/`
**Implementation:** `../../src/observability/`
**Canonical architecture doc:** [`../../src/observability/ARCHITECTURE.md`](../../src/observability/ARCHITECTURE.md)

---

## 1. Overview

`include/observability/` defines the **public telemetry, tracing, profiling, and diagnostics contract** for ThemisDB. The 22 headers cover metric collection and aggregation, tracing/OpenTelemetry, profiling, alerting and anomaly detection, logs, flame graphs, root-cause analysis, and SLO reporting.

For runtime composition details — telemetry pipelines, exporter behavior, and alert/diagnostic internals — see:
→ [`../../src/observability/ARCHITECTURE.md`](../../src/observability/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Metrics and Export

| Header | Public Type | Purpose |
|--------|------------|---------|
| `metrics_collector.h`, `advanced_metrics.h` | Metrics collectors/types | Metric capture primitives |
| `metric_aggregator.h`, `metrics_stream_server.h` | Aggregation and streaming types | Metric rollup and live export |
| `tenant_metrics_namespace.h`, `otlp_exemplar.h` | Namespacing and exemplar types | Tenant scoping and OTLP detail |

### 2.2 Tracing and Profiling

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tracer.h`, `opentelemetry_tracer.h` | Tracing types | Span lifecycle and OTel integration |
| `continuous_profiler.h`, `query_profiler.h`, `storage_profiler.h` | Profiling types | Runtime profiling surfaces |
| `ebpf_tracer.h`, `distributed_flame_graph.h` | eBPF and flame-graph types | Deep diagnostics and distributed stacks |
| `performance_analyzer.h` | `PerformanceAnalyzer` | Profiling-driven diagnostics |

### 2.3 Alerting and Anomaly Detection

| Header | Public Type | Purpose |
|--------|------------|---------|
| `alerting_engine.h`, `alertmanager.h` | Alerting types | Rule evaluation and notification routing |
| `metric_anomaly_detector.h`, `ml_anomaly_detector.h` | Anomaly-detection types | Statistical and ML-based anomaly detection |
| `slo_reporter.h` | `SLOReporter` | SLO/SLA reporting |

### 2.4 Logs and Operational Diagnostics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `log_aggregator.h`, `log_search_engine.h` | Log pipeline types | Structured log collection and query |
| `root_cause_analyzer.h` | `RootCauseAnalyzer` | Cross-signal incident analysis |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::observability` | Telemetry, tracing, and diagnostics types |

---

## 4. Public Contract Notes

- Metrics and tracing headers remain public because multiple runtime modules emit telemetry without depending on observability implementation internals.
- Profiling and flame-graph contracts expose explicit capture results for operational tooling and embedders.
- Alerting and anomaly detection remain public so deployments can wire custom policies and notification backends.
- Log and RCA headers provide a stable diagnostic envelope across server, storage, query, and sharding modules.
