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

# Embeddings and Index-Version Decisions

## Purpose and scope

This page documents the ingestion-side embedding contract and the currently
implemented reindex decision logic. It is for engineers wiring an embedding
backend or changing chunk/vector metadata. It does not specify the RAG query
path or claim that this module performs a complete index migration.

## Embedding contract

`IEmbeddingBackend` accepts a UTF-8 text snippet and reports its configured
dimension and availability. `builtin.chunk_embed` calls `embed()` for each
chunk and appends a `VectorRecord` with the chunk ID, source file ID, snippet,
embedding vector, and optional section/page metadata. The step does not compare
the returned vector length with `dimensions()`; implementations and callers
must validate that their vectors match the index's configured dimension.

The step checks `isAvailable()` before embedding. When unavailable,
`skip_when_unavailable` defaults to true: the step adds a warning and returns
success without producing embeddings. With the option false, it returns
`ERR_PLUGIN_NOT_FOUND`. An absent backend resolves to `NullEmbeddingBackend`,
whose default `isAvailable()` is false; it is not a production embedding
implementation. Inject a real backend and verify output dimensions before
relying on vector indexing.

## Version-change decision logic

`ReindexDecisionEngine::decide_reindex()` compares old and new metadata in
order, returning on the first match:

1. embedding model ID changed;
2. embedding dimensionality changed;
3. chunking profile ID changed;
4. index schema major version changed.

It reports a reason, a fixed estimate (3,600 seconds for model or dimension
changes, 1,800 for chunking, and 7,200 for schema-major changes), and an
`atomic_rollback_available` flag. The simplified APIs compare only their named
metadata family.

This engine is a decision helper. In the inspected implementation it does not
persist a manifest, build an index, perform dual reads, execute canary rollout,
or switch/restore a previous index. The specification examples in
[`EMBEDDING_VERSION_GOVERNANCE.md`](./EMBEDDING_VERSION_GOVERNANCE.md) describe
broader governance design; their sample commands, environment variables, and
rollout behavior must not be assumed to exist based on this helper alone.

## Reproducibility and trade-offs

Embedding and chunking metadata structures serialize identifiers, dimensions,
hash/provider fields, chunk profile parameters, and schema details to JSON.
Keep the identifiers stable and update them when the model, output dimension, or
chunking behavior changes; the decision helper compares profile ID rather than
all individual chunking fields. Likewise, it compares only the schema major
version, so parameter changes within a major version do not trigger this
decision.

The `chunk_tt_decompose` step is optional and consumes already-produced
embeddings. It applies a `shouldDecompose()` κ gate (default `min_kappa = 1.3`),
then records the returned tensor data and source/section/page provenance.
Unavailable decomposition backends are skipped by default with a warning.
This optional path does not replace ordinary vector embeddings.

## Validation

- [`test_ingestion_inference_backend.cpp`](../../tests/ingestion/test_ingestion_inference_backend.cpp) checks backend availability and default behavior.
- [`test_reindex_decision_engine.cpp`](../../tests/ingestion/test_reindex_decision_engine.cpp) covers all four triggers, no-change behavior, simplified APIs, and metadata serialization.
- [`steps/chunk_embed_step.cpp`](./steps/chunk_embed_step.cpp) and [`steps/chunk_tt_decompose_step.cpp`](./steps/chunk_tt_decompose_step.cpp) define current step behavior.

> Exact model backend not verifiable from final document text alone.
