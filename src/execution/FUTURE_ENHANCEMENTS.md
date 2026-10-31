---
Author: "ThemisDB Team"
Created: "2026-09-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Execution Module — Future Enhancements

## Scope

Forward-looking work for the bounded in-process scheduler and fixed-worker pool:
true work stealing, elastic worker management, queue expiry/cancellation policy,
and operator observability. Current runtime behavior is defined by the public
headers and `ARCHITECTURE.md`.

## Design Constraints

- Preserve the current fail-closed submission behavior when capacity or shutdown conditions reject work.
- Keep queue growth and worker lifecycle bounded by explicit configuration; no hidden unbounded retries.
- Preserve earliest-deadline ordering and query-id tie-breaking; specify deterministic selection rules before adding steal behavior.
- Avoid introducing compatibility-only legacy paths; new execution behavior must replace or clearly supersede the current reserved extension hooks.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `themis::execution::QueryScheduler` | execution-aware server/query paths | retain stable enqueue/dequeue/reporting entry points while extending policy behavior |
| `themis::resource::WorkStealingThreadPool` | in-process execution consumers | preserve bounded submission and shutdown semantics while enabling true steal/deque behavior |
| metrics / observability surface | server and operator tooling | export scheduler and worker-pool health without requiring internal structure access |

## Implementation Notes

### True steal-path activation
**Priority:** High  
**Target:** Q1 2027

- route work into per-worker deques instead of relying exclusively on `dispatch_queue_`
- implement cross-worker steal attempts before returning idle from `tryGetWork()`
- add deterministic steal-order rules to avoid starvation and lock-order regressions

### Elastic worker lifecycle
**Priority:** High  
**Target:** Q1 2027

- decide whether `max_threads` remains a runtime-scaling control or becomes an allocation bound only
- implement worker growth and shrink semantics if the runtime-scaling contract remains public
- redefine `idle_timeout_ms` as a real retirement/parking policy only after focused concurrency validation exists

### Deadline and cancellation policy hardening
**Priority:** Medium  
**Target:** Q1 2027

- add optional expiry eviction or cancellation handling for queued scheduler entries
- surface explicit status for timed-out or abandoned query ids so metrics stay meaningful
- document whether dequeued-but-unreported queries require compensating completion/failure reporting

### Observability and operator evidence
**Priority:** Medium  
**Target:** Q2 2027

- export queue depth, shed count, completion rate, and task-failure counters to a shared telemetry surface
- add operator guidance for diagnosing queue saturation and stuck-drain situations
- align benchmark and soak evidence with representative release hardware baselines

## Test Strategy

- add focused concurrency regressions for real steal-path behavior and lock-order safety
- add lifecycle tests for any elastic worker growth/shrink implementation
- add cancellation/expiry tests before enabling automatic stale-query handling
- keep dedicated benchmark gates and long-duration soak coverage aligned with the new behavior

## Performance Targets

- Establish and record a production-runtime baseline for scheduler enqueue/dequeue
  latency and pool dispatch/drain throughput on named representative hardware
  before enabling new paths. The current benchmark source uses standalone queue
  models and does not provide that baseline.
- For each future steal or elastic-scaling change, compare the same production
  benchmark cases against the recorded baseline and publish measured latency and
  throughput deltas; no validated numeric release target is currently available.

## Security / Reliability

- reject work cleanly under overload instead of silently dropping non-LOW items
- maintain deterministic shutdown even while workers are parked, stealing, or shrinking
- keep exception containment and bounded in-memory metrics behavior intact when new execution paths are added
