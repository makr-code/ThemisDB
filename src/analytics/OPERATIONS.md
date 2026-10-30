---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics Operations

## Scope

This guide is for deployment owners operating analytics runtime paths. It
covers configuration, health signals, and safe changes; it is not a substitute
for the service's storage, query, server, or model-provider runbooks.

## Pre-deployment checks

- Confirm the intended exporters and serving backends were built and are
  available. Treat optional dependencies as explicit capabilities; verify
  `isBackendAvailable()` or `supportsFormat()` rather than assuming support.
- Set explicit streaming and bounded-execution limits for the expected
  workload. Zero values in the shared policy structures mean unconstrained,
  not a safe production default.
- Validate model name/version, input tensor contract, and model-integrity
  policy before accepting production traffic.
- Configure external TensorFlow Serving with HTTPS and certificate
  verification. Plain HTTP is permitted only through an explicit opt-in and
  must not be used in production.
- Verify authorization for analytics export and authentication middleware for
  Arrow Flight at the owning server boundary, as required by
  [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md).
- For process mining, verify the binary was not built with
  `THEMIS_PROCESS_MINING_WINDOWS_STUB` if mining operations are required.

## Monitor

Use structured results rather than relying on a single success boolean:

| Surface | Useful signals |
|---|---|
| Model registry | prediction counts, batch counts, average/p99/last latency, last-used time from `ModelHealthMetrics` |
| External inference | response status, latency, operation/correlation IDs, failure class, operator hints |
| Export | status, rows exported, bytes written, duration, operation/correlation IDs |
| Distributed coordination | success/total shard counts, per-shard execution/circuit state, total and merge duration, failure class |
| Streaming anomaly/window paths | current window size, trained state, anomaly count/rate, `partition_keys_rejected`, `windows_evicted`, and backpressure behavior |

Correlate these fields with upstream request rates, resource saturation, and
provider health. The module's performance thresholds and baseline requirements
are maintained in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
and [`REPRESENTATIVE_HARDWARE_BASELINES.md`](./REPRESENTATIVE_HARDWARE_BASELINES.md);
do not infer a production SLA from the API defaults.

## Change, rollback, and escalation

Roll out a model, limit, or backend change against representative fixtures and
the applicable focused tests/benchmarks. Observe status mix, tail latency,
queue/window limits, partial shard counts, and export volume. Roll back if
results become incomplete, error rates exceed the deployment's approved
thresholds, or a change requires disabling TLS, authorization, integrity, or
resource bounds.

Use [RUNBOOK.md](./RUNBOOK.md) for incident-specific response. Escalate
integrity/security failures through [TRUST.md](./TRUST.md) and project
[`SECURITY.md`](../../SECURITY.md). Preserve correlation identifiers and
redacted diagnostics for maintainers; never attach API keys, tokens, or raw
sensitive records to an issue.
