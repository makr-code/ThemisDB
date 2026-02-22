# Observability Module Roadmap

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

## In Progress 🚧
- [?] OpenTelemetry SDK direct export (OTLP gRPC/HTTP) (Target: Q2 2026)
- [I] Adaptive sampling rate for high-frequency spans (Target: Q3 2026) (Issue: #1963)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Exemplars on Prometheus histograms (link traces to metrics) (Issue: #1995)
- [I] Custom user-defined alert rules via API (Issue: #2025)
- [?] Per-tenant metric namespacing
- [?] Structured log search API (query logs like data)
- [?] Real-time query cost estimator dashboard

### Long-term (6-12 months)
- [I] eBPF-based low-overhead kernel-level tracing (Issue: #2055)
- [I] Anomaly detection on metrics time-series (ML-based) (Issue: #2097)
- [I] Distributed flame graph generation across nodes (Issue: #2108)
- [?] Metrics federation across multiple ThemisDB clusters
- [I] SLO/SLA compliance reporting with burn-rate alerts (Issue: #2148)

## Implementation Phases

### Phase 1: Enterprise Observability Stack (Status: Completed ✅)
- [x] MetricsCollector singleton with Prometheus text-format export at `/metrics` (`observability/metrics_collector.cpp`)
- [x] QueryProfiler: per-phase and per-operator timing with index usage tracking (`observability/query_profiler.cpp`)
- [x] StorageProfiler: RocksDB stats, write/read amplification, cache hit rates
- [x] PerformanceAnalyzer: automated issue detection with optimization recommendations
- [x] Alertmanager integration (alert routing and notifications)
- [x] Distributed tracing with span context propagation (OpenTelemetry-compatible, `observability/tracer.cpp`)
- [x] Structured logging via Core ILogger interface
- [x] Kubernetes readiness and liveness health probes
- [x] Telemetry aggregation across shards
- [x] Grafana dashboard integration and PagerDuty/Slack notification routing

### Phase 2: Native OTLP Export & Continuous Profiling (Status: In Progress 🚧)
- [?] OpenTelemetry SDK direct export via OTLP gRPC/HTTP (`observability/otlp_exporter.cpp`, Target: Q2 2026)
- [x] Continuous profiling integration (pprof / async-profiler compatible) (Target: Q2 2026)
- [ ] Adaptive sampling rate for high-frequency spans (Target: Q3 2026)

### Phase 3: ML-Augmented & Distributed Observability (Status: Planned 📋)
- [ ] Exemplars on Prometheus histograms (link traces to metrics)
- [ ] Custom user-defined alert rules via API
- [ ] eBPF-based low-overhead kernel-level tracing
- [ ] Anomaly detection on metrics time-series (ML-based)
- [ ] Distributed flame graph generation across nodes
- [ ] SLO/SLA compliance reporting with burn-rate alerts

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (Prometheus scrape, Grafana dashboard rendering)
- [?] Performance benchmarks (metrics overhead < 1% CPU)
- [?] Security audit (metrics endpoint authentication, trace data PII)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- OTLP export is not yet implemented; traces use internal span propagation only.
- Adaptive sampling is not configurable at runtime; requires restart.
- Telemetry aggregation across shards is eventually consistent.

## Breaking Changes
- Prometheus metric names follow `themis_*` namespace; stable from v1.x.
- Span context format may change to full W3C Trace Context standard in v2.0.
