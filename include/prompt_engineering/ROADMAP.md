<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Prompt Engineering Module Roadmap

## Current Status

v1.7.0 — production. Full prompt lifecycle (template, versioning, optimization, A/B testing, injection detection, RAG building, CoT, reflection, metrics) is operational.  v1.7.0 adds `IRAGContextBudgetManager` + `RagContextBudgetManager`, `IPromptQualityEvaluator` + `PromptQualityEvaluator`, and `IPromptABFramework` + `SimplePromptABFramework`.

## Completed

- [x] `PromptManager`, `FeedbackCollector`, `PromptEvaluator`, `PromptOptimizer`
- [x] `MetaPromptGenerator`, `PromptTemplateValidator`, `PromptPerformanceTracker`
- [x] `ContextWindowBudgetManager` with `PromptBudgetExceededError`
- [x] `PromptEngineeringIntegration` pipeline facade
- [x] `PromptVersionControl` git-like versioning
- [x] `PromptInjectionDetector` (jailbreak, role-override, indirect)
- [x] `ChainOfThoughtBuilder`, `RAGPromptBuilder`, `SystemPromptManager`
- [x] Multi-modal prompt support
- [x] `ReflectionTuner` (SELF_REFINE / REFLEXION / CONSTITUTIONAL / SOCRATIC)
- [x] `PromptEngineeringMetrics` 4 reflection counters
- [x] `ILLMProviderReflectionAdapter`
- [x] `IRAGContextBudgetManager` + `RagContextBudgetManager` — RAII `BudgetHandle`, `BudgetExhaustedError`, `BudgetSnapshot` (2026-04-19)
  (`include/prompt_engineering/rag_context_budget_manager.h` + `src/prompt_engineering/rag_context_budget_manager.cpp`. 8 tests RCB-01..08 in `tests/test_prompt_engineering_v190.cpp`.)
- [x] `IPromptQualityEvaluator` + `PromptQualityEvaluator` — injection/diversity/repetition checks, `QualityReport`, `QualityConfig` (2026-04-19)
  (`include/prompt_engineering/prompt_quality_evaluator.h` + `src/prompt_engineering/prompt_quality_evaluator.cpp`. 8 tests PQE-01..08.)
- [x] `IPromptABFramework` + `SimplePromptABFramework` — deterministic FNV-1a-32 variant assignment, `ABVariant`, `ExperimentDescriptor` (2026-04-19)
  (Added to `include/prompt_engineering/prompt_ab_experiment.h` + `src/prompt_engineering/prompt_ab_experiment.cpp`. 6 tests ABF-01..06.)

## Implementation Phases

### Phase 1 — Core Template API ✅
- [x] `PromptManager`, `PromptTemplateValidator`
- [x] `FeedbackCollector`, `PromptEvaluator`

### Phase 2 — Optimization & Tracking ✅
- [x] `PromptOptimizer` A/B testing
- [x] `PromptPerformanceTracker` cost/quality

### Phase 3 — Version Control & Context ✅
- [x] `PromptVersionControl` git-like versioning
- [x] `ContextWindowBudgetManager` token budgets

### Phase 4 — Safety & RAG ✅
- [x] `PromptInjectionDetector`
- [x] `RAGPromptBuilder`, `ChainOfThoughtBuilder`
- [x] `SystemPromptManager`

### Phase 5 — Reflection & Self-Improvement ✅
- [x] `ReflectionTuner` four strategies
- [x] `ILLMProviderReflectionAdapter`
- [x] `PromptEngineeringMetrics`

### Phase 6 — Future Enhancements (Planned)
- [x] Structured output enforcement (JSON schema + regex grammar) (Target: Q3 2026)
- [x] Prompt compression / summarization for context reduction (Target: Q3 2026)
- [x] Adversarial prompt testing framework (Target: Q4 2026)

## Production Readiness Checklist

- [x] Injection detector validated against OWASP LLM Top 10 patterns
- [x] Token budget manager tested across GPT-4, Claude 3, and Llama-3 tokenizers
- [x] Reflection strategies validated on synthetic task improvement benchmarks
- [x] Structured output enforcement (Target: Q3 2026)
