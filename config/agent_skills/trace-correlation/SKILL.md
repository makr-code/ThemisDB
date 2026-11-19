---
name: trace-correlation
description: Reconstruct an authorized ThemisDB request path from trace spans and correlate it with logs without inventing causal links.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Correlate a distributed trace

Use this skill when investigating a request across components. Use
`log-analysis` for an investigation without trace or span evidence.

1. Establish the deployment, authorized services, time window, and supplied
   trace or request ID. Use only a host-authorized read-only telemetry
   interface or a supplied export. Do not infer identity or access from trace
   headers or baggage.
2. Use trace ID to group spans and parent span IDs to connect them. Correlate
   logs by recorded trace, span, or request identifiers. Label timestamp-only
   matches as tentative and preserve timezone and clock-skew caveats.
3. Identify the first recorded error and longest well-supported elapsed stage.
   Do not add overlapping child durations or assume a missing span means no
   call occurred. Consider sampling, propagation gaps, and exporter health;
   see `src/observability/API.md` for the deployment's integration boundaries.
4. Return a short timeline with observed facts, unresolved links, and one
   read-only verification step. Redact sensitive attributes and do not alter
   tracing configuration, restart services, or replay writes.
