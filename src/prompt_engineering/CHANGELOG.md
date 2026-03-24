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

## [2.0.0] — 2026-03-24

### Added
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
