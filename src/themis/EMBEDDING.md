---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Boundary — Themis Core

## Purpose and scope

This page answers whether `src/themis` implements embedding generation and
identifies the boundary for developers looking for that behavior. The module
does **not** tokenize text, call an embedding model, produce vectors, or own
vector-index insertion and search. Those responsibilities belong to the
relevant LLM, embedding, and vector-index components, not this runtime
foundation module.

The `themis` core scope is build identity, edition/license decisions, module
verification and loading, dependency resolution, and wire-server operation;
see [`README.md`](./README.md) and [`ARCHITECTURE.md`](./ARCHITECTURE.md).
The headers under [`include/themis`](../../include/themis/) expose build,
edition, license, and verification contracts. They do not define an embedding
request/result interface.

## Related boundary

Edition policy can expose ceilings consumed by other components, including
LLM resource policy values in [`edition.h`](../../include/themis/edition.h).
Those constants constrain resources; they do not choose an embedding model,
define input normalization, or guarantee vector dimensions or quality.
Likewise, the wire server may transport application requests, but transport
does not make it the owner of the model or vector semantics.

## Evaluation implications

Embedding quality and reproducibility must be tested where the embedding model
and vector pipeline are implemented. Relevant measures include model/version
identity, input preprocessing, output dimension and numeric validity, and
downstream retrieval evaluation. No embedding-specific benchmark or quality
metric is claimed for `src/themis`; the core module's benchmark mappings are
listed in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

When modifying this boundary, update the owning module's API and evaluation
documentation. Keep this page aligned with
[`MODULE_GUIDE.md`](./MODULE_GUIDE.md) so integration consumers do not mistake
edition resource limits or network transport for embedding functionality.
