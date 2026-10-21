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

# Observability Runbook

## Purpose

This runbook is for operators and maintainers who need to diagnose and recover from observability failures in ThemisDB. It is intentionally tied to the actual failure surfaces exposed by the module: exporter health, malformed telemetry, trace propagation issues, alerting failures, and profiling performance degradation.

## Incident Categories

### 1. Exporter outage or degraded health

Symptoms:

- exporter health status indicates failure or repeated recovery loops,
- monitoring shows spikes in exporter failure counters,
- requested metrics or traces are not reaching the external backend.

What to check:

- `MetricsCollector::recordExporterFailure()` / recovery paths
- exporter endpoint and transport configuration
- network reachability and backend saturation

Recovery:

- restore backend connectivity,
- allow the collector to continue local metrics collection,
- confirm the exporter recovery counter returns to baseline,
- revalidate the external endpoint integration.

### 2. Malformed telemetry or cardinality spikes

Symptoms:

- increased rejection counts,
- abrupt growth in invalid label or cardinality diagnostics,
- large discrepancies between attempted series and emitted series.

What to check:

- `kMaxMetricLabels`, `kMaxLabelKeyBytes`, `kMaxLabelValueBytes`, and cardinality limits in `observability_api_contract.h`
- label generation in the emitting module
- pre-aggregation or dimension reduction in the producer path

Recovery:

- reduce label cardinality or sanitize label keys/values,
- ensure invalid inputs are rejected before hitting the collector,
- re-run the focused observability tests to confirm rejection behavior remains bounded.

### 3. Trace propagation problems

Symptoms:

- high numbers of invalid or dropped spans,
- missing causal ancestry across service boundaries,
- inconsistent W3C headers or orphaned trace chains.

What to check:

- `OpenTelemetryTracer` propagation and baggage handling,
- upstream/downstream header injection,
- `kMaxSpanNestingDepth`, `kMaxBaggageItems`, and trace-context size limits.

Recovery:

- re-establish upstream propagation,
- validate trace IDs and span IDs across the boundary,
- ensure the client or service emits valid `traceparent` and baggage values.

### 4. Alerting failure or delivery degradation

Symptoms:

- alert rules fire but notifications do not arrive,
- channel configuration is rejected or timed out,
- firing/resolved transitions are inconsistent.

What to check:

- `Alertmanager` configuration and HTTP delivery paths,
- retry behavior and timeout settings,
- alert state transitions in `AlertingEngine`.

Recovery:

- restore alert receiver connectivity,
- validate the endpoint and auth configuration,
- replay or reconcile the active alert list after network recovery.

### 5. Profiler overhead or degraded diagnostics

Symptoms:

- high CPU overhead while profiling,
- performance regressions in query or storage workloads,
- profiler snapshots are missing or stale.

What to check:

- `QueryProfiler`, `ContinuousProfiler`, and `StorageProfiler` sampling choices,
- retention and profiling frequency,
- any operation that attaches snapshot data to active spans.

Recovery:

- lower sampling or reduce profile retention,
- temporarily disable aggressive diagnostics for the affected path,
- confirm the profiler still records bounded snapshots without blocking the main path.

## Recovery Sequence

1. Confirm the failing signal and count type.
2. Identify the module or backend associated with the incident.
3. Check the contract limits and related counters.
4. Isolate whether the issue is ingestion, propagation, export, or operator notification.
5. Apply the smallest corrective action.
6. Re-run the relevant observability test or benchmark.
7. Confirm the recovery is visible in counters and health status.

## Rollback Guidance

Rollback is appropriate when a configuration change or code change causes an observability regression that prevents accurate diagnosis or degrades the cores of the module. Prefer rollback of the specific configuration change or the minimal runtime patch rather than disabling the whole observability layer unless the impact is severe.

## Escalation

Escalate when:

- exporter failure persists across multiple recovery cycles,
- telemetry rejection rates remain elevated after label sanitization,
- trace or alerting data becomes materially misleading,
- profiles or diagnostics add enough overhead to affect user-facing SLAs.

## References

- [OPERATIONS.md](OPERATIONS.md)
- [SECURITY.md](SECURITY.md)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
