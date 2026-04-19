> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Observability Module

## [Unreleased]
- Metrics endpoint authentication (`/metrics` behind auth layer)
- Trace span attribute PII scanning and sanitization
- Prometheus integration test suite
- Structured log search API

---

## [1.7.0] — SLO/SLA & Anomaly Detection
### Added
- SLO/SLA compliance reporting with burn-rate alerts (`slo_reporter.cpp`)
- ML-based anomaly detection for metric streams (`metric_anomaly_detector.cpp`)
- Distributed flame graph collection and rendering (`distributed_flame_graph.cpp`)
- Metric aggregation operators: rate, histogram, cardinality (`metric_aggregator.cpp`)

---

## [1.6.0] — eBPF Kernel Tracing
### Added
- eBPF-based kernel tracing for low-overhead system-level profiling (`ebpf_tracer.cpp`)
- PMU hardware performance counter integration (exposed via metrics pipeline)

---

## [1.5.0] — Custom Alert Rules & Alerting Engine
### Added
- Custom alert rules API via `AlertRuleManager`
- `AlertingEngine` with predefined rules: CPU, memory, latency, error-rate, disk (`alerting_engine.cpp`)
- Exemplar support on histogram metrics for trace linkage
- OTLP gRPC and HTTP export endpoints

---

## [1.4.0] — Continuous Profiling & Adaptive Sampling
### Added
- Continuous profiling pipeline (`continuous_profiler.cpp`)
- Adaptive sampling — dynamic trace sampling rate based on error rate and latency
- Grafana dashboard integration (pre-built dashboard JSON provisioning)

---

## [1.3.0] — Distributed Tracing & Structured Logging
### Added
- Distributed tracing with OpenTelemetry-compatible spans (`tracer.cpp`)
- W3C Trace Context propagation (`traceparent`/`tracestate` headers)
- Structured logging via `LogAggregator` — JSON output with correlation IDs (`log_aggregator.cpp`)
- Alertmanager integration for routing alerts to external receivers (`alertmanager.cpp`)

---

## [1.2.0] — Profilers & Performance Analysis
### Added
- `QueryProfiler` — per-phase and per-operator query timing (`query_profiler.cpp`)
- `StorageProfiler` — RocksDB statistics collection and exposure (`storage_profiler.cpp`)
- `PerformanceAnalyzer` — automated performance recommendations from profiler data (`performance_analyzer.cpp`)

---

## [1.1.0] — Kubernetes & Health Probes
### Added
- Kubernetes liveness, readiness, and startup probe endpoints
- Grafana-compatible label conventions on all exported metrics

---

## [1.0.0] — Metrics Core
### Added
- `MetricsCollector` — Prometheus text-format `/metrics` endpoint (`metrics_collector.cpp`)
- Counter, gauge, histogram, and summary metric types
- Per-metric cardinality limits to prevent label explosion
