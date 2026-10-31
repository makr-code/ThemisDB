**Author:** ThemisDB Contributors
**Created:** 2026-09-21
**Last Updated:** 2026-10-31
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

Exact model backend not verifiable from final document text alone.

# Execution Module

## Zweck

The execution module provides two bounded runtime primitives that higher-level components can compose for in-process execution control:

- `themis::execution::QueryScheduler` implements a thread-safe earliest-deadline-first queue with backpressure, low-priority shedding, and SLA-completion metrics.
- `themis::resource::WorkStealingThreadPool` executes submitted work on a fixed worker set behind a bounded central dispatch queue.

The current implementation is intentionally compact and deterministic. It does **not** yet implement elastic worker scaling or per-worker dequeue stealing. The per-worker queue objects are private implementation scaffolding, not part of the public API contract.

## Scope

In scope:
- bounded query admission with timeout-based backpressure
- deadline-ordered dequeue with FIFO tie-breaking for equal deadlines
- low-priority shedding once the configured shed threshold is reached
- fixed worker execution with bounded task queueing
- graceful shutdown and lightweight runtime metrics

Out of scope in the current implementation:
- dynamic priority promotion or fairness buckets
- cancellation of already-enqueued queries or tasks
- elastic worker creation/retirement during steady-state execution
- true per-worker deque population and active work stealing
- cross-node or distributed execution coordination

## Quickstart (Build/Run)

The execution module is built as part of the main ThemisDB library. It is enabled
by default; configure with `THEMIS_EXECUTION_MODULE=ON` to enable it explicitly:

```bash
cmake -S . -B build -DTHEMIS_EXECUTION_MODULE=ON
cmake --build build --target themisdb
```

`tests/execution/CMakeLists.txt` registers these three focused CTest cases:

```bash
ctest --test-dir build -R '^(test_execution_engine|test_execution_performance|test_query_compilation)_ExecutionFocusedTests$' --output-on-failure
```

The broader pool integration suite is registered separately as
`test_resource_pooling` by `tests/integration/CMakeLists.txt`:

```bash
ctest --test-dir build -R '^test_resource_pooling$' --output-on-failure
```

`tests/execution/test_execution_highcardinality_stress.cpp` is a local-model
simulation and is not registered by `tests/execution/CMakeLists.txt` as a CTest target.

The dedicated benchmark target can be run when benchmarks are enabled:

```bash
./build/benchmarks/execution/bench_execution_dedicated_gates
```

## API/CLI Einstieg

The module exposes two public headers:

**`include/execution/query_scheduler.h`** — `themis::execution::QueryScheduler`
```cpp
// Enqueue work with a 500 ms SLA and 100 ms admission timeout
auto id = scheduler.enqueue(my_task, SLAPriority::MEDIUM, 500, "example",
                            std::chrono::milliseconds{100});
// Only dequeue after successful admission so a rejected submission does not
// consume an unrelated entry already in the queue.
if (id != 0) {
    QueryEntry entry;
    if (scheduler.dequeue(entry, std::chrono::milliseconds{200})) {
        entry.execute();
        scheduler.reportCompletion(entry.id);
    }
}
```

**`include/execution/thread_pool_manager.h`** — `themis::resource::WorkStealingThreadPool`
```cpp
// Submit a task with a 100 ms queue-wait timeout
bool accepted = pool.submit([]{ /* work */ }, "example",
                            std::chrono::milliseconds{100});
// Wait for the pending dispatch queue to empty; a worker may still be executing
// an item it has already popped.
pool.waitAll(std::chrono::milliseconds{5000});
// Retrieve runtime statistics
auto stats = pool.statistics();
```

## Integrationsueberblick

The server currently owns only primitive construction and teardown. The
scheduler-to-pool handoff below is optional host-consumer composition, not a live
`HttpServer` request path:

```mermaid
flowchart LR
    Server["HttpServer\n(conditional construction/teardown)"] -.-> QS["QueryScheduler"]
    Server -.-> TP["WorkStealingThreadPool"]
    Caller["Consumer / integration layer"] -->|"enqueue / dequeue"| QS
    Caller -->|"submit"| TP
    QS -.->|"optional consumer-owned handoff"| TP
    IT["Scheduler-focused tests"] -->|"direct API"| QS
    POOL_TEST["Thread-pool integration tests"] -->|"direct API"| TP
    MODEL["Stress / benchmark queue models\n(no production runtime path)"]
```

**Kurzinterpretation:** When enabled, `HttpServer` constructs and tears down both
primitives. It does not currently submit request work through them. A separate
consumer must implement any scheduler-to-pool handoff. Scheduler-focused tests
exercise `QueryScheduler`, while the broader integration suite covers
`WorkStealingThreadPool`. The stress test and benchmark use local models, not
production runtime code (see [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)).

## Runtime Behavior and Limits

### QueryScheduler
- `enqueue()` returns `0` when the scheduler is shutting down, when capacity does not free up before the caller timeout, or when a LOW-priority entry is shed at/above `Config::shed_threshold`.
- Dequeue order is driven by the computed absolute deadline first and by monotonically increasing query id second; the `SLAPriority` label is tracked for metrics and shedding decisions, not for a separate priority bucket implementation.
- `Config::urgent_window_ms` and `Config::default_sla_ms` remain reserved compatibility fields in the current implementation and are not consulted by `enqueue()` or `dequeue()`.
- SLA compliance metrics are only meaningful when callers invoke `reportCompletion()` for executed query ids.

### WorkStealingThreadPool
- The pool starts `Config::min_threads` workers during construction and keeps them alive until shutdown.
- `Config::max_threads` currently bounds internal queue allocation and clamps `min_threads`; it does not trigger elastic worker growth at runtime.
- `submit()` blocks until queue capacity is available or the caller timeout expires.
- `waitAll()` polls for an empty dispatch queue and zero pending-item count; it does not guarantee that completed tasks reported their own higher-level side effects.
- Worker task exceptions are caught, counted in `Statistics::failed`, and do not terminate the pool.

## Integration Notes

- `src/server/http_server.cpp` and `include/server/http_server.h` conditionally instantiate both execution primitives when `THEMIS_EXECUTION_MODULE` is enabled. Request-level submission and the scheduler-to-pool handoff are not wired there.
- Scheduler and thread-pool shutdown are coordinated during `HttpServer` teardown.
- The current integration initializes the primitives and exposes them to the server, but request-level query submission/wiring remains higher-level integration work.

## Known Limitations

1. The scheduler does not remove or cancel already-enqueued entries after SLA expiry; deadlines influence ordering and completion accounting, not automatic eviction.
2. The thread pool uses a shared dispatch queue only; the reserved per-thread queue objects are not populated by `submit()` yet.
3. `idle_timeout_ms` acts as the worker wait interval before re-checking the queue, not as an idle-thread retirement mechanism.
4. Runtime metrics are in-memory snapshots only; there is no built-in exporter, tracing hook, or persistence layer in this module.

## Verweise

| Interface / File | Role |
|---|---|
| `include/execution/query_scheduler.h` | Public API for deadline-ordered query admission, dequeue, and SLA metrics |
| `src/execution/query_scheduler.cpp` | Scheduler implementation with queue-depth backpressure and low-priority shedding |
| `include/execution/thread_pool_manager.h` | Public API for bounded task submission, drain/wait, statistics, and shutdown |
| `src/execution/thread_pool_manager.cpp` | Fixed-size worker pool implementation backed by a central dispatch queue |
| `tests/execution/test_execution_engine.cpp` | Scheduler execution, ordering, and completion behavior |
| `tests/execution/test_execution_performance.cpp` | Focused scheduler throughput and metric regressions |
| `tests/execution/test_query_compilation.cpp` | Scheduler id, ordering, shutdown, and timeout behavior |
| `tests/integration/test_load_balancing.cpp` | Broader integration suite that includes scheduler behavior and metrics coverage |
| `tests/integration/test_resource_pooling.cpp` | Broader integration suite that includes thread-pool behavior, shutdown, and exception coverage |
| `tests/execution/test_execution_highcardinality_stress.cpp` | Stress simulation using local queue and pool models; not production-runtime evidence |
| `benchmarks/execution/bench_execution_dedicated_gates.cpp` | Benchmark harness using local queue models; not a production-runtime gate |

- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component boundaries, state flow, and failure paths
- [`FEATURES.md`](FEATURES.md) — capability summary and limitations
- [`MODULE_GUIDE.md`](MODULE_GUIDE.md) — index to developer documentation and source ownership
- [`ROADMAP.md`](ROADMAP.md) — delivery status, remaining gaps, and phased plan
- [`CHANGELOG.md`](CHANGELOG.md) — implementation and documentation history
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — planned feature upgrades from the current bounded baseline
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) — benchmark-linked latency and throughput expectations
- [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) — deployment-time configuration and operational minimums
- [`SECURITY.md`](SECURITY.md) — fail-closed and resource-exhaustion security posture
- [`AUDIT.md`](AUDIT.md) — source-verified audit summary for the current execution module state
