**Author:** ThemisDB Team
**Created:** 2026-10-30
**Last Updated:** 2026-10-30
**Status:** active
**Source level:** Level 1 — `src/content/`
**SOT domain:** `module-behavior`
**Canonical source references:** [embedding pipeline](embedding_pipeline.cpp), [manager adapter](content_manager_embedding.cpp), [public embedding contract](../../include/content/embedding_pipeline.h), [manager contract](../../include/content/content_manager.h), [embedding tests](../../tests/content/test_content_embedding_pipeline.cpp)
**Related issue:** #7109
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Provenance note:** Exact model backend not verifiable from final document text alone.

# Embedding behavior in the content module

## Purpose

This is an implementation-grounded guide to content embeddings for developers and
operators. It describes the API path and its limits; it does not promise a specific
embedding model, vector normalization, or retrieval quality.

## Runtime path

1. A caller may attach a shared `EmbeddingPipeline` to `ContentManager` with
   `setEmbeddingPipeline()`.
2. `EmbeddingPipeline::isEnabled()` is true exactly when its configured `model_name` is
   non-empty.
3. `generateEmbedding()` returns an empty vector if disabled or given empty text.
   Otherwise it calls `THEMIS_LLM().embed(text)`, waits up to the configured timeout,
   returns the vector on success, and returns an empty vector on timeout, empty backend
   output, or caught exception.
4. Failure cases handled by `embedWithTimeout()` increment the pipeline failure count
   and, when supplied, `ContentMetrics::recordEmbeddingFailure()`. Disabled and empty
   input return before that failure path.
5. `ContentManager::generateEmbedding(text, model_name)` prefers an attached enabled
   pipeline; otherwise it delegates to a registered TEXT processor; with neither, it
   returns an empty vector.

The `model_name` argument on the manager method is unnamed/unused in the current
implementation. The pipeline's `model_name` enables the path but is not forwarded to
`THEMIS_LLM().embed()`. Confirm backend selection outside this module before relying on
model identity.

## Configuration and result contract

| Setting / behavior | Current source behavior |
|---|---|
| `model_name` | Empty means pipeline disabled; non-empty means enabled. |
| `batch_size` | Constructor clamps to 1–32. Batch API processes input in sequential subranges and preserves result positions; each item is embedded individually. |
| `timeout_ms` | Constructor clamps values below 100 ms to 100 ms; default is 5000 ms. |
| `embedding_dim` | A positive configured value initializes the reported dimension; otherwise the first non-empty success records a dimension. Later dimension mismatch is not rejected by this class. |
| Output | Vector of floats or empty vector; this implementation does not establish normalization or a cross-model compatibility guarantee. |

The timeout implementation uses `std::async`; the source comment notes that destruction
of a timed-out future can wait for the worker to finish. A timeout return should therefore
not be interpreted as proof that the underlying computation was forcibly cancelled.

## Content ingestion and retrieval

Ingestion enables the embedding stage according to the stage configuration and optional
`embedding_model` config key: an explicitly present empty value disables the stage; a
non-empty value enables it only when the embedding stage is enabled; absence leaves the
stage setting in control. Actual vector generation during ingestion additionally
requires an attached, enabled pipeline. A stage setting is not proof that a vector was
produced or indexed.

For semantic search behavior and index ownership, use the public
[ContentManager contract](../../include/content/content_manager.h) and
[module architecture](ARCHITECTURE.md); do not infer search quality from embedding
generation alone.

## Validation and tests

Run the focused embedding tests registered through
[`tests/content/CMakeLists.txt`](../../tests/content/CMakeLists.txt). The suite verifies
enable/disable behavior, config clamping, empty output, batch result shape, reported
dimension, and the embedding-failure metric. It does not invoke a real model to measure
semantic quality or establish the selected backend.

Monitor `getFailureCount()` and, if configured, the
[`ContentMetrics`](../../include/content/content_metrics.h) embedding failure counter.
An empty result must be diagnosed before retrying or treating the content as enriched.

## Sources

- [Embedding implementation](embedding_pipeline.cpp) and
  [manager integration](content_manager_embedding.cpp).
- [Embedding public API](../../include/content/embedding_pipeline.h),
  [manager public API](../../include/content/content_manager.h), and
  [metrics API](../../include/content/content_metrics.h).
- [Embedding tests](../../tests/content/test_content_embedding_pipeline.cpp).
- [Module overview](README.md), [architecture](ARCHITECTURE.md), and
  [performance expectations](PERFORMANCE_EXPECTATIONS.md).
