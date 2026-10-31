---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval feature inventory

This page describes the checked-in `src/retrieval` code, not ThemisDB's wider
search, vector, or RAG capabilities.

## Present

- LoRA package and portable-product metadata structures, JSON conversion, and
  manifest SHA-256 helpers in `src/retrieval/include/lora_package.h`.
- An in-memory, mutex-protected `LoRAManifestStore` with CRUD, list,
  import/export, and explicit manifest verification methods.
- An optional signature-verifier callback. It is called only when both the
  callback and a non-empty signature are present.
- Retrieval metric calculations and helper methods (`HasRegression()` and
  `ComputeDeltas()`) in the dual-read validator surface.

## Present but incomplete

- `DualReadValidator::ExecuteQuery()` is a placeholder returning no results.
  `ValidateQuery()` returns current/previous result vectors and current
  metrics, logs the previous recall, and does not return deltas.
- `include/retrieval/retrieval.h` is a placeholder umbrella with no includes
  or declarations.
- Tests and benchmarks with retrieval-related names use local mocks, stubs,
  or synthetic routing loops; they are not production hybrid retrieval or GPU
  execution.

## Not provided by this module

No ANN or vector search pipeline, ranking/reranking runtime, embedding
generation, persistence backend, lifecycle/deployment enforcement, runtime
configuration system, or production performance baseline is implemented by
this target. See the [module README](README.md), [architecture](ARCHITECTURE.md),
and [API reference](API.md).
