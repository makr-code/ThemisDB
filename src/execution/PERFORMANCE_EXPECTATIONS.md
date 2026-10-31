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

# Performance Expectations — Execution Module

## Scope

This document records the current performance evidence boundary for bounded
scheduler admission/dequeue and fixed-worker dispatch. No production-runtime
baseline has been verified for this module.

## Benchmark Reference

Relevant benchmark source:
- `benchmarks/execution/bench_execution_dedicated_gates.cpp`

## Benchmark Harness Scope

| Case | Harness behavior | Production-runtime evidence |
|---|---|---|
| `EX-BM-01/EnqueueP95` | Times enqueue calls against a local `BoundedQueue` model | Does not exercise `QueryScheduler` or compute a production p95 |
| `EX-BM-02/DequeueP95` | Times dequeue calls against a local `BoundedQueue` model | Does not exercise `QueryScheduler` or compute a production p95 |
| `EX-BM-03/WorkStealThroughput` | Drains a local `StealPool` model | Does not exercise `WorkStealingThreadPool` |
| `EX-BM-04/ConcurrentDispatchThroughput` | Eight producer threads enqueue into a local `BoundedQueue` | Does not exercise the production worker pool |

The case names and source comments describe aspirational gates, but the
benchmark implementation does not validate those thresholds against the
production scheduler or pool. Treat them as harness metadata, not release gates.

## Expectations Before Production Release Claims

- Establish and publish production-runtime scheduler latency and pool throughput
  baselines on named representative hardware and a release build.
- Report the measurement method and distribution (including percentile
  calculation) rather than inferring p95 from benchmark case names.
- For future stealing or elastic scaling, compare results against that baseline
  and document the measured regression or improvement.
- Keep stress-test completion evidence separate from latency/throughput claims.

## Validation

Relevant source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`.
Scheduler runtime tests are in `tests/execution/`; the broader
`tests/integration/test_resource_pooling.cpp` suite exercises the thread pool.
`tests/execution/test_execution_highcardinality_stress.cpp` is a local-model
simulation, not a production stress test. A successful run and a production
runtime baseline have not been attached in this checkout.
