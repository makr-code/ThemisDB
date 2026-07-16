# PERFORMANCE_EXPECTATIONS - src/aql

## Scope

- Module: src/aql
- This file defines measurable AQL module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_aql_functions.cpp
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

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | highlighter/scorer/few-shot path p99 <= release threshold | p99 from mapped bench_hybrid_aql_sugar cases |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: aql/performance)

- Verified benchmark sources:
  - benchmarks/bench_aql_functions.cpp
  - benchmarks/bench_hybrid_aql_sugar.cpp
- Verified mapping surfaces:
  - function-library benchmark paths
  - highlighter/scorer benchmark paths
  - few-shot benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.