<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Observability Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/observability/CHANGELOG.md`.

## [1.7.0] — 2026-01

### Added
- `slo_reporter.h` — `SloReporter` for error-budget burn-rate reporting and multi-window SLO tracking.
- `metric_anomaly_detector.h` — `MetricAnomalyDetector` statistical change-point detection.
- `distributed_flame_graph.h` — `DistributedFlameGraph` cross-service flame graph aggregation.
- `metric_aggregator.h` — `MetricAggregator` time-window rollup and downsampling.

## [1.6.0] — 2025-10

### Added
- `ebpf_tracer.h` — `EbpfTracer` for kernel scheduler, network, and disk I/O tracing via eBPF.
- PMU hardware performance counter support in `performance_analyzer.h`.

## [1.5.0] — 2025-07

### Added
- `alerting_engine.h` — `AlertingEngine` with multi-window error budget evaluation.
- `alertmanager.h` — `AlertmanagerClient` Prometheus Alertmanager webhook integration.
- `opentelemetry_tracer.h` — `OpenTelemetryTracer` OTLP gRPC and HTTP span export.
- `ml_anomaly_detector.h` — `MlAnomalyDetector` (Isolation Forest / LSTM).

## [1.4.0] — 2025-04

### Added
- `continuous_profiler.h` — `ContinuousProfiler` with adaptive head+tail sampling.
- `root_cause_analyzer.h` — `RootCauseAnalyzer` automated signal correlation.
- `metrics_stream_server.h` — SSE/gRPC live metric streaming endpoint.

## [1.0.0] — 2025-01

### Added
- `metrics_collector.h` — Core Prometheus-compatible counter/gauge/histogram registry.
- `tracer.h` — Lightweight internal span tracer.
- `log_aggregator.h` — Structured log aggregation.
- `query_profiler.h` — Per-query execution plan profiling.
- `storage_profiler.h` — LSM/RocksDB I/O profiling.
- `advanced_metrics.h` — Extended histogram and summary metrics.
- `performance_analyzer.h` — Cross-signal latency correlation.
