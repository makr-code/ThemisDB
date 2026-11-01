---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/retrieval/retrieval.h"
  - "../../include/retrieval/dual_read_validator.h"
  - "include/lora_package.h"
  - "dual_read_validator.cpp"
  - "src/lora_package.cc"
  - "CMakeLists.txt"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Module Architecture

## Scope, boundaries, and ownership

The checked-in `src/retrieval/` implementation is narrower than a general retrieval engine. It contains a dual-read quality validator and a separately built LoRA artifact/manifest library. The public `include/retrieval/retrieval.h` is an umbrella-header placeholder; it does not include or declare a general ANN/search pipeline.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Public include surface | [`retrieval.h`](../../include/retrieval/retrieval.h) | Umbrella include file; current contents do not wire in the implementation headers. |
| Dual-read validation | [`DualReadValidator`](../../include/retrieval/dual_read_validator.h) | Accept index-path strings and map-supplied judgments; current query execution returns empty result vectors, while metric helpers operate on supplied results. |
| Adapter artifacts | [`lora_package.h`](include/lora_package.h) | `LoRAPackage`, `PortableAdapterProduct`, RocksDB-backed `LoRAManifestStore`, config-loaded pinned Ed25519 verification, and gated deployment API. |

The artifact implementation is built from [`src/lora_package.cc`](src/lora_package.cc) by [`CMakeLists.txt`](CMakeLists.txt). The top-level `src/retrieval/dual_read_validator.cpp` is a separate implementation source.

## Data and event flow

1. `DualReadValidator` accepts current/previous index path strings and uses map-supplied ground truth; `RetrievalMetrics` separately exposes `HasRegression()` and `ComputeDeltas()`.
2. Its `ExecuteQuery()` implementation is a placeholder that returns an empty result vector. Consequently, `ValidateQuery()` returns current-result metrics and both result lists, logs a previous-result metric, but does not calculate or return a current-versus-previous delta or perform a live index comparison.
3. The LoRA artifact path serializes package/product metadata, stores/reloads
   records through RocksDB, and fails signature verification closed unless the
   YAML configuration supplies valid pinned-key material. The store binds the
   Ed25519 verifier internally; callers cannot replace it.
4. Product deployment calls the RAG quality controller and then an independent deployment authorization callback. The quality controller is not authorization; no owner/RBAC policy or current composition wiring is claimed.

## Failure domains and edge cases

- Empty ground truth produces zero-valued metric calculations; the placeholder query path also produces empty result sets.
- `ValidateQueryBatch()` logs and rethrows a caught validation exception rather than converting it to a successful batch result.
- Artifact JSON parsing, manifest integrity checks, signature-verifier configuration, and local manifest-store operations are separate error points.
- The umbrella header's documented broader intent must not be read as proof of a production retrieval executor.

## Explicit non-goals

- ANN/vector query execution, hybrid search fusion, RAG context assembly, or model inference.
- Claiming the dual-read validator currently queries actual index backends.
- Treating planned EPIC files or stale README/API statements as implemented behavior.

## Verified cross-module references

The inspected retrieval implementation has no direct include or call edge to index, search, RAG, or LLM modules: the dual-read executor is a stub and the artifact target only links JSON support. Search provides a separate search composition surface; see [search architecture](../search/ARCHITECTURE.md). No runtime integration between these two pages is claimed.

## References

- [Module README](README.md) · [API notes](API.md) · [module roadmap](ROADMAP.md)
- [`dual_read_validator.h`](../../include/retrieval/dual_read_validator.h) · [`retrieval.h`](../../include/retrieval/retrieval.h) · [`include/lora_package.h`](include/lora_package.h)
