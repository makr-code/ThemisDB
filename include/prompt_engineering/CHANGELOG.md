<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Prompt Engineering Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/prompt_engineering/CHANGELOG.md`.

## [1.6.0] — 2026-02

### Added
- `self_improvement_orchestrator.h` — `ILLMProviderReflectionAdapter` provider-specific reflection API abstraction.

## [1.5.0] — 2026-01

### Added
- `self_improvement_orchestrator.h` — `ReflectionTuner` with four strategies: SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC.
- `prompt_engineering_metrics.h` — `PromptEngineeringMetrics` with 4 reflection counters and Prometheus export.

## [1.4.0] — 2025-10

### Added
- `prompt_injection_detector.h` — `PromptInjectionDetector` covering jailbreak, role-override, and indirect injection.
- `chain_of_thought.h` — `ChainOfThoughtBuilder` structured CoT construction.
- `rag_prompt_builder.h` — `RAGPromptBuilder` RAG-augmented prompt building with citation tracking.
- `system_prompt_manager.h` — `SystemPromptManager` system prompt lifecycle and persona management.
- Multi-modal prompt support in `PromptManager` (image, audio context tokens).

## [1.3.0] — 2025-07

### Added
- `prompt_version_control.h` — `PromptVersionControl` git-like branching, tagging, diff, and merge.

## [1.0.0] — 2025-01

### Added
- `prompt_manager.h` — `PromptManager` central template registry.
- `feedback_collector.h` — `FeedbackCollector` human/automatic feedback collection.
- `prompt_evaluator.h` — `PromptEvaluator` automated quality evaluation.
- `prompt_optimizer.h` — `PromptOptimizer` A/B-testing-driven optimization.
- `meta_prompt_generator.h` — `MetaPromptGenerator` meta-prompt generation.
- `prompt_template_validator.h` — `PromptTemplateValidator` schema validation.
- `prompt_performance_tracker.h` — `PromptPerformanceTracker` latency/cost/quality tracking.
- `context_window_manager.h` — `ContextWindowBudgetManager` token budget enforcement.
- `prompt_engineering_integration.h` — Full pipeline integration facade.
