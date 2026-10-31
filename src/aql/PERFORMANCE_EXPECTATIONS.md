---
Author: "ThemisDB Team"
Created: "2026-07-20"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Performance Expectations

## Scope and evidence status

This page maps current benchmark source files, registered cases, and threshold
annotations. It does not report a current benchmark run. Previously recorded
latencies, variance, sanitizer/test status, and “locked/pass” claims are removed
from the active expectation table because the underlying run artifacts were not
validated in this documentation review.

## Benchmark sources and build registration

`benchmarks/aql/CMakeLists.txt` adds the following targets when
`THEMIS_BUILD_BENCHMARKS` is enabled:

- `bench_aql_functions`
- `bench_aql_mutations`
- `bench_aql_translation`
- `bench_aql_helper_paths`
- `bench_aql_assistance_gates`
- `bench_aql_consistency_performance_gates`

Additional AQL-related cases are registered from the parent benchmark
configuration, including `bench_aql_geo_filter` and
`bench_hybrid_aql_sugar`.

## Current case map

| Area | Source and registered case examples |
|---|---|
| AQL built-in functions | `bench_aql_functions.cpp`: `StringLength`, `StringConcat`, `StringRegexTest`, `MathSqrt`, `ArrayFlatten`, `GeoDistance`, `VectorCosineSimilarity`, `GraphShortestPath` |
| Mutations | `bench_aql_mutations.cpp`: `BM_ParseInsert`, `BM_ValidateInsert`, `BM_TranslateInsert`, `BM_ExecuteInsert`, `BM_ExecuteInsertBatch` |
| Translation and validation | `bench_aql_translation.cpp`: `BM_AQLTranslationSimple`, `BM_AQLTranslationComplex`, `BM_AQLValidationSimple`, `BM_AQLValidationBatch` (batch arguments 32 and 128); translation uses mock components |
| Helper paths | `bench_aql_helper_paths.cpp`: `BM_AQLConfidenceScorerSimple`, `BM_AQLFewShotRetrieval` (arguments 3 and 5), `BM_AQLHighlighterSimple`, `BM_AQLTokenEstimation` (arguments 5, 20, and 50); implementations use mock helper components |
| Consolidated gates | `bench_aql_assistance_gates.cpp`: `BM_AG4_TranslationSimple`, `BM_AG4_TranslationComplex`, `BM_AG5_ValidationBatch`, `BM_AG5_ValidationSingle`, `BM_AG6_TokenEstimation_20turns`, `BM_AG6_TokenEstimation_1turn` |
| Consistency gates | `bench_aql_consistency_performance_gates.cpp`: `BenchValidateAQLWithParser`, `BenchTranslateNLToAQL`, `BenchBridgeExecution`, `BenchFullPipeline` |
| Highlighter/scorer/few-shot | `bench_hybrid_aql_sugar.cpp`: tokenization, annotation, confidence scoring, example lookup and prompt formatting |
| Geo-filter operations | `bench_aql_geo_filter.cpp`: indexed and sequential `STWithin` cases |

The case names above were checked against benchmark source registrations; they
do not establish target build success or measured performance.

## Threshold annotations in source

The benchmark files contain stated thresholds, including:

- Translation mock cases: p95 ≤ 2 ms (simple) and ≤ 5 ms (complex).
- Validation mock case: p95 ≤ 200 µs; the batch source uses 32- and 128-query
  arguments.
- The assistance-gate source labels AG-4, AG-5, and AG-6 with thresholds for
  translation, validation throughput, and 20-turn token estimation. Its source
  comments contain historical baseline numbers, but this page does not certify
  those numbers or a current gate result.
- The consistency-gate source checks p99 limits for its four named mock
  operations; read that source for exact thresholds and implementation details.

These are source annotations/benchmark checks, not a current release decision.
Use benchmark output from the target build and hardware before making
performance claims. The mock translation/helper paths measure local component
overhead, not live provider latency, translation accuracy, or production
workload performance.

## Validation and follow-up

- Run the mapped benchmark targets in the intended release configuration before
  asserting a baseline or gate result.
- Record build revision, benchmark target/case, command/configuration, hardware,
  repetitions, and raw output alongside any new baseline.
- **TODO:** Re-establish and review an authoritative baseline manifest for the
  current cases; no run manifest was inspected in this documentation-only task.

## Canonical references

- [`benchmarks/aql/CMakeLists.txt`](../../benchmarks/aql/CMakeLists.txt)
- [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt)
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
- [`TESTING_COVERAGE.md`](TESTING_COVERAGE.md)
