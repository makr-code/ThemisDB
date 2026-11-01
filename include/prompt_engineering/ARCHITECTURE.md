---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering Public Header Architecture

## Scope and ownership

`include/prompt_engineering/` declares the public C++ interfaces. Implementations are in [`../../src/prompt_engineering/`](../../src/prompt_engineering/), and the canonical runtime overview is [`../../src/prompt_engineering/ARCHITECTURE.md`](../../src/prompt_engineering/ARCHITECTURE.md). Header filenames and current entry points are indexed in [`README.md`](./README.md); declarations in each header are authoritative for signatures and types.

## Interface groups

- **Template lifecycle and storage:** `prompt_manager.h`, `prompt_template_validator.h`, `prompt_template_compiler.h`, `prompt_library_io.h`, `system_prompt_manager.h`
- **Revision and quality workflows:** `prompt_version_control.h`, `prompt_optimizer.h`, `prompt_evaluator.h`, `prompt_regression_runner.h`, `prompt_ab_experiment.h`, `prompt_quality_evaluator.h`
- **Context and retrieval:** `context_window_manager.h`, `rag_context_budget_manager.h`, `rag_prompt_builder.h`, `prompt_compressor.h`
- **Safety and errors:** `prompt_injection_detector.h`, `adversarial_prompt_tester.h`, `prompt_engineering_errors.h`
- **Reasoning and integrations:** `chain_of_thought.h`, `cot_tracer.h`, `tree_of_thoughts.h`, `reflection_tuner.h`, `llm_reflection_adapter.h`, `meta_prompt_generator.h`, `dspy_module.h`, `self_improvement_orchestrator.h`
- **Rewrite contracts:** `rewrite_engine.h`, `rewrite_rule.h`, `rewrite_rule_loader.h`
- **Feedback and observability:** `feedback_collector.h`, `prompt_engineering_metrics.h`, `prompt_performance_tracker.h`

These are library interfaces, not an LLM transport or serving API. `PromptEngineeringIntegration` provides `beforeExecution()` and `afterExecution()` hooks; the caller owns the model call and decides how to handle failures. The rewrite engine is a separate public surface and is not automatically invoked by those hooks.

## Build and contract boundaries

The root CMake configuration adds prompt-engineering implementation sources to `themis_core`; module tests and benchmarks are registered separately. Database wrapper and column-family pointers accepted by manager/version/feedback interfaces are non-owning, so their owners must outlive the receiving objects. See the individual headers for exact lifetime, concurrency, and error contracts.

Use [`../../src/prompt_engineering/API.md`](../../src/prompt_engineering/API.md) for a call-flow overview and [`../../src/prompt_engineering/MODULE_GUIDE.md`](../../src/prompt_engineering/MODULE_GUIDE.md) for build and focused-test commands.
