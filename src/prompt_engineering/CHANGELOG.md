<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Prompt Engineering Module

All notable changes to the Prompt Engineering module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Typed DSL for structured prompt authoring (Phase 2)
- Token counting and context-window budget manager
- Context-window budget enforcement (caller-side guard)
- Chain-of-thought execution tracer
- Prompt regression suite for detecting quality regressions across model updates

## [1.5.0] — 2026-03-23

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
