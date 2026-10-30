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

# Embedding the Execution Module

## Meaning and scope

Here, “embedding” means composing the execution primitives inside a host
component. It does not mean vector embedding generation or retrieval. This
guide is for developers integrating the scheduler or worker pool into an
in-process consumer.

## Current server integration

`HttpServer` owns `QueryScheduler` and `WorkStealingThreadPool` instances when
`THEMIS_EXECUTION_MODULE` is enabled. The constructor creates them with default
configuration, and teardown requests scheduler shutdown and resets the pool.
This is construction and lifecycle wiring only: the current server request path
does not submit queries through these members. Do not infer request-level
backpressure from the existence of the members.

The compile option and owner are described in the server source and
`include/server/http_server.h`. The [README.md](README.md) gives the existing
module build command.

## Integration sequence for a new consumer

1. Construct one scheduler and/or pool with explicit queue limits suitable for
   the consumer's workload.
2. Keep scheduler admission and pool submission as separate decisions. A
   consumer that dequeues an entry and submits its callable must handle a
   scheduler id of `0` and a pool result of `false`.
3. Use finite timeouts. Avoid unbounded caller waits and blind immediate
   retries, which can move overload into the host component.
4. If SLA metrics are consumed, report completion exactly once for each known
   accepted id after the host has finished the work.
5. On teardown, stop scheduler admission, coordinate any consumer that drains
   remaining entries, then shut down the pool and join workers before destroying
   state captured by task callables.

The module does not provide an adapter that automatically connects the
scheduler to the pool. The consumer owns that handoff and its failure policy.

## Lifetime and ownership constraints

`QueryEntry` and `WorkItem` own their `std::function` callables. Captures inside
those callables retain their usual C++ lifetime rules: do not capture references
to objects that can be destroyed before execution completes. Shutdown is not a
cancellation mechanism for running callables. Pool shutdown has a best-effort
queue drain timeout; after that timeout, completion of all queued work is not
guaranteed.

Both primitives are non-copyable and own synchronization state. Their public
headers document the exact API and default timeout values:

- [`query_scheduler.h`](../../include/execution/query_scheduler.h)
- [`thread_pool_manager.h`](../../include/execution/thread_pool_manager.h)

## Validation before integration

Exercise success, capacity timeout, post-shutdown rejection, callable
exceptions, and teardown with the focused tests in
`tests/integration/test_load_balancing.cpp` and
`tests/integration/test_resource_pooling.cpp`. If the consumer makes a
scheduler-to-pool handoff, add or run an integration test that checks failure
at either admission boundary and verifies the consumer's cleanup behavior.

For code-level invariants, see [IMPLEMENTATION.md](IMPLEMENTATION.md); for
caller-facing semantics, see [API.md](API.md).

Exact model backend not verifiable from final document text alone.
