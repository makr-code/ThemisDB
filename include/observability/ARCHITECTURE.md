<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Observability Module — Architecture Guide

## Overview

The observability module provides end-to-end telemetry for ThemisDB: distributed tracing (OpenTelemetry), structured metrics (Prometheus/OTLP), continuous CPU profiling, eBPF kernel-level tracing, alerting, SLO reporting, and ML-based anomaly detection. It is the primary instrumentation layer consumed by Grafana dashboards and PagerDuty-style alert pipelines.

## Design Principles

- **OpenTelemetry-native** — all traces and metrics use OTLP gRPC/HTTP export paths.
- **eBPF for hot paths** — `ebpf_tracer.h` instruments kernel scheduler, network, and disk I/O without application changes.
- **Adaptive sampling** — `continuous_profiler.h` uses head-based sampling with tail-based upgrade for slow/error spans.
- **SLO-first alerting** — `slo_reporter.h` drives burn-rate alerts; `alerting_engine.h` evaluates multi-window error budgets.
- **ML anomaly detection** — `ml_anomaly_detector.h` and `metric_anomaly_detector.h` provide online change-point detection.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `advanced_metrics.h` | `AdvancedMetrics` | Extended histogram and summary metrics beyond standard counters |
| `alerting_engine.h` | `AlertingEngine` | Multi-window SLO burn-rate rule evaluation |
| `alertmanager.h` | `AlertmanagerClient` | Prometheus Alertmanager webhook integration |
| `continuous_profiler.h` | `ContinuousProfiler` | CPU/heap continuous profiling with adaptive sampling |
| `distributed_flame_graph.h` | `DistributedFlameGraph` | Distributed-trace-aware flame graph aggregation |
| `ebpf_tracer.h` | `EbpfTracer` | eBPF programs for scheduler/network/disk I/O tracing |
| `log_aggregator.h` | `LogAggregator` | Structured log aggregation with Loki/OTLP sink |
| `metric_aggregator.h` | `MetricAggregator` | Time-window metric rollup and downsampling |
| `metric_anomaly_detector.h` | `MetricAnomalyDetector` | Statistical change-point detection on metric streams |
| `metrics_collector.h` | `MetricsCollector` | Core Prometheus-compatible counter/gauge/histogram registry |
| `metrics_stream_server.h` | `MetricsStreamServer` | SSE/gRPC streaming endpoint for live metric export |
| `ml_anomaly_detector.h` | `MlAnomalyDetector` | ML-based anomaly detection (Isolation Forest / LSTM) |
| `opentelemetry_tracer.h` | `OpenTelemetryTracer` | OTLP gRPC and HTTP span exporter |
| `performance_analyzer.h` | `PerformanceAnalyzer` | Cross-signal latency correlation and bottleneck identification |
| `query_profiler.h` | `QueryProfiler` | Per-query execution plan profiling |
| `root_cause_analyzer.h` | `RootCauseAnalyzer` | Automated root-cause suggestion from correlated signals |
| `slo_reporter.h` | `SloReporter` | SLO/SLA error-budget burn-rate reporting |
| `storage_profiler.h` | `StorageProfiler` | LSM/RocksDB I/O profiling and stall detection |
| `tracer.h` | `Tracer` | Lightweight internal span tracer (pre-OTLP shim) |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `performance` | `EbpfTracer`, `ContinuousProfiler` | PMU hardware counters and profiling |
| `query` | `QueryProfiler` | Per-query execution plan instrumentation |
| `storage` | `StorageProfiler` | RocksDB stall and compaction metrics |
| `network` | `MetricsCollector` | Per-connection latency histograms |
| `scheduler` | `AlertingEngine` | Task anomaly and SLO alerts |
| Grafana / Prometheus | `MetricsStreamServer`, `AlertmanagerClient` | External dashboard and alert routing |

## Implementation

Implementation in `../../src/observability/`.
