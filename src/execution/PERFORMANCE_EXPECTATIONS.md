**Author:** ThemisDB Contributors  
**Created:** 2026-09-21  
**Last Updated:** 2026-09-21  
**Status:** active

# Performance Expectations — Execution Module

## Scope

This document defines the measurable performance expectations for the current execution-module baseline: bounded scheduler admission/dequeue and bounded fixed-worker dispatch.

## Benchmark Reference

Relevant benchmark source:
- `benchmarks/execution/bench_execution_dedicated_gates.cpp`

## Current Benchmark Mapping

| Target ID | Expectation | Benchmark case |
|---|---|---|
| EX-1 | scheduler enqueue latency remains bounded for the in-memory queue baseline | `EX-BM-01/EnqueueP95` |
| EX-2 | scheduler dequeue latency remains bounded for the in-memory queue baseline | `EX-BM-02/DequeueP95` |
| EX-3 | shared-queue drain throughput remains bounded under fixed-worker contention | `EX-BM-03/WorkStealThroughput` |
| EX-4 | concurrent producer dispatch remains bounded for the fixed-worker central queue | `EX-BM-04/ConcurrentDispatchThroughput` |

## Hard Gates

| Gate ID | Expectation | Measurement |
|---|---|---|
| EG-1 | enqueue p95 stays within the current release baseline budget | benchmark aggregate for `EX-BM-01/EnqueueP95` |
| EG-2 | dequeue p95 stays within the current release baseline budget | benchmark aggregate for `EX-BM-02/DequeueP95` |
| EG-3 | drain/dispatch throughput does not regress beyond the accepted release baseline | benchmark aggregates for `EX-BM-03` and `EX-BM-04` |
| EG-4 | focused stress coverage continues to complete without queue-loss regressions | `tests/execution/test_execution_highcardinality_stress.cpp` |

## Interpretation Notes

- `EX-BM-03/WorkStealThroughput` currently measures drain throughput of the dedicated execution benchmark harness; it should **not** be presented as proof that the production thread pool already performs live cross-worker stealing.
- Release evidence must be tied to the current implementation contract: bounded central-queue dispatch with fixed worker count.
- Any future activation of elastic scaling or true steal paths requires new baselines before these targets remain authoritative.

## Validation

Expectations are met when the benchmark cases run in a release-like configuration and remain inside the tracked release baselines, and when the focused stress tests continue to drain all submitted work without loss.
