# PERFORMANCE_EXPECTATIONS - src/ai

## Scope

- Module: src/ai
- This file defines measurable AI module performance expectations for release gating.
- Scope includes AI plugin generation, orchestration, and experimental vector-/KI-near storage and retrieval paths.
- Experimental logarithmic vector representations are evaluated only for vector and AI workloads; they do not redefine global database numeric semantics.

## Benchmark Reference

- Dedicated benchmark coverage now exists for AI generation path behavior.
- Relevant benchmark files:
  - benchmarks/bench_ai_plugin_generator.cpp
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
- Planned benchmark expansion for logarithmic vector evaluation should include dedicated AI/vector retrieval and storage comparisons against canonical numeric baselines.

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AI-1 | prompt-validation-path overhead remains within release baseline budget | BM_AIPluginGeneratorValidatePrompt |
| AI-2 | generation-orchestration error-path overhead remains bounded | BM_AIPluginGeneratorErrorPathMalformedJson |
| AI-3 | concurrent lookup/scan pressure does not exceed release threshold | BM_ConcurrentQueries, BM_ConcurrentScans, BM_ConcurrentGetAllPlugins |
| AI-4 | generation success-path orchestration overhead remains bounded | BM_AIPluginGeneratorGeneratePlugin |
| AI-5 | memory overhead for plugin lifecycle path remains bounded | BM_MemoryOverhead, BENCHMARK_F(HotPlugBenchmarkFixture, MonitorMemoryFootprint) |
| AI-6 | experimental logarithmic vector storage must demonstrate bounded retrieval overhead versus canonical vector baseline before promotion | planned dedicated vector retrieval benchmark mapping |
| AI-7 | experimental logarithmic vector storage must demonstrate measurable memory-footprint benefit or cache-locality benefit versus canonical vector baseline | planned dedicated vector storage benchmark mapping |
| AI-8 | similarity/ranking quality under logarithmic vector representation must stay within documented acceptance thresholds before any default enablement | planned benchmark + quality evaluation harness |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | proxy lifecycle path p99 <= release threshold | p99 from mapped plugin_system and plugin_hot_plug cases |
| AG-3 | no mapped benchmark case missing in release run | benchmark run manifest completeness |
| AG-4 | experimental logarithmic vector path cannot be promoted without benchmark-backed comparison against Float32/Float16/BFloat16 and, where available, Int8 or other compact baselines | benchmark evidence completeness |
| AG-5 | no default enablement of logarithmic vector path unless accuracy and latency acceptance criteria are both satisfied | paired performance + quality gate |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Dedicated AI benchmark mapping is now available for prompt validation and generation-path orchestration.
- Wave C benchmark-style acceptance checks (`CAI-BENCH-01`, `FEDERATED-BENCH-01`) are tracked in test coverage for issue scope, but they are not currently part of the production benchmark gate manifest.
- Experimental logarithmic vector evaluation must be benchmark-driven and compared against canonical representations rather than assessed in isolation.
- Experimental logarithmic vector paths remain opt-in until benchmark, memory, latency, and ranking-quality evidence supports broader adoption.

## Experimental Logarithmic Vector Evaluation

The AI module will experimentally investigate whether logarithmic numeric representations improve vector- and KI-near workloads in ThemisDB.

### Evaluation Scope

- embedding storage
- similarity search
- retrieval / RAG-near paths
- candidate generation and scoring support paths

### Required Baselines

Experimental results should be compared against at least:

- Float32
- Float16
- BFloat16
- Int8 where implemented
- other compact vector baselines where available

### Required Metrics

At minimum, the evaluation should capture:

- Recall@K
- ranking quality / score drift
- memory footprint
- query latency (p50 / p95 / p99)
- throughput
- ingest / materialization overhead
- rebuild cost where applicable

### Promotion Rule

The logarithmic vector path must remain experimental unless it shows a clear benchmark-backed advantage in at least one of:

- memory efficiency
- cache locality
- retrieval latency
- throughput

while maintaining acceptable retrieval and ranking quality.

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
- Architecture proposal: `docs/architecture/experimental-logarithmic-vector-storage.md`

## Sourcecode Verification (Module: ai/performance)

- Verified benchmark sources:
  - benchmarks/bench_ai_plugin_generator.cpp
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
- Verified mapping surfaces:
  - lifecycle and manifest handling benchmarks
  - concurrent access benchmarks
  - hot-plug monitoring and memory-footprint benchmarks
  - experimental logarithmic vector work currently tracked as planned benchmark expansion
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.
  - Experimental logarithmic vector evaluation requires dedicated benchmark additions before hard release-gate enforcement can expand to those paths.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
