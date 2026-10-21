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

# Observability Research Notes

## Purpose

This document captures the design assumptions behind ThemisDB's observability stack and explains why the module is structured around explicit contracts, bounded cardinality, and fail-closed monitoring behavior.

The primary source material is the live code under `src/observability/`, the public API headers in `include/observability/`, and the normative contract in `include/observability/observability_api_contract.h`.

## Problem Statement

Observability is not a passive logging concern in ThemisDB. It is the operational control plane for production correctness: metrics reveal throughput, tracing explains request latency and failure paths, profiling identifies hotspots, and alerting converts abnormal conditions into actionable operator signals.

The codebase treats the module as a reliability primitive rather than a dashboard-only add-on. That is visible in the hardening work around metrics cardinality, malformed telemetry rejection, and exporter health tracking in `MetricsCollector`, `OpenTelemetryTracer`, and `AlertingEngine`.

## Research Basis

The following source files define the module's assumptions:

- `include/observability/observability_api_contract.h` — frozen limits and error taxonomy
- `include/observability/metrics_collector.h` — metrics API and exporter-health surfaces
- `include/observability/opentelemetry_tracer.h` — tracing/propagation contract
- `src/observability/metrics_collector.cpp` — bounded ingest and malformed-telemetry rejection
- `src/observability/tracer.cpp` — span retention, profiling integration, and sampling behavior
- `src/observability/alerting_engine.cpp` — deterministic alert generation and external delivery paths

## Key Assumptions

1. Telemetry must remain bounded.
   - `kMaxMetricLabels`, `kMaxLabelKeyBytes`, `kMaxLabelValueBytes`, and `kMaxMetricCardinality` are explicit contract limits.
   - The code rejects malformed or oversize labels instead of silently accepting them.

2. Failure must be explicit.
   - The observability contract includes `EXPORTER_UNAVAILABLE`, `TRACE_SPAN_DROPPED`, `SLO_WINDOW_INVALID`, and cardinality-related errors.
   - Exporter failures and malformed telemetry are counted and exposed through diagnostic surfaces, not hidden.

3. Tracing and metrics must preserve causal meaning.
   - W3C trace context propagation is a first-class API feature in `OpenTelemetryTracer`.
   - Span nesting depth and baggage size are bounded to prevent corrupted or unbounded causal structure.

4. SLO evaluation must be deterministic.
   - SLO windows are closed only at configured boundaries.
   - Late writes are rejected after the grace window using the contract's error set.

## Evaluation Model

The repository's observability implementation is evaluated in three ways:

- correctness under malformed input and concurrent load,
- bounded behavior under high cardinality and exporter outage,
- operator clarity through incident counters and alerting signals.

The module's test and benchmark files validate these dimensions through focused coverage and release-gate style checks. Example evidence includes:

- `tests/observability/test_observability_contract_hardening_focused.cpp`
- `tests/observability/test_observability_block_b_focused.cpp`
- `benchmarks/observability/bench_observability_release_gates.cpp`
- `benchmarks/observability/bench_observability_block_b_gates.cpp`

## Trade-offs and Limitations

- Bounded storage and cardinality are prioritized over unrestricted metric growth. This is intentional: unbounded telemetry is operationally dangerous and creates false confidence.
- Alerting is deterministic but still dependent on the correctness of upstream metrics and exporter availability.
- Trace propagation is reliable under the supported configuration but cannot recover a valid span tree if upstream context has already been lost or malformed.
- Retrieval of high-volume operator signals is operationally important, but performance budgets must remain within the module's contract floor.

## Operational Relevance

For maintainers, this should be the default reasoning model when changing observability code:

- preserve the contract before changing behavior,
- reject malformed or excessive telemetry instead of dropping it silently,
- keep failure counters and operator signals visible,
- validate the change against the relevant observability test or benchmark.

## References

- [README.md](README.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [ROADMAP.md](ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- [SECURITY.md](SECURITY.md)
