# Observability Module Roadmap

<!-- Status: current | validated: 2026-03-09 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · docs/de/observability/README.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Enterprise-grade observability stack. Prometheus metrics, query profiling, storage profiling, automated performance analysis, Alertmanager integration, and distributed tracing are all implemented.

## Completed ✅
- [x] MetricsCollector singleton with Prometheus text-format export (`/metrics`)
- [x] QueryProfiler – per-phase and per-operator timing with index usage tracking
- [x] StorageProfiler – RocksDB stats, write/read amplification, cache hit rates
- [x] PerformanceAnalyzer – automated issue detection with optimization recommendations
- [x] Alertmanager integration (alert routing and notifications)
- [x] Distributed tracing with span context propagation (OpenTelemetry-compatible)
- [x] Structured logging via Core ILogger interface
- [x] Kubernetes readiness and liveness health probes
- [x] Telemetry aggregation across shards
- [x] Grafana dashboard integration
- [x] PagerDuty/Slack notification routing
- [x] Continuous profiling integration (pprof / async-profiler compatible) (Issue: #2418)
- [x] Adaptive sampling rate for high-frequency spans (Issue: #1963)

## In Progress 🚧
- [?] OpenTelemetry SDK direct export (OTLP gRPC/HTTP) (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Exemplars on Prometheus histograms (link traces to metrics) (Issue: #1995)
  - Files: `observability/metrics_collector.h`, `observability/metrics_collector.cpp`
  - Implementation: `Exemplar` struct + `observeHistogramWithExemplar()` + inline p99 exemplar export
  - Tests: `tests/test_metrics_exemplar.cpp`
- [x] Custom user-defined alert rules via API (Issue: #2025)
  - Files: `observability/alertmanager.h`, `observability/alertmanager.cpp`
  - Implementation: `AlertRule`, `AlertRuleOperator`, `AlertRuleManager` (CRUD + `evaluateRules()`)
  - Tests: `tests/test_alert_rules.cpp`
- [?] Per-tenant metric namespacing
- [?] Structured log search API (query logs like data)
- [?] Real-time query cost estimator dashboard

### Long-term (6-12 months)
- [x] eBPF-based low-overhead kernel-level tracing (Issue: #2055) — ✅ implemented; `ebpf_tracer.cpp` (0 stubs, unit tests in `tests/test_ebpf_tracer.cpp`)
  - Files: `observability/ebpf_tracer.h`, `observability/ebpf_tracer.cpp`
  - Subsystems: MetricsCollector (gauge export), background sampling thread
  - Behaviour: polls `perf_event_open(2)` software counters at configurable interval (default 1 s); publishes `themis_ebpf_{context_switches,page_faults,cpu_migrations,task_clock_ns,collection_cycles}_total` gauges; optional libbpf BPF program attach via `THEMIS_ENABLE_EBPF`
  - Error handling: graceful no-op on non-Linux; fails-open when perf fd open fails (continues without that probe)
  - Tests: unit tests in `tests/test_ebpf_tracer.cpp` (lifecycle, config, stats, ring-buffer, callback, metrics)
  - Perf target: < 0.1 % CPU overhead per probe type at 1-second interval
- [x] Anomaly detection on metrics time-series (ML-based) (Issue: #2097)
  - Files: `observability/metric_anomaly_detector.h`, `observability/metric_anomaly_detector.cpp`
  - Implementation: `MetricAnomalyDetector` (monitor/observe/getAnomalies/publishMetrics/generateReport), bridges `analytics::StreamingAnomalyDetector` (Z_SCORE, MODIFIED_Z_SCORE, IQR, ISOLATION_FOREST, LOF, ENSEMBLE) with `MetricsCollector` gauges; `AnomalyCallback` hook; JSON & text reports
  - Tests: `tests/test_metric_anomaly_detector.cpp`
- [x] Distributed flame graph generation across nodes (Issue: #2108)
  - Files: `observability/distributed_flame_graph.h`, `observability/distributed_flame_graph.cpp`
  - Implementation: `DistributedFlameGraph` (add/merge/diff node profiles, normalise-per-node, JSON/folded export)
  - Tests: `tests/test_distributed_flame_graph.cpp`
- [?] Metrics federation across multiple ThemisDB clusters
- [x] SLO/SLA compliance reporting with burn-rate alerts (Issue: #2148)
  - Files: `observability/slo_reporter.h`, `observability/slo_reporter.cpp`
  - Implementation: `SloReporter` with `SloDefinition`, multi-window burn-rate detection (FAST 14.4×/MEDIUM 6×/SLOW 3×), `publishMetrics()`, `generateReport()`, `generateReportJson()`
  - Tests: `tests/test_slo_reporter.cpp`

## Implementation Phases

### Phase 1: Enterprise Observability Stack (Status: Completed ✅)
- [x] MetricsCollector singleton with Prometheus text-format export at `/metrics` (`observability/metrics_collector.cpp`)
- [x] QueryProfiler: per-phase and per-operator timing with index usage tracking (`observability/query_profiler.cpp`)
- [x] StorageProfiler: RocksDB stats, write/read amplification, cache hit rates
- [x] PerformanceAnalyzer: automated issue detection with optimization recommendations
- [x] Alertmanager integration (alert routing and notifications)
- [x] Distributed tracing with span context propagation (OpenTelemetry-compatible, `src/observability/continuous_profiler.cpp`)
- [x] Structured logging via Core ILogger interface
- [x] Kubernetes readiness and liveness health probes
- [x] Telemetry aggregation across shards
- [x] Grafana dashboard integration and PagerDuty/Slack notification routing

### Phase 2: Native OTLP Export & Continuous Profiling (Status: In Progress 🚧)
- [?] OpenTelemetry SDK direct export via OTLP gRPC/HTTP (`observability/otlp_exporter.cpp`, Target: Q2 2026)
- [x] Continuous profiling integration (pprof / async-profiler compatible) (Target: Q2 2026)
- [x] Adaptive sampling rate for high-frequency spans (Target: Q3 2026)

### Phase 3: ML-Augmented & Distributed Observability (Status: In Progress 🚧)
- [x] Exemplars on Prometheus histograms (link traces to metrics)
  — `observability/metrics_collector.h/cpp`, tests: `tests/test_metrics_exemplar.cpp`
- [x] Custom user-defined alert rules via API
  — `observability/alertmanager.h/cpp`, tests: `tests/test_alert_rules.cpp`
- [x] eBPF-based low-overhead kernel-level tracing
  — `observability/ebpf_tracer.h/cpp`, tests: `tests/test_ebpf_tracer.cpp`
- [x] Anomaly detection on metrics time-series (ML-based)
  — `observability/metric_anomaly_detector.h/cpp`, tests: `tests/test_metric_anomaly_detector.cpp`
- [x] Distributed flame graph generation across nodes
  — `observability/distributed_flame_graph.h/cpp`, tests: `tests/test_distributed_flame_graph.cpp`
- [x] SLO/SLA compliance reporting with burn-rate alerts
  — `observability/slo_reporter.h/cpp`, tests: `tests/test_slo_reporter.cpp`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — `test_observability_profilers.cpp` (280 LOC), `test_observability_hardening.cpp`; focused targets: `ObservabilityProfilersFocusedTests`, `ObservabilityHardeningFocusedTests`
- [?] Integration tests (Prometheus scrape, Grafana dashboard rendering)
- [x] Performance benchmarks (metrics overhead < 1% CPU) — `benchmarks/bench_metrics_collector.cpp` registered (2026-03-10)
- [?] Security audit (metrics endpoint authentication, trace data PII)
- [x] Documentation complete — README.md, ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, docs/de/observability/ all present
- [?] API stability guaranteed

## Known Issues & Limitations
- OTLP export is not yet implemented (`otlp_exporter.cpp` does not exist); traces use internal span propagation only.
- Telemetry aggregation across shards is eventually consistent.
- `query_profiler.cpp`, `storage_profiler.cpp`, and `performance_analyzer.cpp` were missing from `cmake/CMakeLists.txt` — fixed 2026-03-09; `test_observability_profilers.cpp` would fail to link without this fix.
- `tracer.cpp` and `log_aggregator.cpp` are referenced in older docs but do not exist; distributed tracing is in `continuous_profiler.cpp`; logging is via Core `ILogger`; see `docs/de/observability/missing-implementations.md`.

## Breaking Changes
- Prometheus metric names follow `themis_*` namespace; stable from v1.x.
- Span context format may change to full W3C Trace Context standard in v2.0.
