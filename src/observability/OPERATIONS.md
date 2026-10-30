---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Observability Operations Guide

Exact model backend not verifiable from final document text alone.

## Purpose

This document describes the routine operational model for ThemisDB observability. It is not a marketing overview; it is a guide for operators who need to keep metrics, traces, alerting, and diagnostics available and trustworthy.

## Normal Operational Signals

Operators should watch the following categories:

- metric ingest and rejection rates,
- exporter health and recovery status,
- trace sampling and dropped-span counters,
- alert firing and resolution state,
- profiling overhead and retained diagnostic volume.

The module exposes these signals through metrics and status outputs rather than through hidden side effects.

## Actionable metric and event checks

| Signal | Interpretation and operator action |
|---|---|
| `malformed_telemetry_rejections_total{metric,reason}` | Rejected metric samples; group by `reason` and `metric`, then fix the emitting labels or cardinality at the source. Do not mask the rejection by dropping the counter. |
| `exporter_failures_total{exporter}` and `exporter_recoveries_total{exporter}` | Cumulative failure/recovery counts. Compare rates or deltas per exporter; a recovery does not reset the failure counter. |
| `exporter_health_status{exporter}` | Exporter health state; investigate unhealthy status alongside the backend endpoint and delivery logs. |
| `themis_otel_active_spans`, `themis_otel_dropped_spans_total`, `themis_otel_spans_total` | Active, dropped, and processed span signals from the OpenTelemetry tracer. Correlate drops with sampling, propagation, and exporter health. |
| `themis_slo_met`, `themis_slo_error_budget_remaining`, `themis_slo_burn_rate` (plus fast/medium/slow burn-rate gauges) | SLO status and budget burn. Alert on deployment-defined SLO policy; inspect the contributing request/error series before changing the SLO. |
| Alert firing/resolution and delivery warnings | `Alertmanager` and `AlertingEngine` report rule transitions and send/resolve failures. Restore the receiver, then verify active-alert state and delivery rather than assuming an alert was replayed. |

These are real emitted identifiers from the observability sources, but not every deployment exposes every tracer/SLO component. Confirm exporter registration and scrape configuration before creating an alert against a series.

## Typical Health Checks

A minimal operational check for the module should answer these questions:

- Are metric records being accepted without sustained rejection spikes?
- Are exporters healthy or recovering?
- Are traces legitimately parented and sampled within the configured contract?
- Are alert rules firing and resolving in a predictable way?
- Are profiling or RCA diagnostics adding material cost to the hot path?

## Operational Boundaries

The observability module owns the runtime behavior of in-process telemetry and diagnostics, but not the full external telemetry backend dependency chain. In practical terms:

- The module owns generation and formatting of observability data.
- It owns health and failure surfaces for the export path.
- It does not guarantee remote backend latency guarantees beyond the configured exporter contract.

## Operator Actions

### During a rejection spike

- validate label key/value constraints,
- inspect whether a single emitter is producing unbounded series,
- reduce cardinality or add a local pre-aggregation step,
- confirm rejection diagnostics and exporter health remain within expected bands.

### During exporter outage

- confirm endpoint configuration,
- verify the network path and target service health,
- compare `exporter_recoveries_total` as a counter delta; it does not reset,
- ensure telemetry processing continues locally instead of failing the whole application.

### During trace anomalies

- inspect upstream propagation, baggage content, and span depth,
- verify the relevant trace IDs and parent-child relationships,
- confirm no code path is silently dropping valid context without a diagnostic signal.

### During alert churn

- validate the alert rule source data,
- confirm the alert condition is still aligned with the current metric or SLO semantics,
- ensure the notification backend is reachable and the alert state transitions are consistent.

## Validation Pattern

Use the smallest relevant validation command after a change:

- targeted observability unit tests where the logic changed,
- focused benchmark runs when performance or cardinality assumptions changed,
- module-specific verification of affected exporters or alerting channels.

The repo's relevant evidence is under `tests/observability/` and `benchmarks/observability/`.

## Reliability Expectations

The module is expected to remain non-silent in degraded states. The operator should be able to diagnose the problem from counters, explicit status, and structured alert data. Any behavior that makes the module appear healthy while silently dropping telemetry should be treated as a regression.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `src/observability/metrics_collector.cpp`
- `src/observability/opentelemetry_tracer.cpp`
- `src/observability/slo_reporter.cpp`
