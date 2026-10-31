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

# Prompt Engineering Models and Assumptions

## Purpose

This document describes the computational models implemented by `prompt_engineering`. It is for developers selecting a scoring, optimization, or rewrite path; it does not describe model training or LLM serving.

## Quality scoring model

`PromptEvaluator` computes four per-output signals:

1. semantic similarity: normalized word-token Jaccard overlap by default, optionally cosine similarity from an attached embedding provider;
2. exact match: equality after whitespace and case normalization;
3. partial match: one minus normalized Levenshtein distance;
4. relevance: fraction of expected tokens present in the output.

Batch aggregation applies the configured similarity, exact-match, and relevance weights. Partial match is used as a lower bound for each of those weighted components. The default weights are 0.5, 0.3, and 0.2 respectively; the default pass threshold is 0.7. These are engineering defaults, not a validated universal quality rubric. See [`prompt_evaluator.h`](../../include/prompt_engineering/prompt_evaluator.h) and [`prompt_evaluator.cpp`](./prompt_evaluator.cpp).

## Optimization and experimentation

`PromptOptimizer` runs a bounded iterative loop controlled by `OptimizationConfig`; evaluation and prompt improvement are supplied as callbacks (or the default improvement routine is selected). An optimizer score is meaningful only relative to the callback, cases, and stopping configuration used to produce it. It is not a guarantee of better user outcomes.

The A/B framework compares explicit control and treatment version IDs and selects a variant deterministically from the request identifier. The regression runner compares outputs over fixed fixtures and can return an inconclusive result when fixture coverage is below its configured minimum. Statistical checks and fixture-based checks should be used together with human review for a production prompt change.

## Rewrite model

The rewrite engine is a deterministic rule executor, distinct from an LLM inference model. It runs phases in this order: input normalization, policy enforcement, NL-to-AQL preprocessing, then post-generation canonicalization. Within a phase, rules are sorted by ascending priority with registration order preserved for ties. It repeats a phase while transformations continue, subject to a step limit; a failed rule or terminal block stops the pipeline.

YAML configuration is restricted to regex and dictionary lexical rules. Policy and semantic rule implementations are C++-only. A YAML loader failure does not append partially loaded rules to its output vector. See [`rewrite_engine.h`](../../include/prompt_engineering/rewrite_engine.h), [`rewrite_rule.h`](../../include/prompt_engineering/rewrite_rule.h), and [`rewrite_rule_loader.cpp`](./rewrite_rule_loader.cpp).

This describes the engine core, not automatic request-pipeline behavior. The module roadmap tracks wiring the core into prompt and NL-to-AQL flows, and functional rule-execution coverage, as incomplete. Do not infer production integration from the presence of the engine implementation.

## Assumptions and evaluation

- Deterministic scoring and rewrite results require unchanged inputs, configuration, rule registration order, and provider behavior.
- Caller-provided evaluation data must represent the task and include negative cases; aggregate metrics alone do not measure policy compliance.
- Rule changes require tests for order, idempotence where claimed, terminal behavior, step exhaustion, and malformed configuration.
- Changes to scoring weights, tokenization, fixtures, embedding provider, or experiment assignment alter the meaning of historical results. Record those changes alongside benchmark and experiment evidence.

The focused evaluator, regression, and rewrite tests are listed in [`tests/prompt_engineering/README.md`](../../tests/prompt_engineering/README.md). Performance cases and release thresholds are mapped in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

## Non-goals

- The module does not provide a built-in embedding model in the `IEmbeddingProvider` contract.
- Lexical scores are not a substitute for domain-specific quality review.
- Rewrite rules do not make an LLM safe by themselves; authorization, tool policy, and downstream validation remain the responsibility of their owning components.

See [`RESEARCH.md`](./RESEARCH.md) for evaluation methodology and [`TRUST.md`](./TRUST.md) for security boundaries.

Exact model backend not verifiable from final document text alone.
