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

`LoRAManifestStore` holds `LoRAPackage` and `PortableAdapterProduct` values in in-memory `std::unordered_map` containers protected by a `std::mutex`. It provides CRUD/list, import/export, and explicit verification methods. It does not persist data or enforce state consistency beyond its map operations.

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
- `LoRAManifestStore` -> in-memory package map + product map
- `DualReadValidator` -> results + ground truth -> `RetrievalMetrics`

The structures distinguish artifact metadata from product metadata and metric values; they do not implement runtime deployment.

## Lifecycle model

`LoRAPackageStatus` and `AdapterProductStatus` encode status values:

- DRAFT / VALIDATED / DEPRECATED / REVOKED
- BUILDING / READY / DEPLOYED / RETIRED / FAILED

These are representable enum values. The store can list by status, but it does not enforce transitions or determine whether an artifact is safe to deploy.

## Invariants

The implementation provides the following limited behaviors:

1. Manifest hashes use a canonical serialization path and can be verified explicitly.
2. The API does not automatically require verification before storing or loading an artifact.
3. Package and product are distinct C++ structures with a source-package identifier relation.
4. Metric computation accepts caller-supplied judgments; missing judgments are empty.
5. The code defines statuses but does not implement a lifecycle state machine.

## Design trade-offs

### Source-of-truth vs convenience

The code provides typed metadata and in-memory CRUD; durable source-of-truth behavior and governance are not implemented.

### Evaluation vs speculation

The metric routines evaluate the results passed to them against supplied relevance data. The current validator query path itself is stubbed.

### Trust boundary vs flexibility

No operator- or deployment-policy controls are implemented by this module.

## Conclusion

The current data model carries artifact metadata and exposes hash/metric helpers. It does not implement the broader provenance enforcement, policy, retrieval execution, or deployment chain described by its fields.
