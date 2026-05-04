> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Prompt Engineering Module

All notable changes to the Prompt Engineering module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Per-language prompt template variants (i18n support)
- Reinforcement learning from human feedback (RLHF) integration
- Cross-model prompt portability scoring

## [2.0.0] — 2026-03-23

### Added
- `PromptLibraryIO` — import/export of prompt template libraries to JSON and YAML
  for cross-environment portability (Phase 5, item 4).  Files:
  `include/prompt_engineering/prompt_library_io.h` +
  `src/prompt_engineering/prompt_library_io.cpp`.
- `PromptLibraryBundle` — self-contained snapshot: `name`, `description`,
  `version`, `format_version`, `created_at`, `checksum`,
  `templates`; `toJson()` / `fromJson()`.
- `ExportFormat` — `JSON` or `YAML`.
- `ImportResult` — `success`, `templates_loaded`, `error_message`,
  `checksum_valid`.
- `ExportResult` — `success`, `templates_written`, `error_message`.
- **FNV-1a 64-bit checksum** over sorted template JSON for bundle integrity; stored as
  16-character lowercase hex.
- `exportToJson()` / `exportToYaml()` — auto-compute checksum when
  `bundle.checksum` is empty.
- `exportToFile(bundle, path, fmt)` — format auto-detected from `.yaml`/`.yml`
  extension.
- `importFromJson()` / `importFromYaml()` / `importFromFile()` — all parse-safe
  (return `nullopt` / `success=false` on error).
- `verifyChecksum()` — `bundle.checksum == computeChecksum(bundle)`.
- 30 focused unit tests in `tests/test_prompt_library_io.cpp`
  (AC-1 through AC-30).
- CI: `.github/workflows/prompt-library-io-ci.yml` (GCC-12/14, Clang-15).
- German docs updated: `docs/de/prompt_engineering/README.md` item added.



### Added
- `PromptABExperimentFramework` — public A/B experiment framework for prompt template variants
  (Phase 5, item 3).  Files:
  `include/prompt_engineering/prompt_ab_experiment.h` +
  `src/prompt_engineering/prompt_ab_experiment.cpp`.
- `ExperimentVariant` — `CONTROL` / `TREATMENT` enum.
- `ExperimentContext` — `experiment_id` + `request_id` routing struct.
- `ExperimentStatus` — `RUNNING`, `WINNER_CONTROL`, `WINNER_TREATMENT`, `INCONCLUSIVE`, `COMPLETED`.
- `PromptExperiment` — experiment descriptor: `template_id`, `control_version_id`,
  `treatment_version_id`, `split_pct` (0–100), `min_samples`, `confidence_level`.
  `toJson()` / `fromJson()`.
- `ExperimentOutcome` — single scored observation; `toJson()`.
- `ExperimentSummary` — snapshot: per-variant counts, mean scores, `delta_pct`,
  `p_value`, `significant`, `winner_version_id`; `toJson()`.
- **Deterministic variant assignment** via MurmurHash3-32 of `request_id`
  (seed `0x9747b28c`): `murmur3_32(request_id) % 100 < split_pct → TREATMENT`.
- **Welch two-sample t-test** with Welch-Satterthwaite degrees of freedom for
  significance detection; auto-promotes winner when both variants reach
  `min_samples` and `p < 1 − confidence_level`.
- `setWinnerCallback()` — exception-safe callback fired on auto-promotion.
- 30 focused unit tests in `tests/test_prompt_ab_experiment.cpp`
  (AC-1 through AC-30).
- CI: `.github/workflows/prompt-ab-experiment-ci.yml` (GCC-12/14, Clang-15).
- German docs updated: `docs/de/prompt_engineering/README.md` +
  `MISSING_IMPLEMENTATIONS.md` item #4 resolved.



### Added
- `PromptRegressionRunner` — automated regression harness around `PromptEvaluator`;
  compares candidate vs. baseline outputs on golden-set fixtures; gates publish via
  `RegressionResult::blocked`.  Files:
  `include/prompt_engineering/prompt_regression_runner.h` +
  `src/prompt_engineering/prompt_regression_runner.cpp`.
- `RegressionFixture` — evaluation pair: `template_id`, `prompt_text`,
  `expected_output`, `source` (`"golden"` / `"feedback"`), `baseline_score`.
  `toJson()` / `fromJson()`.
- `RegressionConfig` — `max_regression_pct` (5 %), `min_fixtures`, `confidence_level`,
  `block_on_regression`.
- `RegressionResult` — `fixture_count`, `mean_candidate_score`, `mean_baseline_score`,
  `delta_pct`, `is_regression`, `blocked`, `inconclusive`, `statistically_significant`,
  `fixture_deltas`, `toJson()`.
- `FixtureDelta` — per-fixture score breakdown: index, template_id, baseline_score,
  candidate_score, delta.
- `PromptRegressionRunner::loadFeedbackFixtures()` — imports `USER_POSITIVE` entries
  from `FeedbackCollector` as regression fixtures.
- `PromptRegressionRunner::setLogCallback()` — structured JSON log event per run; log
  exceptions are suppressed so the runner is never interrupted.
- 30 focused unit tests in `tests/test_prompt_regression_runner.cpp` (AC-1 through AC-30).
- CI: `.github/workflows/prompt-regression-runner-ci.yml` (GCC-12/14, Clang-15).
- German docs updated: `docs/de/prompt_engineering/README.md` + `MISSING_IMPLEMENTATIONS.md`
  item #4 resolved.

### Added
- `IChainOfThoughtTracer` — pluggable per-step tracing interface; both
  `onStepBegin(StepId, label)` and `onStepEnd(StepId, content, duration)` are
  `noexcept`.  Stored in `include/prompt_engineering/cot_tracer.h` +
  `src/prompt_engineering/cot_tracer.cpp`.
- `CoTSpanRecord` — immutable span value type: step_index, label, content,
  token_count (chars/4 BPE approx), duration, start_time; `toJson()`.
- `RecordingCoTTracer` — concrete tracer for testing / offline analysis; thread-safe;
  `spans()`, `spanCount()`, `hasSpans()`, `reset()`, `toJson()`.
- `CoTTraceCollector` — fan-out tracer; forwards to N registered children;
  `addTracer()`, `removeTracer()`, `tracerCount()`, `totalStepsTraced()`,
  `spanCount()`, `reset()`, `toJson()`.
- `ChainOfThoughtBuilder::attachTracer()` / `detachTracer()` / `hasTracer()`.
- `ChainOfThoughtBuilder::build()` — fires `onStepBegin` / `onStepEnd` per step
  when a tracer is attached; tracer exceptions are caught and suppressed so the
  prompt-construction hot path is never interrupted.
- 30 focused unit tests in `tests/test_cot_tracer.cpp` (AC-1 through AC-30).
- CI: `.github/workflows/cot-tracer-ci.yml` (GCC-12/14, Clang-15).
- German docs updated: `docs/de/prompt_engineering/README.md` (CoT Tracer
  components); `MISSING_IMPLEMENTATIONS.md` (item #3 resolved).

### Added
- `ILLMProviderReflectionAdapter` — adapter bridging any `ILLMProvider` to `IReflectionProvider`;
  uses `DynamicReflectionPromptBuilder` for prompt construction; pluggable `IReflectionScorer`
  with built-in heuristic fallback.  Stored in `include/prompt_engineering/llm_reflection_adapter.h`
  + `src/prompt_engineering/llm_reflection_adapter.cpp`.
- Reflection Tuning metrics — 4 new counters in `PromptEngineeringMetrics`:
  `recordReflectionCycleStart`, `recordReflectionCycleComplete`, `recordReflectionGuardFired`,
  `recordReflectionQualityDelta`; all exported via `exportMetrics()`, persisted via
  `snapshotToJson()` / `restoreFromJson()`, and reset by `reset()`.
- `IntegrationConfig::enable_reflection_tuning` (default: `false`) + `reflection_max_iterations`
  (default: 3) — opt-in reflection pass in `PromptEngineeringIntegration::afterExecution()`.
- `PromptEngineeringIntegration::setReflectionTuner()` + `setMetrics()` — inject a
  `ReflectionTuner` and `PromptEngineeringMetrics` instance for observability.
- 28 focused unit tests in `tests/test_reflection_integration.cpp` (AC-1 through AC-28).
- CI: `.github/workflows/reflection-integration-ci.yml` (GCC-12/14, Clang-15).
- German docs updated: `docs/de/prompt_engineering/README.md` (new components table, architecture
  diagram) and `MISSING_IMPLEMENTATIONS.md` (items 2 + 7 resolved).

### Added
- `ReflectionTuner` — iterative self-critique and revision cycle for LLM responses,
  implementing four strategies: `SELF_REFINE` (Madaan et al., NeurIPS 2023),
  `REFLEXION` (Shinn et al., NeurIPS 2023), `CONSTITUTIONAL` (Bai et al., Anthropic 2022),
  and `SOCRATIC` (Socratic questioning).
- `IReflectionProvider` — pluggable LLM backend interface with `generate`, `critique`,
  `revise`, and `score` methods; fallback template-and-heuristic mode when no provider
  is attached.
- `DynamicReflectionPromptBuilder` — generates strategy-specific, self-aware critique
  and revision prompts; injects `SelfAwareContext` into prompts dynamically.
- `SelfAwareContext` — extracts the model's self-reported confidence and uncertainty
  from response text via linguistic marker analysis; drives adaptive critique prompts.
- `ReflectionHallucinationGuard` — detects hallucination signals (marker scan) and
  quality divergence (rolling-average trajectory analysis); halts the reflection cycle
  before errors compound (mitigates risk documented in Golem.de, 2026-03).
- `ReflectionConfig` — full configuration: strategy, max iterations, convergence
  threshold, plateau detection, guard thresholds, constitutional principles, and
  self-aware context toggle.
- `ReflectionResult` — complete result with per-step trace, quality trajectory,
  convergence/guard flags, and `toJson()` for observability.
- 38 unit tests in `tests/test_reflection_tuner.cpp` (AC-1 through AC-20).
- CI: `.github/workflows/reflection-tuner-ci.yml` (multi-platform, AC-1–AC-20).

### Added (2026-03-24)
- `TreeOfThoughtsBuilder` – multi-path reasoning with BFS, DFS, and BEAM search strategies; pluggable `IToTThoughtGenerator` and `IToTEvaluator` interfaces; depth-bounded pruning; `ToTConfig`, `ToTNode`, `ToTResult` data types; `TreeOfThoughtsBuilder::buildThoughtPrompt()`, `buildEvaluationPrompt()`, `buildSynthesisPrompt()` static helpers (30 unit tests; CI: `tree-of-thoughts-ci.yml`)
- `ProTeGiOptimizer` – automatic prompt optimisation via natural-language ("textual") gradients; `IProTeGiLLMProvider` interface for critique and candidate generation; `ProTeGiConfig`, `ProTeGiGradient`, `ProTeGiResult` types; mini-batch error sampling, beam search, early-stop and convergence-stop conditions (18 unit tests; CI: `protegi-optimizer-ci.yml`)
- `DspySignature` – typed input/output contract declaration with `buildPrompt()` and `parseResponse()`; `DspyField` with `DspyFieldType` (STRING/INT/FLOAT/BOOL/LIST/JSON); `DspyPredict` and `DspyChainOfThought` modules; `IDspyLLMProvider` interface; `EchoDspyLLMProvider` echo stub; `DspyMissingFieldError` exception (30 unit tests; CI: `dspy-module-ci.yml`)
- New focused CMake test targets: `TreeOfThoughtsFocusedTests`, `ProTeGiOptimizerFocusedTests`, `DspyModuleFocusedTests`
- All three source files registered in `THEMIS_LLM_SOURCES` (ModularBuild.cmake) and the top-level `themis_core` source list (cmake/CMakeLists.txt)

## [1.4.0] — 2026-03-12

### Added
- `PromptInjectionDetector` with 10 built-in detection patterns, keyword/syntax scoring, and `sanitize()` method
- `ChainOfThoughtBuilder` for structured multi-step reasoning prompt construction
- `RAGPromptBuilder` with budget-aware chunk selection and citation tracking
- `SystemPromptManager` for system-prompt lifecycle and versioning
- Multi-modal prompt support (text, image reference, structured data)
- `AlertConfig` threshold alerting on metric anomalies
- Integration facade (`prompt_engineering_integration.cpp`) providing a unified entry point

### Changed
- `SelfImprovementOrchestrator` wired to real `PromptEvaluator` (previously stub-connected)
- A/B testing p-value computation upgraded to erfc-based normal CDF for numerical accuracy
- Prometheus metrics now support crash-safe snapshot/restore on restart

### Fixed
- Outlier detection in `FeedbackCollector` emitting false positives under low sample counts
- Version control branch diff producing incorrect deltas on merge conflicts

## [1.3.0] — 2025-09-01

### Added
- Git-like version control for prompt templates (branches, commits, diffs) via `prompt_version_control.cpp`
- A/B testing framework with erfc-based statistical significance gates
- `PromptOptimizer` for automated template improvement based on evaluation scores
- `MetaPromptGenerator` with `ILLMProvider` interface for model-agnostic generation
- `SelfImprovementOrchestrator` coordinating evaluator + optimizer feedback loop

### Changed
- `PromptEvaluator` upgraded with Welch's t-test for statistical significance reporting
- Performance tracker (`prompt_performance_tracker.cpp`) now records per-template latency histograms

## [1.2.0] — 2025-03-01

### Added
- `FeedbackCollector` supporting 10 feedback types, aggregate statistics, Z-score outlier detection, and FNV-1a audit checksum on every entry
- `PromptEvaluator` with semantic similarity scoring and `IEmbeddingProvider` interface
- Prometheus metrics integration (`prompt_engineering_metrics.cpp`)
- YAML bulk-load support for batch-importing template libraries

### Fixed
- Thread-safety issue in template map during concurrent read/write (resolved via TBB `concurrent_hash_map`)

## [1.1.0] — 2024-08-01

### Added
- Template validation (`validateTemplate` + `ValidationResult`) with structural and placeholder checks
- Context injection via `{placeholder}` substitution (structural, not evaluated)
- RocksDB persistence backend for `PromptManager` (durable CRUD across restarts)
- Thread-safe `PromptManager` using TBB `concurrent_hash_map`

### Fixed
- `PromptManager` CRUD operations not persisting across process restart

## [1.0.0] — 2024-01-01

### Added
- Initial `PromptManager` with in-memory CRUD for LLM prompt templates
- Basic context injection via `{placeholder}` token substitution
- Foundation interfaces: `ILLMProvider`, `IEmbeddingProvider`
