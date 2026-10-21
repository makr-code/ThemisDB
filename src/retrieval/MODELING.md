---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Modeling Notes

## Purpose

This page explains the model that the retrieval module is currently built around. It is intentionally narrow and source-driven: it maps the implemented artifact lifecycle and quality-validation logic rather than describing speculative production retrieval features.

## Core model entities

### 1. Adapter artifact model

The primary logical object is `LoRAPackage`, defined in `src/retrieval/include/lora_package.h`.

It represents a source-of-truth adapter package with:

- identity (`package_id`, `name`, `version`),
- architecture compatibility (`supported_architectures`, `target_modules`),
- provenance (`LoRAPackageProvenance`),
- policy (`AdapterUsagePolicy`),
- weight metadata (`weights_path`),
- integrity state (`ArtifactIntegrity`),
- lifecycle and timestamp metadata.

This means the module models adapters as explicit, auditable assets rather than as anonymous runtime blobs.

### 2. Deployable product model

`PortableAdapterProduct` is the deployable derivative of a package. It binds the adapter to a target model and quantization environment, and it carries the product-level resource and compatibility information required for controlled deployment.

This model intentionally separates:

- the rebuildable source artifact (`LoRAPackage`), and
- the model-bound, deployable form (`PortableAdapterProduct`).

That is a clean trust boundary: a deployable product is not treated as equivalent to the original source artifact.

### 3. Manifest store model

`LoRAManifestStore` is the registry layer that persists `LoRAPackage` and `PortableAdapterProduct` objects and provides CRUD and verification functions. The store is thread-safe via a `std::mutex`, and its main job is to keep package/product state consistent, queryable, and integrity-aware.

### 4. Retrieval-quality model

`RetrievalMetrics` in `src/retrieval/include/retrieval/dual_read_validator.h` defines the evaluation surface for retrieval quality. It captures metrics such as:

- `recall_at_10`
- `ndcg_at_10`
- `mrr_at_10`
- aggregate counts and regressions

`DualReadValidator` compares current retrieval results against a previously known index or a cached ground-truth set. This gives the module a concrete evaluation model for index drift and quality degradation.

## Data relationships

The conceptual graph is simple but intentional:

- `LoRAPackage` -> `PortableAdapterProduct` via `source_package_id`
- `LoRAPackage` -> `ArtifactIntegrity` -> `signed_at`, `weights_hash`, `manifest_hash`
- `LoRAManifestStore` -> package collection + product collection
- `DualReadValidator` -> results + ground truth -> `RetrievalMetrics`

This modeling preserves the distinction between provenance, runtime deployment, and measurement.

## Lifecycle model

`LoRAPackageStatus` and `AdapterProductStatus` encode the observable runtime state machine:

- DRAFT / VALIDATED / DEPRECATED / REVOKED
- BUILDING / READY / DEPLOYED / RETIRED / FAILED

These states are not decorative. They are part of the governance contract because the source code uses status checks to determine whether an artifact is safe to deploy or should be treated as invalid.

## Invariants

The implementation enforces a small but important set of invariants:

1. Manifest hashing is deterministic for canonical JSON serialization.
2. Package integrity must be verified before a package is treated as deployable.
3. The source artifact remains distinct from any model-bound output.
4. Retrieval quality must be measured against explicit judgments, not assumptions.
5. State transitions are explicit and auditable.

## Design trade-offs

### Source-of-truth vs convenience

The design prefers a source-of-truth model for adapter packaging. That adds implementation and governance cost but prevents ambiguous runtime behavior when a package is imported, signed, or redeployed.

### Evaluation vs speculation

The dual-read validator is intentionally narrow: it evaluates retrieved results against known relevance data and reports metrics, rather than pretending to know the full search semantics of every workload. This keeps the model honest.

### Trust boundary vs flexibility

The module is strict about operator- and policy-level controls. It does not assume that a model or index is valid just because it exists on disk or is reachable over a network.

## Conclusion

The retrieval module is modeled around explicit ownership, integrity, and measurement. The most important idea is that retrieval behavior is not merely a search algorithm; it is also a chain of provenance, policy compliance, and evidence-based validation.
