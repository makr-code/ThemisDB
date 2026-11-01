---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Modeling Notes

## Purpose

This page explains the model that the retrieval module is currently built around. It is intentionally narrow and source-driven: it maps the implemented artifact lifecycle and quality-validation logic rather than describing speculative production retrieval features.

## Core model entities

### 1. Adapter artifact model

The primary logical object is `LoRAPackage`, defined in `src/retrieval/include/lora_package.h`.

It represents source-artifact metadata, including:

- identity (`package_id`, `name`, `version`),
- architecture compatibility (`supported_architectures`, `target_modules`),
- provenance (`LoRAPackageProvenance`),
- policy (`AdapterUsagePolicy`),
- weight metadata (`weights_path`),
- integrity state (`ArtifactIntegrity`),
- lifecycle and timestamp metadata.

This means the module models adapters as explicit, auditable assets rather than as anonymous runtime blobs.

### 2. Deployable product model

`PortableAdapterProduct` is a data structure representing a model-bound product related to a package. It carries target-model, quantization, file, resource, compatibility, integrity, and status fields. The module does not build the product or control its deployment.

This model intentionally separates:

- the rebuildable source artifact (`LoRAPackage`), and
- the model-bound, deployable form (`PortableAdapterProduct`).

The types distinguish source-package metadata from product metadata; no runtime trust boundary or deployment workflow is implemented here.

### 3. Manifest store model

`LoRAManifestStore::openFromThemisConfig()` loads typed settings from `config/ai_ml/retrieval.yaml` through ThemisDB's path resolver/YAML loader and persists records to RocksDB. Production CRUD/import and reopen require valid Ed25519 signatures over the matching manifest hash using the shared pinned-key verifier. The source-compatible no-argument constructor is isolated non-production mode and rejects all artifact writes and deployment.

### 4. Retrieval-quality model

`RetrievalMetrics` in `include/retrieval/dual_read_validator.h` defines the evaluation surface for retrieval quality. It captures metrics such as:

- `recall_at_10`
- `ndcg_at_10`
- `mrr_at_10`
- aggregate counts and regressions

`DualReadValidator` accepts current/previous index path strings and map-supplied ground truth. Its current query executor returns no results, and `ValidateQuery()` does not return a comparison delta; the type does not currently evaluate real index drift.

## Data relationships

The conceptual graph is simple but intentional:

- `LoRAPackage` -> `PortableAdapterProduct` via `source_package_id`
- `LoRAPackage` -> `ArtifactIntegrity` -> `signed_at`, `weights_hash`, `manifest_hash`
- `LoRAManifestStore` -> RocksDB records + synchronized package/product cache
- `DualReadValidator` -> results + ground truth -> `RetrievalMetrics`

The structures distinguish artifact metadata from product metadata and metric values. `deployProduct()` integrates the RAG quality controller but requires a separate application-provided authorization callback; that callback is not an implemented owner/RBAC policy.

## Lifecycle model

`LoRAPackageStatus` and `AdapterProductStatus` encode status values:

- DRAFT / VALIDATED / DEPRECATED / REVOKED
- BUILDING / READY / DEPLOYED / RETIRED / FAILED

These are representable enum values. CRUD/import cannot set product status DEPLOYED; the module's deployment method sets it only after signature, quality, and external authorization checks. Package validation status itself is metadata, not proof of authorization.

## Invariants

The implementation provides the following limited behaviors:

1. Manifest hashes use a canonical serialization path and can be verified explicitly.
2. The API does not automatically require verification before storing or loading an artifact.
3. Package and product are distinct C++ structures with a source-package identifier relation.
4. Metric computation accepts caller-supplied judgments; missing judgments are empty.
5. The deployment entry point requires READY/VALIDATED metadata, but the owner service remains responsible for authorization and policy.

## Design trade-offs

### Source-of-truth vs convenience

The code provides typed metadata and RocksDB persistence; key lifecycle, owner/RBAC governance, and application deployment wiring are not implemented.

### Evaluation vs speculation

The metric routines evaluate the results passed to them against supplied relevance data. The current validator query path itself is stubbed.

### Trust boundary vs flexibility

The module does not implement owner/RBAC policy. It fails closed unless the owning service supplies the authorization callback.

## Conclusion

The current data model carries artifact metadata and exposes hash/metric helpers. It does not implement the broader provenance enforcement, policy, retrieval execution, or deployment chain described by its fields.
