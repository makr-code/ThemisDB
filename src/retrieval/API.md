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

# Retrieval API Reference

## Scope

This document covers the API surfaces that are actually present in the current source tree for the retrieval module. It focuses on the public contract in `src/retrieval/include/lora_package.h` and the validation helper in `src/retrieval/include/retrieval/dual_read_validator.h`.

## Public artifact contract

### `LoRAPackage`

`LoRAPackage` is the canonical adapter artifact type. It stores:

- package identity (`package_id`, `name`, `version`, `description`),
- compatibility and targeting (`supported_architectures`, `target_modules`, `lora_rank`, `lora_alpha`),
- provenance (`LoRAPackageProvenance`),
- policy (`AdapterUsagePolicy`),
- weight reference (`weights_path`),
- integrity (`ArtifactIntegrity`),
- lifecycle metadata (`status`, `created_at`, `updated_at`).

The type exposes serialization and helper methods:

- `to_json()`
- `from_json(const json&)`
- `computeManifestHash()`
- `supportsArchitecture(const std::string&)`
- `statusToString()`
- `statusFromString(const std::string&)`

### `PortableAdapterProduct`

`PortableAdapterProduct` models the deployable, model-bound adapter result. It includes:

- product identity, source package linkage, and runtime binding
- target model architecture and quantization details
- file descriptor and size metadata
- resource envelope information such as `max_context_length` and `memory_requirement_mb`
- product lifecycle state

The class exposes:

- `to_json()`
- `from_json(const json&)`
- `statusToString()`
- `statusFromString(const std::string&)`

### `LoRAManifestStore`

`LoRAManifestStore` provides the registry surface for persisted artifact state. The API includes:

- `setSignatureVerifier(...)`
- `storePackage(...)` / `loadPackage(...)` / `deletePackage(...)`
- `listPackageIds()` / `listPackagesByStatus(...)`
- `storeProduct(...)` / `loadProduct(...)` / `deleteProduct(...)`
- `listProductIds()` / `listProductsByPackage(...)` / `listProductsByStatus(...)`
- `verifyPackageIntegrity(...)` / `verifyProductIntegrity(...)`
- `exportPackages()` / `importPackages(...)`
- `exportProducts()` / `importProducts(...)`
- `packageCount()` / `productCount()`

This is the operational API for integrity-aware package management.

## Integrity helper API

`IntegrityHelper` exposes a minimal hashing and verification surface:

- `sha256Hex(const uint8_t* data, size_t size)`
- `sha256Hex(const std::string& input)`
- `verifyHash(const uint8_t* data, size_t size, const std::string& expected_hex)`
- `verifyHash(const std::string& input, const std::string& expected_hex)`

These functions are the low-level mechanism that supports package integrity validation.

## Retrieval evaluation API

### `RetrievalMetrics`

`RetrievalMetrics` contains retrieval quality measurements for ranking-based evaluation, including:

- `recall_at_10`
- `ndcg_at_10`
- `mrr_at_10`
- `total_relevant`
- `relevant_count`

It exposes:

- `HasRegression(const RetrievalMetrics&, double tolerance_pp = 2.0)`
- `ComputeDeltas(const RetrievalMetrics&)`

### `DualReadValidator`

`DualReadValidator` compares a current index against a previous one using a shared query and ground-truth state.

The API includes:

- constructor with current index path and previous index path
- `ComputeRecallAtK(...)`
- `ComputeNdcgAtK(...)`
- `ComputeMrrAtK(...)`
- `ExecuteQuery(...)`
- `ComputeMetrics(...)`
- `ValidateQuery(...)`
- `ValidateQueryBatch(...)`
- `GetGroundTruth(...)`
- `SetGroundTruth(...)`

This is a validation API, not a broad retrieval execution framework.

## Edge cases and contract expectations

The source is clear about the following expectations:

- missing or malformed JSON should fail rather than silently coerce data,
- empty ground-truth sets produce zero-valued metric output rather than false positives,
- all verification paths are explicit and inspectable,
- lifecycle status and deployment gating are part of the contract, not incidental metadata.

## Conclusion

The module's active API is narrow but disciplined: it exposes the package lifecycle and integrity contract, plus the retrieval-quality regression check needed to compare retrieval behavior across index versions.
