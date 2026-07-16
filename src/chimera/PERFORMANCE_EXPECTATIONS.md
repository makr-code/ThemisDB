# PERFORMANCE_EXPECTATIONS - src/chimera

## Scope

- Module: src/chimera
- This file defines measurable chimera module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark file:
  - benchmarks/llm_bench.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CHI-1 | adapter request parsing path remains within release baseline budget | BM_OpenAICompatAdapter_ParseRequest |
| CHI-2 | adapter response building path remains within release baseline budget | BM_OpenAICompatAdapter_BuildResponse |
| CHI-3 | adapter parse/build roundtrip path remains bounded in release profile | BM_OpenAICompatAdapter_RoundTrip |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CIG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CIG-2 | adapter compatibility path p99 <= release threshold | p99 from mapped adapter benchmark cases |
| CIG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Current mapping is a compatibility proxy and should be replaced by dedicated chimera-native benchmark cases.

## Sourcecode Verification (Module: chimera/performance)

- Verified benchmark source:
  - benchmarks/llm_bench.cpp
- Verified mapping surfaces:
  - OpenAI compatibility adapter parse, build, and roundtrip benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.