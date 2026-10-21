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

# Observability Module Guide

## Purpose

This is the entry point for engineers working on ThemisDB's observability subsystem. It is intentionally compact but connects the module's public surface, implementation files, test evidence, and operational runbooks.

## Reading Order

1. [README.md](README.md)
2. [ARCHITECTURE.md](ARCHITECTURE.md)
3. [API.md](API.md)
4. [IMPLEMENTATION.md](IMPLEMENTATION.md)
5. [RUNBOOK.md](RUNBOOK.md)
6. [OPERATIONS.md](OPERATIONS.md)
7. [ROADMAP.md](ROADMAP.md)
8. [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)

## Ownership and Source of Truth

- Public APIs: `include/observability/`
- Runtime implementations: `src/observability/`
- Benchmarks: `benchmarks/observability/`
- Focused tests: `tests/observability/`

The contract in `include/observability/observability_api_contract.h` is the normative source for limits, error codes, and supported behaviors.

## Maintenance Checklist

Before merging a change in this module, confirm:

- the change preserves the contract in `observability_api_contract.h`,
- label and span limits remain enforced,
- exporter health and malformed telemetry are still visible,
- tracing and metrics remain causal and bounded,
- the relevant test or benchmark still passes,
- any runtime behavior changes are reflected in the docs and runbook if necessary.

## Common Subsystem Entry Points

### Metrics and export

- `MetricsCollector`
- `MetricAggregator`
- `HighCardinalityMetrics`
- exporter health surfaces

### Tracing and propagation

- `OpenTelemetryTracer`
- `ObservabilityTracer`
- W3C baggage and span propagation code

### Profiling and RCA

- `ContinuousProfiler`
- `QueryProfiler`
- `RootCauseAnalyzer`
- alerting and incident correlation

## Change Guidance

When changing the module, prefer the following order:

1. check the contract,
2. inspect only the impacted implementation file(s),
3. validate with a focused observability test or benchmark,
4. update the relevant doc if the runtime contract or failure semantics changed.

## References

- [README.md](README.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [API.md](API.md)
- [IMPLEMENTATION.md](IMPLEMENTATION.md)
- [RUNBOOK.md](RUNBOOK.md)
- [SECURITY.md](SECURITY.md)
