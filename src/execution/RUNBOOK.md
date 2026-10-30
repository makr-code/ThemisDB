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

# Execution Runbook

## Scope and safety

This runbook is for an operator or service maintainer diagnosing queue
backpressure, task failures, or teardown problems in a process using execution.
The module has no built-in metrics exporter, logging channel, or administrative
endpoint. Obtain `QueryScheduler::metrics()` and
`WorkStealingThreadPool::statistics()` through the integrating component; do
not assume these snapshots are already present in dashboards.

## Queue rejection or rising backlog

**Symptoms:** scheduler `enqueue()` returns `0`, pool `submit()` returns
`false`, or the host reports rising queue depth.

1. Determine which boundary rejected work. Scheduler depth and shed count are
   separate from the pool's `queued_items`.
2. Check shutdown state before treating rejection as ordinary overload.
3. For the scheduler, inspect `queue_depth_high/medium/low` and `total_shed`.
   A rising shed count means LOW work reached the configured threshold. For the
   pool, inspect `queued_items`, `active_threads`, and completed/failed counts.
4. Check the caller's timeout and upstream retry policy. Repeated immediate
   retries can amplify load; do not increase queue limits as the first response
   without checking memory and downstream capacity.
5. Apply the host's established load-shedding or admission-control procedure.
   The module cannot cancel or remove already queued work.

## Task failures

**Symptoms:** pool `failed` increases or expected task effects are absent.

1. Compare `failed` and `completed` deltas with the task submission interval.
2. Identify the submitted callable in the integrating component; the pool
   catches all task exceptions but does not preserve or log the exception.
3. Handle the application failure at the task owner. Do not treat the worker
   remaining alive as evidence that the task succeeded.
4. If failures follow a release or configuration change, use the service
   rollback procedure and preserve the relevant counters/logs before restart.

## Slow or incomplete shutdown

**Symptoms:** teardown waits for the configured drain timeout or queued work
does not complete.

1. Check whether a task is still running; the pool cannot interrupt it.
2. Check whether new submissions are still reaching the pool. Once shutdown
   begins, new submissions should fail.
3. Treat the drain timeout as best effort, not proof that every queued task ran.
   `waitAll()` can observe an empty queue while a worker is still executing.
4. Do not destroy objects captured by task callables until the owning pool has
   completed its join.
5. Escalate a reproducible stuck task to the owner of the callable and the
   server lifecycle integration. A process restart is a host-level recovery,
   not a module-level cancellation mechanism.

## Recovery and rollback

There is no execution-specific CLI or safe runtime queue-resize control
documented by this module. Make configuration changes through the host's
reviewed deployment process, keep finite timeouts, and revert a change if
focused scheduler/pool tests or benchmark baselines regress. After recovery,
verify queue depths stop growing, rejection rates return to expected levels,
and task failure counts stabilize.

For routine capacity policy see [OPERATIONS.md](OPERATIONS.md); for known
limitations see [TRUST.md](TRUST.md) and [SECURITY.md](SECURITY.md).

Exact model backend not verifiable from final document text alone.
