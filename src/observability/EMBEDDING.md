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

# Observability Embedding and Correlation Model

## Purpose

This document explains how ThemisDB embeds operational context into metrics, traces, and diagnostic outputs so operators can correlate a single runtime event across multiple observability surfaces.

The live examples are in `include/observability/metrics_collector.h` and the tracer implementations in `src/observability/tracer.cpp` and `src/observability/opentelemetry_tracer.cpp`.

## Core Idea

Observability data is not just a set of counters. It is a correlated operational signal that connects:

- what happened,
- which request or trace caused it,
- which component observed it,
- which failure or recovery path followed.

This correlation is represented in two forms:

1. metric exemplars, where a histogram observation is associated with a trace ID,
2. W3C trace context and baggage propagation, where a request carries causal metadata across processes.

## Metric Exemplars

`MetricsCollector` supports exemplars through `Exemplar` in `include/observability/metrics_collector.h`.

The contract is intentionally narrow and deterministic:

- only a single exemplar is retained per metric series,
- the exemplar records a trace ID and value at observation time,
- the metric remains a metric, while the trace ID makes the output diagnosable.

This is the module's canonical example of embedding trace context into a metric stream without making the metric itself a large structured object.

## Trace Context Embedding

`OpenTelemetryTracer` injects `traceparent` and `baggage` headers to propagate downstream causal context. The API contract in `include/observability/opentelemetry_tracer.h` defines the context objects and the valid limits:

- `SpanContext` with `trace_id` and `span_id`
- baggage item limits from `kMaxBaggageItems`, `kMaxBaggageKeyBytes`, and `kMaxBaggageValueBytes`
- bounded serialization length via `kMaxTraceContextHeaderBytes`

This is the key embedding mechanism for distributed requests: a single trace carries the operational identity across services and backends.

## Profiling-Trace Coupling

In `tracer.cpp`, `ObservabilitySpan` may attach a `ContinuousProfiler` snapshot when profiling is enabled. That gives the span a richer operational record while keeping the main metric/trace pipeline independent from profiler availability.

The implementation intentionally treats profiler capture as best-effort:

- if profiling snapshot creation fails, the span still ends normally,
- the failure is not allowed to block request completion,
- the diagnostic remains non-silent through the underlying profiler and trace record structure.

## Operational Trade-offs

The embedding model chooses portability and boundedness over unrestricted correlation:

- baggage is bounded for safety and size control,
- exemplars are last-write-wins rather than full per-event history,
- contextual enrichment is preserved only while the runtime metadata stays valid,
- invalid or oversized context is rejected or dropped explicitly rather than being silently persisted.

This is consistent with the module-wide fail-closed design.

## Where to Look in the Code

- `include/observability/metrics_collector.h` — `Exemplar` and metric API
- `include/observability/opentelemetry_tracer.h` — `SpanContext`, baggage, and tracing config
- `src/observability/tracer.cpp` — span lifecycle and profiling snapshot attachment
- `src/observability/opentelemetry_tracer.cpp` — context propagation and exporter integration
- `include/observability/observability_api_contract.h` — metric/tracing limits and failure codes

## Change Guidance

When changing observability correlation behavior, preserve these invariants:

- do not expand baggage or exemplar size without a matching contract update,
- do not drop trace IDs silently during metric recording,
- do not allow profiler attachment to block span completion,
- do not weaken the correlation failure signals for malformed context.

## References

- [MODELING.md](MODELING.md)
- [IMPLEMENTATION.md](IMPLEMENTATION.md)
- [API.md](API.md)
- [RUNBOOK.md](RUNBOOK.md)
