---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# LLM Embedding and Retrieval Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

This document covers the embedding path and the runtime assumptions around semantic vector generation in the LLM module. The concrete implementation is in:

- `include/llm/embedded_llm.h`
- `src/llm/embedded_llm.cpp`
- `src/llm/lora_router.cpp`
- `src/llm/llm_prefix_cache.cpp` and related cache code

## Public embedding interface

`EmbeddedLLM` exposes the relevant embedding APIs in `include/llm/embedded_llm.h`:

- `embed(const std::string& text)`
- `embedBatch(const std::vector<std::string>& texts)`
- `setEmbedFn()` for externally injected embedding implementations

The interface is intentionally small and direct. It requires either a real model-backed embedding path or a deterministic fallback, and it does not permit silent null or stale vector outputs.

## Runtime behavior

The implementation in `src/llm/embedded_llm.cpp` follows a simple flow:

1. validate the input text and determine whether a custom embed function is configured
2. if no custom function is present, fall back to the default runtime behavior
3. if a model load is not available, return a normalized deterministic vector produced by `buildFallbackEmbedding()`
4. cache the embedding under a mutex-protected map if caching is enabled

This is a particularly important detail: the embed API is designed to remain callable even when the model is unavailable. The fallback output is not semantic quality; it is a safety-preserving output shape used to keep the runtime stable.

## Fallback embedding semantics

The fallback path creates a 64-dimensional float vector. The algorithm:

- iterates over the input bytes
- maps bytes to a bucket using `(byte + index) % 64`
- adds weighted contributions per character to the bucket
- computes the Euclidean norm
- normalizes the vector to unit length

This is documented directly in `buildFallbackEmbedding()`. It gives a predictable, reproducible embedding for degraded environments without pretending to represent a trained embedding model.

## Relationship to routing and adapters

`src/llm/lora_router.cpp` shows that embedding results are also used as a decision signal for adapter routing. The router computes candidate adapters using a semantic similarity model and a configurable similarity threshold. This means embedding quality is part of request classification and adapter selection, not just isolated vector generation.

The expected behavior is:

- use embeddings to produce meaningful similarity scores
- reject or fall back when the embedding path is unavailable or below threshold
- avoid silently picking an arbitrary adapter without a reasoned decision

## Operational constraints

The embedding subsystem has several practical constraints encoded by the code:

- a model may be missing, partially loaded, or degraded
- embedding generation is concurrency-sensitive and uses `cache_mutex_`
- fallback behavior is deterministic but low-confidence
- adapter routing depends on score quality and thresholding, so poor-quality embeddings can drive incorrect route selection

These constraints are exactly why the module keeps the embedding contract narrow and explicit.

## Failure modes and expected behavior

The runtime should treat the following as operationally relevant outcomes:

- no model loaded: fallback vector is returned, but with explicit degraded semantics
- custom embed function failure: surfaced via the runtime callback path and not silently ignored
- cache contention: serialized under mutex to avoid undefined behavior
- routing ambiguity: no silent default; the decision should be traceable to a route or fallback policy

## Conclusion

The LLM module treats embedding as a structured runtime capability with a bounded degraded mode. Its primary goal is not to guarantee semantic quality in the absence of a proper model, but to make degraded behavior predictable, measurable, and safe enough for downstream operators and application code.
