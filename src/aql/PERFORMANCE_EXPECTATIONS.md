# PERFORMANCE_EXPECTATIONS - src/aql

<!-- Updated: 2026-07-20 — Phase 6 AQL Translation & Helper Path Benchmarks added -->

## Scope

- Module: src/aql
- This file defines measurable AQL module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/aql/bench_aql_functions.cpp
  - benchmarks/aql/bench_aql_mutations.cpp
  - benchmarks/aql/bench_aql_translation.cpp   ← Phase 6 (translation + validation pipeline)
  - benchmarks/aql/bench_aql_helper_paths.cpp  ← Phase 6 (scorer + few-shot + highlighter + tokens)
  - benchmarks/bench_hybrid_aql_sugar.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AQL-1 | core string and regex function paths remain within release baseline budget | BENCHMARK_F(AQLFunctionBenchmark, StringLength), BENCHMARK_F(AQLFunctionBenchmark, StringConcat), BENCHMARK_F(AQLFunctionBenchmark, StringRegexTest) |
| AQL-2 | math and aggregate function paths remain bounded | BENCHMARK_F(AQLFunctionBenchmark, MathSqrt), BENCHMARK_F(AQLFunctionBenchmark, MathTrigonometry), BENCHMARK_F(AQLFunctionBenchmark, MathAggregateSum) |
| AQL-3 | array function paths remain bounded | BENCHMARK_F(AQLFunctionBenchmark, ArrayFlatten), BENCHMARK_F(AQLFunctionBenchmark, ArrayUnique), BENCHMARK_F(AQLFunctionBenchmark, ArraySorted) |
| AQL-4 | geo/vector/graph function paths remain bounded | BENCHMARK_F(AQLFunctionBenchmark, GeoDistance), BENCHMARK_F(AQLFunctionBenchmark, VectorCosineSimilarity), BENCHMARK_F(AQLFunctionBenchmark, GraphShortestPath), BENCHMARK_F(AQLFunctionBenchmark, GraphPageRank) |
| AQL-5 | highlighter tokenization/annotation paths remain within release baseline budget | BM_Highlighter_Tokenize_Simple, BM_Highlighter_Tokenize_Complex, BM_Highlighter_AnnotateErrors_Valid, BM_Highlighter_AnnotateErrors_Malformed |
| AQL-6 | confidence scorer paths remain bounded | BM_ConfidenceScorer_NoSchema, BM_ConfidenceScorer_WithSchema, BM_ConfidenceScorer_Simple |
| AQL-7 | few-shot selection and prompt formatting remain bounded | BENCHMARK_F(FewShotFixture, FindRelevant_Top3), BENCHMARK_F(FewShotFixture, BuildPromptSection), BENCHMARK_F(FewShotFixture, FormatForPrompt) |
| AQL-8 | NL→AQL simple translation pipeline overhead p95 ≤ 2 ms (mock) | BM_AQLTranslationSimple |
| AQL-9 | NL→AQL complex translation pipeline overhead p95 ≤ 5 ms (mock) | BM_AQLTranslationComplex |
| AQL-10 | AQL validation p95 ≤ 200 µs (mock, single query) | BM_AQLValidationSimple |
| AQL-11 | AQL validation batch(32) throughput ≥ 100 000 queries/s | BM_AQLValidationBatch(32) |
| AQL-12 | Confidence scorer p95 ≤ 100 µs | BM_AQLConfidenceScorerSimple |
| AQL-13 | Few-shot retrieval k=3 p95 ≤ 500 µs | BM_AQLFewShotRetrieval(3) |
| AQL-14 | Token estimation for 20-turn history p95 ≤ 50 µs | BM_AQLTokenEstimation(20) |

## Phase 6 Translation/Validation Gate Table

| Operation | p50 Target | p95 Gate | p99 Target | Baseline Hardware |
|-----------|-----------|---------|-----------|-------------------|
| BM_AQLTranslationSimple | ≤ 500 µs | ≤ 2 ms | ≤ 5 ms | x86-64 ≥3 GHz, Release mode |
| BM_AQLTranslationComplex | ≤ 1 ms | ≤ 5 ms | ≤ 10 ms | x86-64 ≥3 GHz, Release mode |
| BM_AQLValidationSimple | ≤ 50 µs | ≤ 200 µs | ≤ 500 µs | x86-64 ≥3 GHz, Release mode |
| BM_AQLValidationBatch(32) | ≤ 1 ms | ≤ 3 ms | ≤ 8 ms | x86-64 ≥3 GHz, Release mode |
| BM_AQLConfidenceScorerSimple | ≤ 20 µs | ≤ 100 µs | ≤ 300 µs | x86-64 ≥3 GHz, Release mode |
| BM_AQLFewShotRetrieval(k=3) | ≤ 50 µs | ≤ 200 µs | ≤ 500 µs | x86-64 ≥3 GHz, Release mode |
| BM_AQLHighlighterSimple | ≤ 20 µs | ≤ 100 µs | ≤ 200 µs | x86-64 ≥3 GHz, Release mode |
| BM_AQLTokenEstimation(20 turns) | ≤ 15 µs | ≤ 50 µs | ≤ 100 µs | x86-64 ≥3 GHz, Release mode |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | highlighter/scorer/few-shot path p99 <= release threshold | p99 from mapped bench_hybrid_aql_sugar cases |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |
| AG-4 | NL→AQL simple translation p95 ≤ 2 ms (mock pipeline) | BM_AQLTranslationSimple p95 |
| AG-5 | AQL validation batch throughput ≥ 100 000 queries/s | BM_AQLValidationBatch(32) items/s |
| AG-6 | Token estimation p95 ≤ 50 µs for 20-turn history | BM_AQLTokenEstimation(20) p95 |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: aql/performance)

- Verified benchmark sources:
  - benchmarks/aql/bench_aql_functions.cpp
  - benchmarks/aql/bench_aql_mutations.cpp
  - benchmarks/aql/bench_aql_translation.cpp  ← Phase 6 (added 2026-07-20)
  - benchmarks/aql/bench_aql_helper_paths.cpp ← Phase 6 (added 2026-07-20)
  - benchmarks/bench_hybrid_aql_sugar.cpp
- Verified mapping surfaces:
  - function-library benchmark paths
  - highlighter/scorer benchmark paths
  - few-shot benchmark paths
  - translation + validation pipeline paths (Phase 6)
  - token estimation and helper paths (Phase 6)
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.