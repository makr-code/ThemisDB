# Execution Module — Future Enhancements

## Scope

Forward-looking work for the execution module after the current bounded baseline: true work stealing, elastic worker management, richer scheduler policies, and stronger observability/export surfaces.

## Design Constraints

- Preserve the current fail-closed submission behavior when capacity or shutdown conditions reject work.
- Keep queue growth and worker lifecycle bounded by explicit configuration; no hidden unbounded retries.
- Maintain deterministic dequeue order for equal deadlines and deterministic steal/selection rules once new behavior is added.
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

- populate per-worker deques during submission instead of routing everything through `dispatch_queue_`
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

- maintain bounded enqueue/dequeue overhead after adding steal-path logic
- keep worker wake-up and drain latency within benchmarked release envelopes
- ensure elastic-scaling changes do not regress steady-state throughput for small fixed workloads

## Security / Reliability

- reject work cleanly under overload instead of silently dropping non-LOW items
- maintain deterministic shutdown even while workers are parked, stealing, or shrinking
- keep exception containment and bounded in-memory metrics behavior intact when new execution paths are added
