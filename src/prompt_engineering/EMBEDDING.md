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

# Embedding Provider Contract

## Purpose and boundary

`PromptEvaluator` optionally delegates semantic-similarity scoring to an `IEmbeddingProvider`. The interface is an integration point only: the provider implementation, model lifecycle, transport, credentials, batching, and resource limits belong to the caller. The module has no built-in provider implementation.

## Provider requirements

Implement `embed(text)` to return a dense vector for the input text and `name()` to provide a stable, non-secret identifier for logs and evaluation details. For each comparison, both texts must produce non-empty vectors of the same dimension. Keep the provider's model and preprocessing configuration stable across a baseline/candidate evaluation.

`PromptEvaluator::setEmbeddingProvider()` and `clearEmbeddingProvider()` protect the provider pointer with a mutex. Each evaluation takes a shared-pointer snapshot before invoking the provider, so replacing the configured provider does not invalidate an in-flight call. The provider itself must support the concurrency pattern used by callers.

## Scoring and fallback behavior

When vectors are usable, the evaluator computes cosine similarity and clamps the result to `[0, 1]`. An absent provider, an empty embedding, or an exception from `embed()` signals fallback to the built-in Jaccard token-overlap score; provider exceptions are logged. The fallback is a different metric, so record whether embedding scoring was active when comparing results.

There is a contract discrepancy to account for when implementing providers: [`prompt_evaluator.h`](../../include/prompt_engineering/prompt_evaluator.h) documents a dimension mismatch as a `-1.0` failure, while the current implementation passes non-empty vectors to `computeCosineSimilarity()`, which returns `0.0` for mismatched dimensions. That value is treated as a valid score, not as a request to fall back. Return equal dimensions and add a focused regression if changing this behavior.

The cosine helper returns `0.0` for empty vectors, mismatched dimensions, or a near-zero norm. `evaluateSingle()` records the provider name in `details` when a provider was attached, including when a failed embedding call subsequently uses the lexical fallback.

## Validation protocol

1. Test the provider's behavior for empty text, empty vectors, mismatched dimensions, zero vectors, exceptions, and representative domain pairs.
2. Verify finite numeric output and equal dimensions before relying on evaluation scores.
3. Compare provider-assisted results with lexical fallback on a fixed, reviewed fixture set; do not treat higher similarity alone as improved task quality.
4. Pin the provider/model version in experiment and benchmark records. A model or preprocessing change requires a new baseline.

The interface and fallback implementation are in [`prompt_evaluator.h`](../../include/prompt_engineering/prompt_evaluator.h) and [`prompt_evaluator.cpp`](./prompt_evaluator.cpp). Evaluation metrics and experiment assumptions are described in [`RESEARCH.md`](./RESEARCH.md).

## Privacy and operational limits

Embedding inputs are sent to whatever service or model the caller configures. The module does not redact inputs, manage provider credentials, or guarantee that embeddings are anonymous or non-reversible. Apply the deployment's data-handling policy before embedding user prompts or responses; see [`TRUST.md`](./TRUST.md).

Exact model backend not verifiable from final document text alone.
