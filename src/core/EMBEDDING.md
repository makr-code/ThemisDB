---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Boundary

> Exact model backend not verifiable from final document text alone.

## Applicability

Core does not generate, store, index, or evaluate vector embeddings. Its public headers under `include/core/` define concern interfaces, configuration, initialization helpers, and adapter lifecycle; the implementation under `src/core/` provides those facilities. There is no embedding model, vector representation, tokenization contract, or embedding-specific test suite in this module.

This page exists to make the module boundary explicit for developers routing embedding work. Do not add embedding semantics to `ConcernsContext` merely because embedding consumers may also use Core logging, tracing, metrics, caching, secrets, or circuit breaking.

## Integration boundary

Embedding generation and vector indexing belong to the owning feature/module and its API. Such code may receive a `ConcernsContext` and use its interfaces for cross-cutting behavior, but Core does not define:

- model identity, weights, tokenizer, dimensions, normalization, or vector compatibility;
- embedding request/response types, batching, retries, or model fallback;
- vector persistence, ANN indexing, similarity metrics, or relevance evaluation.

The actual embedding owner must document those contracts and their security, reproducibility, and performance requirements. Core's [`API.md`](API.md) and [`MODULE_GUIDE.md`](MODULE_GUIDE.md) describe only the shared infrastructure API and integration path.

## Evidence and change gate

This boundary is grounded in the Core public header inventory in [`include/core/README.md`](../../include/core/README.md) and the implementation/test inventory under `src/core/` and `tests/core/`. If embedding behavior is introduced to this module, first update the architecture and API contract, identify the model and vector source of truth, and add tests that verify vector dimensions, invalid values, compatibility, and failure handling. Until then, no embedding quality or latency claims apply to Core.
