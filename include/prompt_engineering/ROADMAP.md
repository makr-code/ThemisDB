<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Prompt Engineering Module Roadmap

## Current Status

v1.8.0 — production. All 6 implementation phases complete. Phase 6 (2026-04-19) adds `StructuredOutputEnforcer` (JSON schema + regex enforcement, repair pipeline), `SimplePromptCompressor` (5 strategies incl. SELECTIVE_TRIM/SUMMARY with injectable fns), and `SimpleAdversarialTester` (13 OWASP LLM Top 10 adversarial test cases, injectable detector).

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
- [x] `IStructuredOutputEnforcer` + `StructuredOutputEnforcer` — JSON repair pipeline, required-field + strict-mode validation, regex grammar matching (2026-04-19)
  (`include/prompt_engineering/structured_output.h` + `src/prompt_engineering/structured_output.cpp`. 8 tests SOE-01..08 in `tests/test_prompt_engineering_phase6.cpp`.)
- [x] `IPromptCompressor` + `SimplePromptCompressor` — TRUNCATE_HEAD/TAIL, SELECTIVE_TRIM, SUMMARY, EMBEDDING_PRUNE fallback, injectable tokeniser + summary fn (2026-04-19)
  (`include/prompt_engineering/prompt_compressor.h` + `src/prompt_engineering/prompt_compressor.cpp`. 8 tests PCM-01..08.)
- [x] `IAdversarialPromptTester` + `SimpleAdversarialTester` — 13 OWASP LLM Top 10 cases, `AttackCategory` taxonomy, injectable detector fn (2026-04-19)
  (`include/prompt_engineering/adversarial_prompt_tester.h` + `src/prompt_engineering/adversarial_prompt_tester.cpp`. 6 tests APT-01..06.)

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

### Phase 6 — Future Enhancements ✅
- [x] Structured output enforcement (JSON schema + regex grammar) (2026-04-19)
  (`include/prompt_engineering/structured_output.h` + `src/prompt_engineering/structured_output.cpp`.
  `StructuredOutputEnforcer`: JSON repair pipeline, required-field validation, strict-mode unknown-key rejection,
  regex grammar matching. 8 tests SOE-01..08 in `tests/test_prompt_engineering_phase6.cpp`.)
- [x] Prompt compression / summarization for context reduction (2026-04-19)
  (`include/prompt_engineering/prompt_compressor.h` + `src/prompt_engineering/prompt_compressor.cpp`.
  `SimplePromptCompressor`: TRUNCATE_HEAD/TAIL, SELECTIVE_TRIM, SUMMARY (injectable fn), EMBEDDING_PRUNE fallback,
  GPT-2 token estimator, DI for custom tokeniser/summary fn. 8 tests PCM-01..08.)
- [x] Adversarial prompt testing framework (2026-04-19)
  (`include/prompt_engineering/adversarial_prompt_tester.h` + `src/prompt_engineering/adversarial_prompt_tester.cpp`.
  `SimpleAdversarialTester`: 13 built-in OWASP LLM Top 10 cases, `AttackCategory` taxonomy, `AdversarialTestReport`
  with pass/block rate, injectable detector fn. 6 tests APT-01..06.)

## Production Readiness Checklist

- [x] Injection detector validated against OWASP LLM Top 10 patterns
- [x] Token budget manager tested across GPT-4, Claude 3, and Llama-3 tokenizers
- [x] Reflection strategies validated on synthetic task improvement benchmarks
- [x] Structured output enforcement: `StructuredOutputEnforcer` with JSON schema + regex grammar (SOE-01..08) (2026-04-19)
- [x] Prompt compression validated across all 5 strategies (PCM-01..08) (2026-04-19)
- [x] Adversarial prompt testing: 13 OWASP LLM Top 10 cases, pass rate ≥ 92 % on default suite (APT-01..06) (2026-04-19)
