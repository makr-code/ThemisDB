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

# Execution API

## Scope

This page documents the two in-process C++ APIs owned by execution. It is for
callers integrating bounded admission or task dispatch. The headers are the
canonical signature source:
[`query_scheduler.h`](../../include/execution/query_scheduler.h) and
[`thread_pool_manager.h`](../../include/execution/thread_pool_manager.h).

## QueryScheduler

Namespace: `themis::execution`.

| Operation | Contract |
|---|---|
| `QueryScheduler()` / `QueryScheduler(const Config&)` | Construct with defaults or an explicit configuration. Configuration fields are not normalized by the constructor. |
| `enqueue(execute, priority, sla_ms, name, timeout)` | Waits for capacity, records a deadline as enqueue-start time plus `sla_ms`, and returns a nonzero id on acceptance. Returns `0` for shutdown, capacity timeout, or LOW shedding. |
| `dequeue(out, timeout)` | Waits for an entry and returns the earliest deadline; query id breaks deadline ties. Returns `false` on timeout or shutdown with an empty queue. |
| `reportCompletion(query_id, completion_time)` | Records completion against the stored deadline. Unknown ids do not match a deadline; every call still increments the total completion-report count. |
| `metrics()` / `size()` | Return current in-memory metrics or queue depth. SLA completion ratio is zero before any reports. |
| `shutdown()` / `is_shutdown()` | Stop new admissions and wake blocked callers. Existing entries can still be dequeued. |

`SLAPriority` values are `HIGH`, `MEDIUM`, and `LOW`. The label controls
per-class depth counters and LOW shedding; it does not change deadline order.
`urgent_window_ms` and `default_sla_ms` are reserved fields and are not used by
the current implementation. A zero `max_queue_depth` prevents new admission.
An empty callable is not rejected by `enqueue()`; the downstream consumer must
not invoke an empty `std::function` without handling `std::bad_function_call`.

Call `reportCompletion()` once for each known accepted id when completion
metrics are required. The scheduler does not expire queued entries, cancel
work, or execute the callable itself.

## WorkStealingThreadPool

Namespace: `themis::resource`.

| Operation | Contract |
|---|---|
| `WorkStealingThreadPool()` / `WorkStealingThreadPool(const Config&)` | Start a fixed set of workers. `min_threads` is clamped to at least one and at most `max_threads`; a zero `max_threads` selects a hardware-based allocation bound. |
| `submit(WorkItem, timeout)` or `submit(fn, name, timeout)` | Wait for capacity in the shared dispatch queue. Returns `false` on shutdown or timeout. |
| `waitAll(timeout)` | Poll until the dispatch queue and queued count are empty. This can return while a worker is still executing a popped item. |
| `statistics()` / `thread_count()` | Return worker count, queue count, completed/failed counts, and sampled callable-runtime percentiles. |
| `shutdown(drain_timeout)` / `is_shutdown()` | Reject new submissions, wait for a best-effort drain, wake workers, and join them. The timeout does not cancel running work or guarantee all queued work completes. |

Despite the public class name, submissions currently use one shared dispatch
queue. `max_threads` is not an elastic scaling target, and `idle_timeout_ms` is
a wait interval, not a worker retirement timer. Exceptions thrown by a task are
caught and reflected in `Statistics::failed`; they are not returned to the
submitter.

## Concurrency and ownership

The scheduler and pool are non-copyable. Submitted callables are owned by queue
entries but any referenced objects captured by them remain the caller's
lifetime responsibility. The primitives provide no sandbox, tenant isolation,
or execution cancellation.

For behavior and implementation invariants, see
[IMPLEMENTATION.md](IMPLEMENTATION.md). For integration and teardown sequencing,
see [EMBEDDING.md](EMBEDDING.md).

Exact model backend not verifiable from final document text alone.
