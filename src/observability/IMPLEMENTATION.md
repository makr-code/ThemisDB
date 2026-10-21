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

# Observability Implementation Notes

## Purpose

This document explains how the module implements its declarative contract in practice. It is meant to orient maintainers who are modifying metrics, tracing, alerting, or profiling behavior and need a reliable map of the actual code paths and edge-case semantics.

## Primary Implementation Files

- `src/observability/metrics_collector.cpp` — collector logic, cardinality limits, exporter health, malformed-telemetry diagnostics
- `src/observability/tracer.cpp` — span lifecycle, ring-buffer retention, profiling snapshots
- `src/observability/opentelemetry_tracer.cpp` — W3C trace context, exporter wiring, propagation helpers
- `src/observability/alerting_engine.cpp` — rule evaluation and notification output
- `src/observability/root_cause_analyzer.cpp` — operator diagnosis and incident correlation
- `src/observability/continuous_profiler.cpp` — runtime profiling and sample capture

## Implementation Principles

### 1. Bounded telemetry is a correctness property

The collector enforces label count, key length, and value length constraints and retains a cardinality limit to prevent the runtime from silently exploding into high-cardinality series. This is implemented as explicit validation and rejection logic rather than deferred cleanup.

### 2. Fail-closed semantics for exporter problems

Exporter failures are not treated as harmless background noise. The code records exporter health transitions and keeps the primary telemetry collection path alive. In other words, exporter outage is visible, but it does not block the application from producing local telemetry.

### 3. Traces are causal data structures

Span creation and closure are idempotent and tracked with explicit lifetime transitions. In `tracer.cpp`, spans retain their trace and parent IDs and keep diagnostics for the requested profile and error states. Sampling and retention are designed to avoid silent span loss in the supported deployment path.

### 4. Alerts are operationally deterministic

The alerting engine serializes alert metadata into a consistent data model and then pushes the alert into the configured notification channel. The system prefers explicit failure returns (for example invalid URL or failed transport) over silent no-op behavior.

## Edge Cases the Implementation Explicitly Handles

- malformed metric labels or label value overflow
- oversize cardinality on metric families
- exporter recovery and failure transitions
- SLO window closure and late updates
- invalid trace context or unsupported propagation format
- over-deep nesting or unsupported span state
- profiler capture failure without blocking the main execution path

A large share of these semantics is codified in `include/observability/observability_api_contract.h`, which is the canonical behavior reference for future changes.

## Threading and Stability Expectations

The implementation uses locking and synchronization to keep the telemetry pipeline safe under concurrent writes:

- `MetricsCollector` exposes thread-safe access patterns and synchronization for metric maps and histograms.
- span attributes and ring-buffer retention are safe under concurrent lifecycle changes.
- alert manager state uses explicit locking around active alert management.

This is not a guarantee of unlimited throughput — it is a guarantee that the module tries to preserve correctness even under parallel producer load.

## Failure Handling Model

The code generally follows this sequence:

1. validate the input or config,
2. reject or drop with explicit diagnostics when the input is invalid,
3. preserve the main runtime path,
4. expose a clear operator signal through counters, status fields, or logs,
5. keep the affected part in a bounded, recoverable state.

This pattern appears in both metrics and tracing code and is the expected pattern for any new observability change.

## Validation Checklist for Changes

When editing observability code, verify:

- the new path still respects `observability_api_contract.h` limits,
- invalid labels or malformed contexts are surfaced explicitly,
- exporter and alert failures still emit visible signal,
- tracing remains causal and bounded,
- metrics retention and rejection counters remain meaningful,
- the relevant observability tests or benchmark still pass.

## References

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [MODELING.md](MODELING.md)
- [API.md](API.md)
- [RESEARCH.md](RESEARCH.md)
- [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- [include/observability/observability_api_contract.h](../../include/observability/observability_api_contract.h)
