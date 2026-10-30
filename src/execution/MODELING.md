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

# Execution Queue Model

## Purpose

This document gives developers a compact model of admission, ordering, and
dispatch in `src/execution/`. It describes deterministic code behavior; it is
not a predictive or learned scheduling model.

## Scheduler state and transitions

`QueryScheduler` stores entries in a bounded priority queue. For an enqueue at
monotonic time `t0`, it records the deadline `t0 + sla_ms`. The scheduler first
waits until queue capacity is available or the admission timeout expires. Since
the deadline is computed before that wait, time spent waiting for admission
consumes the supplied SLA budget.

The comparator selects the smallest absolute deadline. Equal deadlines are
ordered by the monotonically assigned query id, so earlier accepted entries
come first. `SLAPriority` does not add a separate priority ordering:

- `HIGH`, `MEDIUM`, and `LOW` are counted in depth metrics.
- A new LOW entry is rejected when the current queue size is at or above
  `shed_threshold`.
- Other priority labels do not change the EDF comparator.

`max_queue_depth` is the hard admission limit. The LOW shedding threshold is a
separate check after capacity becomes available; configure it relative to the
capacity if shedding is intended to occur before the queue is full. A zero
capacity prevents admission until timeout or shutdown.

## Worker-pool state and transitions

`WorkStealingThreadPool` has a second, independent bounded queue. `submit()`
waits for `max_queue_depth` capacity and appends to the central dispatch queue.
Workers pop from that queue and execute callables. Although the type is named
`WorkStealingThreadPool`, the live path does not populate or steal from the
reserved per-worker queues.

The constructor starts `min_threads` workers (clamped to at least one and no
more than `max_threads`). No worker growth or retirement occurs during normal
operation. `idle_timeout_ms` controls how long a worker waits before checking
again; it is not a retirement policy.

## Observable outcomes

| Condition | Scheduler | Worker pool |
|---|---|---|
| Queue capacity does not become available before timeout | `enqueue()` returns `0` | `submit()` returns `false` |
| Shutdown has started | new `enqueue()` returns `0` | new `submit()` returns `false` |
| LOW work reaches the shed threshold | `enqueue()` returns `0`, shed count increments | Not applicable |
| Task callable throws | Not executed by the scheduler itself | Exception is caught and `failed` increments |
| Work deadline passes | Entry remains queued until dequeued | Running work is not interrupted |

Scheduler and pool queues have separate limits and counters. A successful
`enqueue()` does not imply a subsequent pool submission succeeds; the component
that bridges the two must handle both outcomes.

## Measurement boundaries

Scheduler `avg_enqueue_us` and `avg_dequeue_us` are successful operation
latencies. SLA completion metrics require callers to invoke
`reportCompletion(query_id, completion_time)`. The report counter includes
unknown or repeated ids, while a deadline is removed only on the first report
for a known id; do not treat raw completion counts as a unique-query count
unless the caller guarantees one report per accepted id.

Pool `p50_latency_us` and `p99_latency_us` summarize callable execution time
from a bounded in-memory sample buffer. They do not include queue wait and are
not exported by this module.

See [RESEARCH.md](RESEARCH.md) for evaluation scope and
[API.md](API.md) for caller-visible signatures.

## Source references

- [`query_scheduler.h`](../../include/execution/query_scheduler.h)
- [`query_scheduler.cpp`](query_scheduler.cpp)
- [`thread_pool_manager.h`](../../include/execution/thread_pool_manager.h)
- [`thread_pool_manager.cpp`](thread_pool_manager.cpp)

Exact model backend not verifiable from final document text alone.
