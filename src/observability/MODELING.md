---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Observability Modeling Notes

## Purpose

This document describes the runtime model of the observability module as implemented in ThemisDB. It is intentionally source-grounded and should be read alongside `ARCHITECTURE.md`, the API headers, and the concrete implementation files in `src/observability/`.

## System Model

The observability module models four operational streams that share a common reliability contract:

1. Metrics stream
   - recorded through `MetricsCollector`
   - aggregated, labeled, and exported for Prometheus/OpenTelemetry consumers
   - bounded by cardinality, label size, and exporter failure signals

2. Trace stream
   - created and propagated by `OpenTelemetryTracer` and the underlying tracer abstraction
   - carries W3C context and baggage across request boundaries
   - retained in a bounded ring buffer and dropped only under explicit overload conditions

3. Profiling stream
   - created by `ContinuousProfiler`, `QueryProfiler`, and storage/profiling helpers
   - used for runtime diagnostics and hot-path analysis
   - integrated with span lifecycle in `tracer.cpp` where profiling snapshots attach to span records

4. Alerting and diagnosis stream
   - driven by `AlertingEngine`, `Alertmanager`, and `RootCauseAnalyzer`
   - evaluates policies and converts abnormal conditions into actionable operator outputs

## Core Invariants

These invariants are repeated in the source and are the expected behavior for future changes:

- metric labels remain bounded and validated before admission
- histograms use immutable bucket boundaries and fail explicitly on invalid ordering
- counter semantics remain monotonically non-decreasing within the supported contract
- trace context remains causal and bounded; invalid or over-deep nesting is rejected or dropped with explicit signaling
- exporter failures never block the main metric/trace pipeline; they are surfaced as health signals
- SLO windows close deterministically and reject late updates after the grace period

## Data Flow

### Metrics path

`MetricsCollector` works as the central aggregator for metric families and exporter health tracking. It records counters, gauges, and histograms and emits rejection counters for malformed label sets and excess cardinality. This logic sits in `src/observability/metrics_collector.cpp` and is backed by the contract in `include/observability/observability_api_contract.h`.

### Trace path

`OpenTelemetryTracer` builds a span hierarchy using trace and span IDs, records span metadata, and injects/extracts W3C `traceparent` and baggage headers. The implementation in `src/observability/opentelemetry_tracer.cpp` preserves the trace context contract while still allowing bounded retention and explicit sampling decisions.

### Profiling path

`QueryProfiler`, `StorageProfiler`, and `ContinuousProfiler` produce runtime diagnostics that can be retained with span context or surfaced through query/operator analysis. Profiling remains a best-effort diagnostic capability: when the profiler is unavailable or a snapshot fails, the runtime must continue without blocking the primary execution path.

### Alert path

`AlertingEngine` evaluates configured conditions against current telemetry and creates an `Alert` object. `Alertmanager` then routes the alert to its configured channels (including webhook/logging backends), while failure states remain explicit and observable.

## Failure Domains

The module is designed around a few distinct failure domains:

- malformed telemetry input
- exporter outage or misconfiguration
- cardinality blowups
- invalid or stale window data
- orphaned or over-nested trace context
- operator remediation rules that cannot be resolved or deduplicated

The error taxonomy in `observability_api_contract.h` is the authoritative inventory for these failures. The code paths in `metrics_collector.cpp`, `tracer.cpp`, and `alerting_engine.cpp` map their diagnostics into this taxonomy rather than inventing separate ad hoc failure models.

## Relationship to Other Modules

Observability is intentionally cross-cutting:

- storage/query systems emit telemetry through the metrics collector,
- request execution emits trace context that flows through the tracer,
- operational and security systems consume alerting and RCA outputs,
- external integration backends receive exported telemetry but remain outside the module's primary ownership.

This means the module's contracts are about explicit responsibility boundaries, not complete end-to-end ownership of the full backend ecosystem.

## Validation Expectations

Changes to this module should preserve the following behavior:

- metric labels remain bounded and visible to the operator,
- trace propagation remains causal and not silently broken,
- alert objects remain deterministic for the same rule inputs,
- exporter failures remain visible as counters and health signals,
- malformed inputs do not produce silent data corruption.

## References

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [RESEARCH.md](RESEARCH.md)
- [IMPLEMENTATION.md](IMPLEMENTATION.md)
- [API.md](API.md)
- [SECURITY.md](SECURITY.md)
- [include/observability/observability_api_contract.h](../../include/observability/observability_api_contract.h)
