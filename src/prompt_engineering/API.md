---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering API Reference

## Purpose and contract source

This page is a map of the C++ public interfaces, not a replacement for their declarations. The authoritative signatures and types are in [`include/prompt_engineering/`](../../include/prompt_engineering/); this page highlights call patterns, return semantics, and integration boundaries.

## Core interfaces

| Header | Entry points | Contract notes |
|---|---|---|
| [`prompt_manager.h`](../../include/prompt_engineering/prompt_manager.h) | `PromptManager`, `PromptTemplate`, `validateTemplate()`, `createTemplate()`, `getTemplate()`, `injectContext()`, `getPromptWithContext()` | Creates in-memory templates or optionally persists them. `getTemplate()` returns `nullopt` on a miss; rendering also returns `nullopt` for an inactive template. Invalid creation returns a template with an empty ID. |
| [`prompt_template_validator.h`](../../include/prompt_engineering/prompt_template_validator.h) | `PromptTemplateValidator`, `TemplateValidationResult` | Validates serialized JSON. `require_id=false` supports pre-persist validation. The string overload returns an invalid result on JSON parse failure; content pattern findings are warnings that make this overload invalid. |
| [`prompt_version_control.h`](../../include/prompt_engineering/prompt_version_control.h) | `PromptVersionControl`, `PromptVersion`, `PromptDiff`, `BranchInfo`, `MergeResult` | Commit/history, branch, diff, merge, tag, and rollback operations. Optional database pointers are non-owning. Check optionals and result status before using retrieved versions or merge output. |
| [`prompt_optimizer.h`](../../include/prompt_engineering/prompt_optimizer.h) | `PromptOptimizer`, `OptimizationConfig`, `TestCase`, `OptimizerResult` | Optimization takes an evaluation callback and optional improvement callback. The result is bounded by configuration and supplied cases. |
| [`prompt_evaluator.h`](../../include/prompt_engineering/prompt_evaluator.h) | `PromptEvaluator`, `IEmbeddingProvider`, `EvaluatorConfig` | Single/batch scoring and optional provider-based semantic similarity. Input vector sizes must match for batch evaluation; a mismatch is logged and returns default aggregate metrics. |
| [`feedback_collector.h`](../../include/prompt_engineering/feedback_collector.h) | `FeedbackCollector`, `FeedbackEntry`, `FeedbackStats`, `FeedbackType` | Records and queries feedback; entries include original query and response. Severity is clamped by the implementation. |
| [`prompt_engineering_integration.h`](../../include/prompt_engineering/prompt_engineering_integration.h) | `PromptEngineeringIntegration`, `IntegrationConfig`, `ExecutionContext` | Hooks before and after model execution; config controls optional tracking, injection detection, reflection tuning, and background optimization. |
| [`prompt_engineering_metrics.h`](../../include/prompt_engineering/prompt_engineering_metrics.h) | `PromptEngineeringMetrics`, `AlertConfig` | Exports Prometheus text, snapshots/restores JSON counters, and may call a threshold callback on the metric-recording thread. |

## Additional public surfaces

- [`prompt_injection_detector.h`](../../include/prompt_engineering/prompt_injection_detector.h): scores prompt and response text, reports matched heuristic patterns, and returns a sanitized copy. Detection is not an authorization decision.
- [`rewrite_engine.h`](../../include/prompt_engineering/rewrite_engine.h), [`rewrite_rule.h`](../../include/prompt_engineering/rewrite_rule.h), and [`rewrite_rule_loader.h`](../../include/prompt_engineering/rewrite_rule_loader.h): deterministic phased rewrite contracts, rule registration, and YAML loading restricted to lexical rules. The core is not automatically invoked by the integration façade; see [`ROADMAP.md`](./ROADMAP.md) for integration status.
- [`prompt_ab_experiment.h`](../../include/prompt_engineering/prompt_ab_experiment.h) and [`prompt_regression_runner.h`](../../include/prompt_engineering/prompt_regression_runner.h): variant assignment, outcome summaries, and fixture-based regression checks.
- Other headers expose template compilation, structured output, RAG prompt building, context budgets, reflection, reasoning helpers, compression, and adversarial testing. See the directory's header inventory in [`include/prompt_engineering/README.md`](../../include/prompt_engineering/README.md); consult individual headers for exact behavior.

## Integration sequence

For the integration façade, call `beforeExecution(prompt_id, context)`, use the resulting `ExecutionContext::enhanced_prompt` in the owning LLM adapter, then call `afterExecution(context, response, success, latency_ms, user_feedback)`. The prompt module does not make the model call and cannot enforce that callers invoke both hooks; define error and retry policy at the integrating service.

For direct manager use, validate the template, create or fetch it, check the optional result and active state, then inject only the caller-authorized context values. The placeholder renderer is literal substitution and does not escape inserted text.

## Error and edge handling

- Inspect result fields (`valid`, `errors`, `warnings`, `is_injection`, `success`, `blocked`) rather than assuming a call succeeded.
- Treat absent template/version IDs as an explicit miss. A manager creation failure is signaled by an empty returned ID.
- Keep provider outputs consistent with the embedding contract in [`EMBEDDING.md`](./EMBEDDING.md).
- The error-code enum and formatter are declared in [`prompt_engineering_errors.h`](../../include/prompt_engineering/prompt_engineering_errors.h). It does not define a shared error-context type; inspect the specific operation's result or exception contract.
- Use [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for persistence, concurrency, and rewrite invariants.

## Validation

Focused API and behavior tests are in [`tests/prompt_engineering/`](../../tests/prompt_engineering/). Build and run instructions are in [`MODULE_GUIDE.md`](./MODULE_GUIDE.md).

Exact model backend not verifiable from final document text alone.
