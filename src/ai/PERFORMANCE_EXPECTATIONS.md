# PERFORMANCE_EXPECTATIONS - src/ai

## Scope

- Module: src/ai
- This file defines measurable AI module performance expectations for release gating.

## Benchmark Reference

- Dedicated benchmark coverage now exists for AI generation path behavior.
- Relevant benchmark files:
  - benchmarks/bench_ai_plugin_generator.cpp
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AI-1 | prompt-validation-path overhead remains within release baseline budget | BM_AIPluginGeneratorValidatePrompt |
| AI-2 | generation-orchestration error-path overhead remains bounded | BM_AIPluginGeneratorErrorPathMalformedJson |
| AI-3 | concurrent lookup/scan pressure does not exceed release threshold | BM_ConcurrentQueries, BM_ConcurrentScans, BM_ConcurrentGetAllPlugins |
| AI-4 | generation success-path orchestration overhead remains bounded | BM_AIPluginGeneratorGeneratePlugin |
| AI-5 | memory overhead for plugin lifecycle path remains bounded | BM_MemoryOverhead, BENCHMARK_F(HotPlugBenchmarkFixture, MonitorMemoryFootprint) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | proxy lifecycle path p99 <= release threshold | p99 from mapped plugin_system and plugin_hot_plug cases |
| AG-3 | no mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Dedicated AI benchmark mapping is now available for prompt validation and generation-path orchestration.
- Wave C benchmark-style acceptance checks (`CAI-BENCH-01`, `FEDERATED-BENCH-01`) are tracked in test coverage for issue scope, but they are not currently part of the production benchmark gate manifest.

## Wave C Acceptance Check Mapping (Issue Scope `#5040`)

| Scope ID | Expectation | Evidence |
|---|---|---|
| C1-AC-1 | safety score alignment >= 0.80 with human annotators | `tests/test_cai_safety_module.cpp` (`CAI-BENCH-01`) |
| C1-AC-2 | latency overhead <= 2.0 s per response | `tests/test_cai_safety_module.cpp` (`CAI-BENCH-01`) |
| C1-AC-3 | false-positive rate <= 10% | `tests/test_cai_safety_module.cpp` (`CAI-BENCH-01`) |
| C2-AC-1 | convergence >= 95% of centralized baseline | `tests/test_federated_privacy_training.cpp` (`FEDERATED-BENCH-01`) |
| C2-AC-2 | gradient communication overhead <= 2.0 s per round | `tests/test_federated_privacy_training.cpp` (`FEDERATED-10`, `FEDERATED-BENCH-01`) |
| C2-AC-3 | configurable epsilon-differential privacy budget | `tests/test_federated_privacy_training.cpp` (DP budget assertions in FEDERATED suite) |

## Planning Traceability

- Wave C strategic planning issue: `#5040`
- Dependency planning issues: Wave A `#5038`, Wave B `#5039`

## Sourcecode Verification (Module: ai/performance)

- Verified benchmark sources:
  - benchmarks/bench_ai_plugin_generator.cpp
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
- Verified mapping surfaces:
  - lifecycle and manifest handling benchmarks
  - concurrent access benchmarks
  - hot-plug monitoring and memory-footprint benchmarks
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.