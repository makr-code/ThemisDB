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

# Observability API Contract Summary

## Purpose

This document is the engineering-facing summary of the observability public APIs exposed by ThemisDB. It ties the implementation to the public headers under `include/observability/` without becoming a duplicate of the source headers.

## Canonical API Surface

### Metrics and exporter health

Relevant headers:

- `include/observability/metrics_collector.h`
- `include/observability/advanced_metrics.h`
- `include/observability/high_cardinality_metrics.h`

Key responsibilities:

- record counters, gauges, and histograms,
- enforce cardinality and label constraints,
- add exporter failure and recovery tracking,
- emit degraded-case metrics for malformed telemetry,
- expose Prometheus text output and exporter incident snapshots.

Critical invariants:

- metric cardinality is bounded,
- invalid or malformed series are explicitly rejected or counted,
- exporter failures remain visible through health counters.

### Tracing and propagation

Relevant headers:

- `include/observability/tracer.h`
- `include/observability/opentelemetry_tracer.h`
- `include/observability/distributed_tracing_sdk.h`

Key responsibilities:

- create spans and child spans,
- propagate W3C trace context and baggage,
- flush and initialize tracer state,
- capture span metrics and attach diagnostic context.

Critical invariants:

- trace context must remain causal and bounded,
- baggage size and span depth remain within contract limits,
- propagation failures surface explicit issues rather than silent no-op behavior.

### Alerting and notification

Relevant headers:

- `include/observability/alerting_engine.h`
- `include/observability/alertmanager.h`
- `include/observability/root_cause_analyzer.h`

Key responsibilities:

- evaluate alert rules,
- create and route alert objects,
- manage firing/resolved/silenced states,
- analyze root cause and surface remediation hints.

Critical invariants:

- alert evaluation remains deterministic under the same input,
- channel failures are surfaced as explicit errors,
- alert state transitions remain clear to operators.

## Contract Boundaries

The module's public contract is intentionally narrow and explicit:

- it controls internal observability semantics,
- it does not own the external telemetry backend, only the integration behavior,
- it treats exporter degradation as part of the runtime contract,
- it rejects invalid internal states instead of silently substituting defaults.

## Required Usage Model

Consumers should treat observability as a best-effort, fail-closed runtime support layer:

1. initialize metric/tracing resources under the contract limits,
2. allow invalid or overloaded inputs to fail explicitly,
3. check exporter health and alert state when operator action is required,
4. prefer structured errors over silent drop behavior.

## Source Grounding

The authoritative implementation references are:

- `src/observability/metrics_collector.cpp`
- `src/observability/tracer.cpp`
- `src/observability/opentelemetry_tracer.cpp`
- `src/observability/alerting_engine.cpp`
- `include/observability/observability_api_contract.h`

## References

- [IMPLEMENTATION.md](IMPLEMENTATION.md)
- [MODULE_GUIDE.md](MODULE_GUIDE.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [SECURITY.md](SECURITY.md)
