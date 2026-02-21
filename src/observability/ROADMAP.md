# Observability Module Roadmap

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

## In Progress 🚧
- [ ] OpenTelemetry SDK direct export (OTLP gRPC/HTTP) (Target: Q2 2026)
- [ ] Continuous profiling integration (pprof / async-profiler compatible) (Target: Q2 2026)
- [ ] Adaptive sampling rate for high-frequency spans (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Exemplars on Prometheus histograms (link traces to metrics)
- [ ] Custom user-defined alert rules via API
- [ ] Per-tenant metric namespacing
- [ ] Structured log search API (query logs like data)
- [ ] Real-time query cost estimator dashboard

### Long-term (6-12 months)
- [ ] eBPF-based low-overhead kernel-level tracing
- [ ] Anomaly detection on metrics time-series (ML-based)
- [ ] Distributed flame graph generation across nodes
- [ ] Metrics federation across multiple ThemisDB clusters
- [ ] SLO/SLA compliance reporting with burn-rate alerts

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (Prometheus scrape, Grafana dashboard rendering)
- [ ] Performance benchmarks (metrics overhead < 1% CPU)
- [ ] Security audit (metrics endpoint authentication, trace data PII)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- OTLP export is not yet implemented; traces use internal span propagation only.
- Adaptive sampling is not configurable at runtime; requires restart.
- Telemetry aggregation across shards is eventually consistent.

## Breaking Changes
- Prometheus metric names follow `themis_*` namespace; stable from v1.x.
- Span context format may change to full W3C Trace Context standard in v2.0.
