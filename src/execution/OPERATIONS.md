---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Execution Operations

## Purpose and boundary

This document covers the ongoing configuration, observability, and lifecycle
responsibilities for teams embedding execution in a service. Incident-specific
symptoms and recovery steps are in [RUNBOOK.md](RUNBOOK.md).

The module is an in-process library component, not a deployable service. It has
no execution CLI, remote management API, persistent queue, or metrics exporter.
The host application owns deployment, telemetry export, and alerting.

## Configuration and capacity

- Set finite `max_queue_depth` values for both scheduler and pool based on the
  host's memory and downstream service budget. They are separate queues.
- Set a finite admission timeout at each submit boundary and define how the host
  handles `enqueue() == 0` and `submit() == false`.
- Set `shed_threshold` intentionally. It is a LOW-priority rejection threshold
  separate from queue capacity; the default threshold can exceed the default
  scheduler capacity and then cannot be reached.
- Set worker count with the live semantics in mind: `min_threads` workers start
  immediately; `max_threads` does not trigger growth. `idle_timeout_ms` is a
  wait interval, not worker retirement.
- Coordinate scheduler and pool limits with upstream retry behavior. A bounded
  queue does not bound retries initiated outside the module.

## Monitoring responsibilities

Export snapshots through the owning service if operators need them:

| Primitive | Useful signals | Interpretation |
|---|---|---|
| Scheduler | queue depth by label, `total_shed`, total enqueued/dequeued, SLA reports, average enqueue/dequeue latency | SLA reports are incomplete unless consumers call `reportCompletion()` consistently. |
| Pool | `queued_items`, `active_threads`, `completed`, `failed`, p50/p99 task runtime | Runtime percentiles exclude time spent waiting in the dispatch queue. |

Metrics are in-memory snapshots and are not durable across process restart.
Define host-level alert thresholds from observed service baselines; the module
does not set universal thresholds.

## Lifecycle and change control

During normal operation, keep task captures alive through execution and handle
both queue boundaries explicitly. At shutdown, stop producers, stop new
scheduler admissions, coordinate the scheduler consumer, then drain and join
the pool before destroying state referenced by tasks. Pool `shutdown()` uses a
best-effort drain timeout; task bodies are not forcibly cancelled.

After changes to queue limits, thread counts, server wiring, or worker behavior:

1. Run focused scheduler and pool integration tests.
2. Add or run a stress test against the production scheduler/pool when changing
   capacity or contention; the existing high-cardinality test uses local models.
3. Use the dedicated benchmark harness only for its local queue models. Compare
   production-runtime benchmarks against a representative-hardware baseline
   once that baseline is established.
4. Confirm the host still exports rejection, shed, task-failure, and queue
   health signals as intended.

See [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) for the module's
deployment requirements and [RESEARCH.md](RESEARCH.md) for evidence boundaries.

Exact model backend not verifiable from final document text alone.
