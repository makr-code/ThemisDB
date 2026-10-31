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

# Execution Research

## Purpose and scope

This note records the evidence and evaluation approach for the in-process
execution primitives. It is for maintainers deciding whether scheduling or
dispatch behavior is ready to change. The implementation and public headers,
not this evaluation summary, define the current contract.

## Problem and assumptions

The module addresses two local resource-control problems: admitting query work
to a bounded queue and dispatching accepted callables to a bounded worker pool.
It assumes callers provide a finite SLA duration and admission timeout, arrange
for an appropriate consumer of scheduler entries, and report completion when
they need SLA metrics. The pool assumes submitted callables are trusted
in-process code; it does not isolate or cancel them.

These are not distributed-scheduling or request-priority research results.
`SLAPriority` labels are used for counters and LOW shedding; dequeue order is
the absolute deadline followed by query id.

## Method and evidence

The primary evaluation method is source-to-test correspondence:

| Question | Evidence |
|---|---|
| Does scheduler admission block only up to the supplied timeout and respect queue capacity? | `tests/integration/test_load_balancing.cpp` |
| Is dequeue ordered by deadline, with deterministic ties? | `include/execution/query_scheduler.h`, `src/execution/query_scheduler.cpp`, and the focused scheduler tests |
| Does the pool execute tasks on a fixed worker set and reject when its bounded queue cannot accept work? | `tests/integration/test_resource_pooling.cpp` |
| Are exception containment and shutdown exercised, and is high-cardinality modeled? | `tests/integration/test_resource_pooling.cpp` exercises the pool; `tests/execution/test_execution_highcardinality_stress.cpp` uses local queue/pool models rather than production primitives |
| What does the dedicated performance harness measure? | `benchmarks/execution/bench_execution_dedicated_gates.cpp` and `PERFORMANCE_EXPECTATIONS.md` |

The benchmark harness measures local queue-model enqueue/dequeue and
dispatch/drain throughput. `EX-BM-03/WorkStealThroughput` is a benchmark case
name, not proof that the runtime performs work stealing. It does not provide a
production-runtime baseline; no numeric service-level threshold is asserted
here.

## Evaluation and limits

Use the focused scheduler tests and the thread-pool integration tests for
runtime behavior. The dedicated benchmark gates exercise local models only and
are not production performance evidence. Test deadlines are workload inputs,
not evidence that the scheduler enforces execution deadlines: expired entries
are not evicted, and the pool does not interrupt running work. Scheduler SLA
measurements depend on explicit `reportCompletion()` calls. Pool latency samples
measure callable runtime, not queue-wait latency, and are retained only in memory.

Current results apply only to the bounded scheduler and fixed-worker,
central-dispatch implementation. They do not establish fairness under
arbitrary mixed workloads, per-worker stealing, elastic scaling, cancellation,
or production observability.

## Reproducible validation

From the repository root, run the focused scheduler tests and
`tests/integration/test_resource_pooling.cpp` as documented in [README.md](README.md).
The high-cardinality test and benchmark can inform local queue-model changes,
but do not validate production capacity or performance. Record the build
configuration, hardware, runtime test result, and production benchmark baseline
together; do not compare results across different configurations as if they
were equivalent.

## Related contracts

- [MODELING.md](MODELING.md) describes the queue and deadline model.
- [IMPLEMENTATION.md](IMPLEMENTATION.md) maps that model to source.
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) names the benchmark
  gates and their current interpretation.

Exact model backend not verifiable from final document text alone.
