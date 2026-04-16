<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Observability Module Roadmap

## Current Status

v1.7.0 — production. Full OTLP tracing, Prometheus metrics, eBPF tracing, SLO reporting, and ML anomaly detection are operational.

## Completed

- [x] Prometheus-compatible metrics registry (`MetricsCollector`)
- [x] Lightweight internal tracer (`Tracer`)
- [x] Structured log aggregation with Loki sink
- [x] Per-query and storage profiling
- [x] OTLP gRPC/HTTP span export (`OpenTelemetryTracer`)
- [x] Alerting engine with multi-window burn-rate (`AlertingEngine`)
- [x] Alertmanager webhook client
- [x] Continuous profiling with adaptive sampling
- [x] eBPF kernel-level tracing
- [x] ML anomaly detection (Isolation Forest / LSTM)
- [x] SLO reporter + error-budget burn-rate
- [x] Distributed flame graph aggregation
- [x] MetricAggregator time-window rollup

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] Telemetry signal taxonomy (traces, metrics, logs, profiles)
- [x] OTLP proto schema alignment

### Phase 2 — Core Implementation ✅
- [x] `MetricsCollector` Prometheus registry
- [x] `OpenTelemetryTracer` OTLP export
- [x] `LogAggregator` structured sink

### Phase 3 — Advanced Signals ✅
- [x] `ContinuousProfiler` adaptive sampling
- [x] `EbpfTracer` BTF-based kernel probes
- [x] `QueryProfiler` + `StorageProfiler`

### Phase 4 — Intelligence Layer ✅
- [x] `MlAnomalyDetector` Isolation Forest / LSTM
- [x] `MetricAnomalyDetector` statistical change-point
- [x] `RootCauseAnalyzer` signal correlation

### Phase 5 — SLO & Alerting ✅
- [x] `SloReporter` error-budget tracking
- [x] `AlertingEngine` multi-window rules
- [x] `AlertmanagerClient` routing

### Phase 6 — Future Enhancements (Planned)
- [x] Exemplar support in OTLP metrics export (Target: Q3 2026)
- [ ] Continuous profiling for GPU workloads (Target: Q4 2026)
- [ ] Adaptive log sampling based on SLO burn rate (Target: Q4 2026)

## Production Readiness Checklist

- [x] OTLP export validated against Grafana Tempo and Jaeger
- [x] eBPF tracer tested on Linux 5.15+ with BTF
- [x] SLO reporter validated against Google SRE burn-rate model
- [ ] GPU profiling support (Target: Q4 2026)
