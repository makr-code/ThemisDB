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
- A mutex-protected, RocksDB-backed `LoRAManifestStore` with explicit typed
  configuration, durable CRUD/import/export, and reload-on-reopen behavior.
- Fail-closed signature verification requiring a matching manifest hash,
  Ed25519 signature, an active public-key fingerprint pin, and ThemisDB's shared
  pinned-key verifier.
- A gated `deployProduct()` path requiring a quality controller's allow result
  and a separate deployment authorization callback from the owning service.
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
generation, owner/RBAC policy, deployment-service callback wiring, or
production performance baseline is implemented by this target. See the
[module README](README.md), [architecture](ARCHITECTURE.md), and
[API reference](API.md).
