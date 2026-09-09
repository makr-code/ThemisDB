# Architecture - Observability Module

<!-- Status: current | validated: 2026-09-09 -->
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

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` (time, string, concurrency helpers) | Timestamp normalisation, label formatting, and thread-safe counter primitives |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/observability/metrics_collector.h`, `include/observability/tracer.h` | Per-request latency, error counters, and distributed trace span creation |
| query | `include/observability/query_profiler.h`, `include/observability/tracer.h` | Query execution profiling and span propagation through the query pipeline |
| transaction | `include/observability/metrics_collector.h`, `include/observability/tracer.h` | Transaction commit/abort counters and MVCC span telemetry |
| storage | `include/observability/storage_profiler.h`, `include/observability/metrics_collector.h` | I/O latency profiles and compaction metric emission |
| llm | `include/observability/metrics_collector.h`, `include/observability/tracer.h` | Inference latency histograms and model-call span creation |
| rag | `include/observability/metrics_collector.h`, `include/observability/tracer.h` | Retrieval pipeline metrics and span propagation |
| replication | `include/observability/metrics_collector.h` | Replication lag counters and heartbeat metrics |
| maintenance | `include/observability/metrics_collector.h`, `include/observability/audit_logger.h` | Job execution counters and maintenance-event audit emission |

## Integration Points

### Critical Integration: MetricsCollector ↔ All Modules
**Files:** `include/observability/metrics_collector.h` ↔ all consuming `src/<module>/` cpp files
**Contract:** `MetricsCollector::record(metric_name, value, labels)` is a non-throwing fire-and-forget call; it must never block the caller's critical path. Internal buffer overflow silently drops samples and increments a dedicated `metrics.dropped` counter.
**Thread Safety:** `MetricsCollector` is fully thread-safe; concurrent `record()` calls from any thread are permitted.
**Failure Mode:** Export failures (Prometheus/OTLP endpoint unavailable) are surfaced in the `metrics.export_errors` counter; in-process recording continues unaffected.

### Critical Integration: QueryProfiler ↔ Query Execution
**Files:** `include/observability/query_profiler.h` ↔ `src/query/` execution pipeline
**Contract:** A `ProfilerScope` RAII guard is acquired at query parse time and released at result flush; timings are attributed per execution stage (parse, plan, execute, encode).
**Thread Safety:** Each query context owns its own `ProfilerScope`; the underlying `QueryProfiler` aggregator uses internal locking for cross-thread rollup.
**Failure Mode:** If the profiler is disabled or encounters an internal error the RAII guard is a no-op; query execution is never gated on profiler availability.

### Critical Integration: ProvenanceStore ↔ Audit Consumers
**Files:** `include/observability/audit_logger.h` ↔ `src/auth/`, `src/security/`, `src/server/`
**Contract:** `AuditLogger::emit(event)` serialises a structured audit record to the `ProvenanceStore`; the call is synchronous to provide write-before-response ordering for security-sensitive events.
**Thread Safety:** `AuditLogger` is thread-safe; multiple modules may call `emit()` concurrently.
**Failure Mode:** `ProvenanceStore` write failure propagates as `AuditWriteError`; the caller decides whether to abort the triggering operation (security paths abort; metrics paths continue).



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