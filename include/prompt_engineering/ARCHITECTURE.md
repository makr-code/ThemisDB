> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/prompt_engineering/ARCHITECTURE.md -->

# Prompt Engineering Module — Public Header Architecture

**Module Path:** `include/prompt_engineering/`  
**Implementation:** `../../src/prompt_engineering/`  
**Canonical architecture doc:** [`../../src/prompt_engineering/ARCHITECTURE.md`](../../src/prompt_engineering/ARCHITECTURE.md)

---

## 1. Overview

`include/prompt_engineering/` defines the **public prompt authoring, optimisation, versioning, evaluation, injection detection, RAG budget management, chain-of-thought, and self-improvement orchestration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/prompt_engineering/ARCHITECTURE.md`](../../src/prompt_engineering/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Prompt Authoring and Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `prompt_manager.h` | `PromptManager` | Prompt lifecycle and versioning |
| `prompt_template_compiler.h` | `PromptTemplateCompiler` | Jinja-style prompt template compilation |
| `prompt_template_validator.h` | `PromptTemplateValidator` | Template syntax and safety validation |
| `prompt_version_control.h` | `PromptVersionControl` | Git-style prompt version control |
| `prompt_library_io.h` | `PromptLibraryIO` | Prompt library serialisation and import/export |
| `system_prompt_manager.h` | `SystemPromptManager` | System prompt lifecycle management |
### 2.2 Optimisation and Compression

| Header | Public Type | Purpose |
|--------|------------|---------|
| `prompt_optimizer.h` | `PromptOptimizer` | Automatic prompt optimisation |
| `protegi_optimizer.h` | `ProtegiOptimizer` | ProTeGi gradient-based prompt optimisation |
| `prompt_compressor.h` | `PromptCompressor` | Semantic prompt compression |
| `context_window_manager.h` | `ContextWindowManager` | Context window budget allocation |
| `rag_context_budget_manager.h` | `RAGContextBudgetManager` | RAG context budget tracking |
| `rag_prompt_builder.h` | `RAGPromptBuilder` | RAG-aware prompt construction |
### 2.3 Evaluation and Testing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `prompt_evaluator.h` | `PromptEvaluator` | Prompt quality evaluation |
| `prompt_quality_evaluator.h` | `PromptQualityEvaluator` | Multi-dimensional quality scoring |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker` | Per-prompt performance telemetry |
| `prompt_regression_runner.h` | `PromptRegressionRunner` | Automated prompt regression tests |
| `prompt_ab_experiment.h` | `PromptABExperiment` | A/B experimentation for prompt variants |
| `adversarial_prompt_tester.h` | `AdversarialPromptTester` | Red-team adversarial prompt testing |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | Prompt injection attack detection |
| `feedback_collector.h` | `PromptFeedbackCollector` | User feedback collection for prompts |
### 2.4 Chain-of-Thought and Reasoning

| Header | Public Type | Purpose |
|--------|------------|---------|
| `chain_of_thought.h` | `ChainOfThought` | Chain-of-thought prompt orchestration |
| `cot_tracer.h` | `COTTracer` | CoT execution trace capture |
| `tree_of_thoughts.h` | `TreeOfThoughts` | Tree-of-Thoughts search over reasoning paths |
| `meta_prompt_generator.h` | `MetaPromptGenerator` | Meta-prompt automatic generation |
| `dspy_module.h` | `DSPyModule` | DSPy-style prompt module abstraction |
| `structured_output.h` | `StructuredOutput` | JSON/grammar-constrained structured output |
| `markdown_utils.h` | `MarkdownUtils` | Markdown-aware prompt utility functions |
### 2.5 Self-Improvement

| Header | Public Type | Purpose |
|--------|------------|---------|
| `self_improvement_orchestrator.h` | `SelfImprovementOrchestrator` | Self-improvement loop orchestration |
| `reflection_tuner.h` | `ReflectionTuner` | Reflection-based prompt tuning |
| `llm_reflection_adapter.h` | `LLMReflectionAdapter` | LLM adapter for reflection prompts |

---

## 3. Namespace Layout

All public types reside in the `themis::prompt_engineering` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/prompt_engineering/` expose the **stable public API**; internal types live in `src/prompt_engineering/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
