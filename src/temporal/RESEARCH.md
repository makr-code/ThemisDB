---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Temporal research and evaluation

This page records the engineering assumptions and evidence used to evaluate
temporal query, interval, and lifecycle behavior. It is for maintainers
validating semantics or performance; it does not claim that a particular
algorithm is universally optimal.

## Questions and assumptions

The runtime models system time (when a version was recorded) separately from
valid time (when its fact applies). The core in-memory types are
`themisdb::temporal::Timestamp` in milliseconds since the Unix epoch and
half-open `TimeRange` intervals `[start, end)`. These assumptions affect point
queries, interval overlap, history scans, retention, and benchmark inputs.

The repository also has a distinct `themis::temporal` contract header,
`include/temporal/temporal_api_contract.h`, whose constants are named in
nanoseconds and whose snapshot example uses inclusive endpoints. Do not mix
those units or endpoint rules with the `themisdb::temporal` runtime types.
Treat the contract header and its focused tests as a separate contract surface
until an integration explicitly maps them.

## Evaluation approach and evidence

- Correctness tests cover system-versioned and bitemporal tables, interval
  indexing, retention, snapshots, query execution, CDC, and conflict handling
  under [`tests/temporal/`](../../tests/temporal/).
- `test_temporal_contract_hardening_focused.cpp` documents the contract-header
  test families. Its helpers are test-local models, not implementations of the
  runtime classes.
- `test_temporal_highcardinality_stress.cpp` contains stub-based versioned-write,
  conflict-resolution, and CDC examples; it does not exercise retention or the
  temporal runtime. The integration soak suite is
  [`test_temporal_store_soak.cpp`](../../tests/integration/test_temporal_store_soak.cpp).
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) maps mutation,
  as-of/valid-time query, and history cases to `benchmarks/temporal/bench_temporal_queries.cpp`.
  Its release gates specify at most a 10% regression against the configured
  baseline, a configured p99 threshold, and complete mapped-case manifests.
  The threshold value is supplied by the release benchmark configuration, not
  by this page.

Use the tests for behavioral evidence and the configured release benchmark
profile for performance evidence. A passing test run does not establish a
latency target, and a benchmark result does not establish semantic correctness.

## Trade-offs and limits

The simple temporal index and augmented interval tree have different lookup
costs; the interval tree header documents overlap queries as
`O(log n + k)`, where `k` is the result count. Query joins evaluate predicates
over scanned row pairs, so their cost depends on both input cardinality and the
predicate. Snapshot capture copies scanned table state, trading memory for an
isolated read view. Retention can remove historical versions, so policies must
be evaluated against recovery and audit requirements before enforcement.

The in-memory CDC replay log is bounded and loses older events on overflow.
Performance and completeness claims must identify which implementation,
workload, configuration, and test or benchmark evidence they refer to.

Exact model backend not verifiable from final document text alone.
