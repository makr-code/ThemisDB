# Performance Expectations — Document Module

<!-- Status: current | source/configuration inspection: 2026-11-19 -->

## Scope and evidence status

The module benchmark directory contains three suites over in-memory document interfaces and one separate synthetic serialization-gates executable. Threshold declarations are not benchmark results. No baseline output or release-gate execution was validated for this update.

## Source-backed cases

| Suite | Source | Exercise |
|---|---|---|
| Store | `benchmarks/document/bench_document_store.cpp` | In-memory `put/get/list/count/update/remove` and schema validation |
| Diff/merge | `benchmarks/document/bench_document_diff_merge.cpp` | `InMemoryDocumentDiffMerge` over `InMemoryDocumentStore` |
| Round-trip | `benchmarks/document/bench_document_round_trip.cpp` | `StoreBackedRoundTripEditor` over `InMemoryDocumentStore` |
| Serialization gates | `benchmarks/document/bench_document_serialization_gates.cpp` | Synthetic string/vector stubs; not production serialization, store, diff, or merge implementations |

The first three suites declare six `GATE-DOC-01..06` runtime checks. Their latency-named checks compare measured mean time to the configured threshold; the source does not calculate per-iteration p99. The serialization-gates suite emits four `DOC-BM-01..04` threshold counters but does not enforce those thresholds with a failing check.

## Threshold inventory

| Gate | Declared threshold | Source |
|---|---:|---|
| GATE-DOC-01 | mean diff time ≤ 500 µs | `bench_document_diff_merge.cpp` |
| GATE-DOC-02 | mean clean-merge time ≤ 200 µs | `bench_document_diff_merge.cpp` |
| GATE-DOC-03 | mean `beginRelay()` time ≤ 1,000 µs | `bench_document_round_trip.cpp` |
| GATE-DOC-04 | mean `loadInteraction()` time ≤ 200 µs | `bench_document_round_trip.cpp` |
| GATE-DOC-05 | `put()` throughput ≥ 100,000 ops/s | `bench_document_store.cpp` |
| GATE-DOC-06 | mean `get()` time ≤ 100 µs | `bench_document_store.cpp` |

TODO: establish representative-hardware baselines, select a statistically supported percentile method, and verify how the release workflow consumes these cases before claiming p95/p99 or release readiness.

## Release evidence gates

- Treat each listed threshold as a source-defined check only. A passing threshold check is not a representative-hardware baseline until the workload, build, host, and raw run output are retained.
- A tail-latency claim requires raw per-operation samples and a declared percentile estimator; current mean-based `GATE-DOC` checks cannot pass a p95/p99 claim.
- Before release sign-off, require at least five independent runs per selected workload, report run-to-run variation, and verify the consuming CI/release gate. Synthetic `DOC-BM-01..04` counters and unexecuted test registrations do not satisfy these gates.
- Correctness gates are separate: run the test-enabled focused document suite, including schema invalid-input and store-failure propagation cases. A benchmark result cannot substitute for correctness or integration/security review.

## Build registration

`benchmarks/document/CMakeLists.txt` registers four executable targets when benchmarks are enabled and Google Benchmark is found. See [DOCUMENT_BENCHMARK_COVERAGE.md](../../benchmarks/document/DOCUMENT_BENCHMARK_COVERAGE.md) and [RELEASE_GATES.md](../../benchmarks/document/RELEASE_GATES.md) for case details and declared threshold behavior.