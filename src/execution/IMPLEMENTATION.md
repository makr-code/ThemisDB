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

# Execution Implementation Guide

## Purpose

This page maps the live implementation to its invariants and test evidence. It
is intended for developers changing queue, shutdown, metric, or server
integration behavior. Public signatures remain defined by the
[`API.md`](API.md) and the two public headers.

## Component map

| File | Responsibility |
|---|---|
| [`query_scheduler.h`](../../include/execution/query_scheduler.h) | `QueryEntry`, `SLAPriority`, `QueryScheduler`, configuration and metrics |
| [`query_scheduler.cpp`](query_scheduler.cpp) | Bounded admission, EDF dequeue, completion accounting, shutdown wakeups |
| [`thread_pool_manager.h`](../../include/execution/thread_pool_manager.h) | Work item, pool configuration, statistics, lifecycle API |
| [`thread_pool_manager.cpp`](thread_pool_manager.cpp) | Fixed worker startup, central dispatch, task execution and shutdown |
| `src/server/http_server.cpp` | Conditional construction and teardown wiring |

## Invariants to preserve

- Queue contents and scheduler counters are protected by the scheduler mutex.
  Producer and consumer condition variables are notified when capacity or work
  changes and on shutdown.
- The scheduler computes deadlines from `steady_clock`, orders by deadline,
  and uses query id as the equal-deadline tiebreaker. Priority is metric/shedding
  metadata, not an ordering bucket.
- The scheduler rejects LOW submissions at or above `shed_threshold`; queue
  capacity remains bounded separately by `max_queue_depth`.
- Pool submissions and workers coordinate access to `dispatch_queue_` through
  `dispatch_mutex_`. Capacity and work waiters are notified after queue changes.
- Pool tasks are run outside the dispatch lock. Exceptions from a task are
  caught, counted, and do not escape the worker loop.
- The pool starts a fixed number of workers at construction. The reserved
  per-worker queue objects are not part of the live dispatch path.

## Edge cases that affect callers

- Scheduler and pool admission timeout results (`0` and `false`) combine
  overload, timeout, and shutdown; callers must inspect context/metrics rather
  than assuming one cause.
- Scheduler SLA time starts before waiting for admission. A past deadline does
  not remove an entry; it can still be dequeued.
- An unreported accepted scheduler id remains in `pending_deadlines_`. Unknown
  and duplicate completion reports increase `completed_total`; report once for
  each known id if the ratio is used as an SLA signal.
- `waitAll()` polls queue emptiness and `queued_count_`; the count drops when a
  worker pops an item, before that callable finishes. Therefore `waitAll()`
  returning `true` is not proof that all callables have completed.
- Pool shutdown waits up to `drain_timeout`, then wakes and joins workers.
  Running callables are not interrupted, and the timeout is not a guarantee
  that every queued callable has run.
- A throwing task increments `failed`; the module does not log the exception or
  expose the exception object to the submitter.

## Change and test workflow

For scheduler changes, update and run
`tests/integration/test_load_balancing.cpp`. For pool changes, update and run
`tests/integration/test_resource_pooling.cpp`. Queue contention or capacity
changes also warrant `tests/execution/test_execution_highcardinality_stress.cpp`
and the dedicated benchmark gates. When changing server lifecycle wiring,
inspect both `include/server/http_server.h` and
`src/server/http_server.cpp`.

Do not claim active work stealing, elastic scaling, cancellation, deadline
eviction, or exported metrics until those paths are implemented and covered by
tests. The roadmap tracks those as future work.

See [RUNBOOK.md](RUNBOOK.md) for failure response and
[MODULE_GUIDE.md](MODULE_GUIDE.md) for repository navigation.

Exact model backend not verifiable from final document text alone.
