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

The coordinator exposes aggregate `AccessModelMetrics`, per-key/per-tier
counters, and recent transition records. Aggregate metrics include initiated,
succeeded, and failed promotion/demotion counters; observed eviction/hot-access
counters; event-processing, policy-decision, promotion, and demotion histograms;
and current/configured pending-task queue depth. Per-key history is retained for
up to 10,000 recently touched keys; per-tier metrics cover registered tiers.
Worker utilization is not exposed.

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

The worker count and maximum pending queue depth are fixed at construction. The
default queue capacity is 10,000; deployments should choose a value based on
measured ingress and backend latency. Pending demotion plans are limited by the
same capacity. If the task queue fills, `promoteAsync()` completes immediately
with failure, while `void` event callbacks record rejection in metrics/history.
Reduce ingress or tune the configured limit only after confirming the backend
can sustain the resulting concurrency.

Compare `promotions_succeeded`/`demotions_succeeded` with backend state.
Transfers invoke adapters, but they are not cross-tier transactions: a failed
source invalidation after destination placement can leave duplicates. Follow
[`RUNBOOK.md`](./RUNBOOK.md) when counters, futures, or backend state disagree.

## Shutdown and rollback

Unregister event listeners before destroying the coordinator. `shutdown()`
rejects new queue admission, drains accepted tasks, and joins workers. An
adapter call without a timeout may delay shutdown; capture diagnostic state
first. Rollback is controlled by the application that owns the integration:
this module defines no runtime feature flag, remote endpoint, or release
rollback command.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
