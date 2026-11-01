---
Author: "ThemisDB Team"
Created: "2026-05-13"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering Module — Public Headers

This directory declares the public C++ interfaces for prompt templates, evaluation, revision, context handling, safety helpers, and integration hooks. Header declarations are authoritative; this index describes responsibility rather than duplicating signatures.

## Header inventory

| Header | Responsibility |
|---|---|
| `adversarial_prompt_tester.h` | Adversarial prompt test cases and reports |
| `chain_of_thought.h` | Chain-of-thought prompt construction |
| `context_window_manager.h` | Token estimation and context-window budgeting |
| `cot_tracer.h` | Chain-of-thought trace collection |
| `dspy_module.h` | DSPy-style signatures and modules |
| `feedback_collector.h` | Feedback recording, aggregation, and pruning |
| `llm_reflection_adapter.h` | Adapter for reflection provider interfaces |
| `markdown_utils.h` | Markdown processing helpers |
| `meta_prompt_generator.h` | Meta-prompt generation contract |
| `prompt_ab_experiment.h` | Prompt variant experiments |
| `prompt_compressor.h` | Prompt compression contract |
| `prompt_engineering_errors.h` | Prompt-engineering error-code enum and formatter |
| `prompt_engineering_integration.h` | Execution hooks and background optimization lifecycle |
| `prompt_engineering_metrics.h` | In-memory counters, export, and snapshot/restore |
| `prompt_evaluator.h` | Output scoring and optional embedding-provider interface |
| `prompt_injection_detector.h` | Prompt/response injection heuristics and sanitization |
| `prompt_library_io.h` | Prompt-library import/export |
| `prompt_manager.h` | Template lifecycle, validation, and context substitution |
| `prompt_optimizer.h` | Bounded optimization loop |
| `prompt_performance_tracker.h` | Per-prompt performance measurements |
| `prompt_quality_evaluator.h` | Prompt quality checks and reports |
| `prompt_regression_runner.h` | Fixture-based prompt regression evaluation |
| `prompt_template_compiler.h` | Typed prompt-template compilation |
| `prompt_template_validator.h` | Serialized-template validation |
| `prompt_version_control.h` | Prompt revisions, branches, diffs, and rollback |
| `protegi_optimizer.h` | ProTeGi-style textual-gradient optimization |
| `rag_context_budget_manager.h` | RAG token-budget reservations |
| `rag_prompt_builder.h` | Retrieval-augmented prompt assembly |
| `reflection_tuner.h` | Reflection-based response refinement |
| `rewrite_engine.h` | Ordered rewrite execution interfaces |
| `rewrite_rule.h` | Rewrite rule types and contracts |
| `rewrite_rule_loader.h` | YAML loader for supported lexical rule types |
| `self_improvement_orchestrator.h` | Optimization orchestration and A/B handoff |
| `structured_output.h` | Structured-output enforcement interfaces |
| `system_prompt_manager.h` | System-prompt lifecycle and rendering |
| `tree_of_thoughts.h` | Tree-of-thought prompt exploration |

## Runtime boundaries

- Most APIs are library calls; this module does not make an LLM request or provide a metrics HTTP endpoint.
- `PromptEngineeringIntegration` exposes `beforeExecution()` and `afterExecution()` hooks. The caller must execute the model call between them and define its own error and retry policy.
- `PromptManager::injectContext()` performs literal substitution; it does not escape inserted values or authorize context.
- The injection detector provides heuristics, not an authorization decision or a complete safety boundary.
- `PromptEvaluator` uses lexical similarity by default; embedding scoring requires a caller-supplied provider.
- `RewriteEngine` is implemented as a separate core API but is not automatically called by the integration hooks.

## Build and use

The module is built as part of ThemisDB. For the supported build preset and focused test commands, see [`../../src/prompt_engineering/MODULE_GUIDE.md`](../../src/prompt_engineering/MODULE_GUIDE.md). For exact API call flow and error handling, see [`../../src/prompt_engineering/API.md`](../../src/prompt_engineering/API.md) and the corresponding public headers.

## Related documentation

- [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- [`../../src/prompt_engineering/README.md`](../../src/prompt_engineering/README.md)
- [`../../src/prompt_engineering/ROADMAP.md`](../../src/prompt_engineering/ROADMAP.md)
- [`../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)
