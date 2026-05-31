# PERFORMANCE_EXPECTATIONS - src/ethics_ai

## Scope

- Module: src/ethics_ai
- This file defines measurable ethics_ai module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_ethics_ai_plugin.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| EAIP-1 | single-school decision path remains within release baseline budget | BM_DiscourseEngine_MakeDecisionSingleSchool |
| EAIP-2 | five-school decision path remains bounded under benchmark profile | BM_DiscourseEngine_MakeDecisionFiveSchools |
| EAIP-3 | context assembly and batch context paths remain bounded | BM_RAGContextEngine_BuildContext, BM_RAGContextEngine_BuildContextBatch10 |
| EAIP-4 | vector semantic lookup path remains bounded at configured input scale | BM_RAGContextEngine_VectorSemanticSearch512 |
| EAIP-5 | evaluator decision recording path remains bounded and stable | BM_EthicsEvaluator_RecordDecision |
| EAIP-6 | debate continuation path remains bounded under round progression | BM_DiscourseEngine_ContinueDebateRound |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| EAIG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| EAIG-2 | ethics hot-path p99 <= release threshold | p99 from mapped ethics_ai benchmark cases |
| EAIG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional ethics_ai benchmark scenarios are introduced.

## Sourcecode Verification (Module: ethics_ai/performance)

- Verified benchmark sources:
  - benchmarks/bench_ethics_ai_plugin.cpp
- Verified mapping surfaces:
  - discourse single/five-school decision benchmark cases
  - context build and vector semantic benchmark cases
  - evaluator record and debate continuation benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.