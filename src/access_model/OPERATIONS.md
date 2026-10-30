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

# Access Model Operations

## Operational surface

`access_model` is a library component, not a standalone service. It does not
provide an HTTP endpoint, command-line control plane, Prometheus exporter, or
operator-managed queue interface. The application embedding it is responsible
for exposing telemetry and deployment controls.

The coordinator exposes aggregate `AccessModelMetrics` and recent transition
records. The metrics structure declares initiated/succeeded/failed promotion
and demotion counters, observed eviction/hot-access counters, and
event-processing, promotion, and policy-decision latency histograms. Current
coordinator event/task processing updates counters but does not record samples
to the latency histograms. `getKeyMetrics()` and `getTierMetrics()` currently
return empty metrics. The coordinator does not expose queue depth or worker
utilization, despite those being discussed in some older module material.

Structured logs include tier transition, eviction, promotion decision, and
lifecycle events with key, tier, reason/status, latency, thread ID, and
correlation ID as applicable. Logging exceptions are caught by the default
logger; absence of a log line is not proof that no event occurred.

## Routine checks

- At startup, confirm that the owning application constructed the expected
  worker count, registered the intended tiers, set a validated policy, started
  the coordinator, and registered listener pointers with safe lifetimes.
- During operation, compare event-observation counters and transition records
  with the owning cache/storage adapter's own metrics and backend state.
- Track the benchmark metrics listed in
  [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) using the
  designated release-profile benchmark. The published values are gates, not
  production alert thresholds.
- Retain the benchmark output/manifest and hardware/configuration details when
  assessing a regression. The Wave B evidence bundle requires representative
  hardware re-capture before GA promotion.

## Capacity and failure handling

The worker count is fixed when the coordinator is constructed. The pending task
queue is an unbounded FIFO, and its depth cannot be queried through the current
API. If event volume outpaces processing, the module has no built-in queue
backpressure control; the caller must reduce or stop event production using
its own safe integration controls and escalate before increasing concurrency.

Do not use `promotions_succeeded`/`demotions_succeeded` as a data-integrity
signal. Current task processing does not execute tier transfers. Verify
application/backend behavior independently and follow
[`RUNBOOK.md`](./RUNBOOK.md) when counters, futures, or backend state disagree.

## Shutdown and rollback

Unregister event listeners before destroying the coordinator. `shutdown()`
stops and joins workers but does not drain pending tasks; capture diagnostic
state before stopping it. Rollback is controlled by the application that owns
the integration: this module defines no runtime feature flag, remote endpoint,
or release rollback command.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
