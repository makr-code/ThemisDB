# Architecture - Observability Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The observability module composes metrics collection, tracing, profiling, streaming, anomaly detection, and alerting into a bounded operational visibility subsystem for ThemisDB.

## Main Execution Planes

1. Metrics and export plane
- metric recording, aggregation, export, and streaming paths
- tenant-aware namespace and cardinality control behavior

2. Tracing and profiling plane
- span lifecycle and OpenTelemetry integration
- query/storage/continuous and eBPF profiling surfaces

3. Alerting and diagnostics plane
- rule-based alerting, anomaly detection, and root-cause analysis
- SLO/SLA burn-rate reporting and operational diagnostics

## Core Contracts

| Contract | Behavior |
|---|---|
| metrics contract | deterministic metric record/export/aggregation semantics |
| tracing contract | explicit span lifecycle and propagation behavior |
| profiling contract | bounded profiler data capture and analysis behavior |
| alerting contract | deterministic rule evaluation and notification surfaces |

## Failure Semantics

- invalid observability input/configuration fails with explicit outcomes.
- export/integration failures are surfaced explicitly.
- profiling and alerting incidents remain observable and non-silent.

## Sourcecode Verification (Module: observability/architecture)

- Verified files:
  - src/observability/metrics_collector.cpp
  - src/observability/tracer.cpp
  - src/observability/opentelemetry_tracer.cpp
  - src/observability/query_profiler.cpp
  - src/observability/storage_profiler.cpp
  - src/observability/alerting_engine.cpp
  - src/observability/metric_aggregator.cpp
  - src/observability/root_cause_analyzer.cpp
- Verified architecture claims:
  - explicit metrics/tracing/profiling/alerting execution planes
  - deterministic failure boundaries across observability workflows
  - module-local ownership of observability orchestration behavior