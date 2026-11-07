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

## External research and applicability

The following sources inform design choices; none is evidence that this
implementation already meets their results or guarantees.

| Source | Finding relevant to execution | Applicability and limitation |
|---|---|---|
| Blumofe and Leiserson, “Scheduling Multithreaded Computations by Work Stealing,” *Journal of the ACM* 46(5), 1999, [DOI: 10.1145/324133.324234](https://doi.org/10.1145/324133.324234) ([research record](../../research/papers/blumofe_leiserson_work_stealing_1999.md)) | Work stealing has provable expected-time bounds for fully strict, dependency-structured computations and can reduce scheduling overhead compared with work sharing in that model. | A database task queue is not automatically a fully strict computation. This supports evaluating stealing under skewed task distributions, not assuming a speedup or importing the theorem as a runtime guarantee. |
| Wu et al., “Improving DBMS Scheduling Decisions with Fine-grained Performance Prediction on Concurrent Queries,” 2025 preprint, [arXiv:2501.16256](https://arxiv.org/abs/2501.16256) ([research record](../../research/papers/wu_concurrent_query_scheduling_2025.md)) | Concurrent-query scheduling decisions can benefit from accounting for runtime variation across system states and query mixes; reported end-to-end improvements are specific to their evaluated PostgreSQL and Redshift workloads. | This module has no execution-time predictor, query-mix model, or integrated request path. Treat the work as motivation to measure representative mixes and tail latency before considering adaptive policies; do not transfer the reported percentages as targets. |
| Transaction Processing Performance Council, [TPC-H](https://www.tpc.org/tpch/) and [current specifications](https://www.tpc.org/tpc_documents_current_versions/current_specifications5.asp?mode=TPC-MEMBER) ([research record](../../research/best_practices/tpc_h_benchmark_methodology.md)) | A decision-support workload should exercise complex queries, varied data volumes, and both single-stream response and concurrent throughput. The current TPC-H specification is 3.0.1. | Use representative workload shapes and concurrency as test-design inputs. An execution-primitive microbenchmark is not a TPC-H result and must not be presented as one. |
| Google, *Site Reliability Engineering*, [Handling Overload](https://sre.google/sre-book/handling-overload/) ([research record](../../research/best_practices/sre_handling_overload.md)) | Reliable services make overload behavior explicit, bound work, and use appropriate shedding/throttling rather than allowing overload to cascade. | The module has bounded queues, timeout rejection, and LOW shedding, but no host-level feedback or export surface. Service integration and retry behavior remain caller responsibilities. |

### Source-to-module gap analysis

| Area | Current source evidence | Gap and planned response |
|---|---|---|
| Performance and scale | `QueryScheduler` uses a mutex-protected deadline priority queue; the pool uses one bounded shared dispatch queue. The dedicated benchmark models different local queues. | No production-runtime latency/throughput baseline, realistic mixed workload, or verified steal-path data. v2.5.0 establishes runtime baselines; v3.0.0 evaluates stealing and scaling against those baselines. |
| Use and integration | `README.md` documents the scheduler and pool as composable primitives; the server constructs and tears them down but does not route requests through them. | End-to-end user benefit and caller retry/backpressure behavior are unproven. Keep v2.5.0 focused on measurable contracts; gate v3.0.0 integration on an approved consumer and service-level tests. |
| Security and resilience | Queue capacities and admission timeouts are bounded; shutdown rejects new submissions and task exceptions are counted. There is no queued-work cancellation, expiry, or interruption of running callables. | Validate saturation, shutdown, task failure, and bounded resource use before adding features. Define any cancellation/expiry contract explicitly; never imply that an SLA deadline interrupts or expires work. |
| Governance and maintainability | Public headers document current behavior and reserved fields; module tests cover scheduler behavior and pool integration. | The `WorkStealingThreadPool` name and unused runtime-scaling configuration can mislead consumers. Resolve the contract before enabling the reserved behavior; retain source-to-test and documentation checks per wave. |
| Architecture and operations | The module exposes in-memory snapshots; no built-in telemetry exporter or active request-path integration exists. | No operator-visible queue-wait, rejection, or dispatch signals. Establish the metric contract in v2.5.0 and implement a host-owned export/integration only after ownership and lifecycle are agreed. |

## Reproducible validation

From the repository root, run the three registered focused execution CTest
cases and the separately registered `test_resource_pooling` integration test
using the exact filters in [README.md](README.md). The high-cardinality source
is not registered as a CTest target and, like the benchmark, uses local models;
neither validates production capacity or performance. Record the build
configuration, hardware, runtime test result, and production benchmark baseline
together; do not compare results across different configurations as if they
were equivalent.

External retrieval note: web-search metadata for the cited works was available
for this review, but direct web-page retrieval was blocked in the execution
environment. Verify bibliographic details and current benchmark specification
before using this note as a formal literature review.

## Related contracts

- [MODELING.md](MODELING.md) describes the queue and deadline model.
- [IMPLEMENTATION.md](IMPLEMENTATION.md) maps that model to source.
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) names the benchmark
  gates and their current interpretation.
- [ROADMAP.md](ROADMAP.md) defines the proposed versioned waves and acceptance KPIs.

Exact model backend not verifiable from final document text alone.
