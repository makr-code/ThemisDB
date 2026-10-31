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

# Retrieval API Reference

## Scope

This page describes the checked-in artifact declarations in `src/retrieval/include/lora_package.h` and the validator declarations in `include/retrieval/dual_read_validator.h`. These are separate surfaces: the `themis_retrieval` library compiles the LoRA artifact implementation; `dual_read_validator.cpp` is not in that target's source list. `include/retrieval/retrieval.h` is an umbrella placeholder with no includes or declarations.

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

`PortableAdapterProduct` models metadata for a model-bound adapter product. The structure alone does not build or deploy a product. It includes:

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

`LoRAManifestStore` is an in-memory registry backed by `std::unordered_map` containers and a mutex. The API includes:

- `setSignatureVerifier(...)`
- `storePackage(...)` / `loadPackage(...)` / `deletePackage(...)`
- `listPackageIds()` / `listPackagesByStatus(...)`
- `storeProduct(...)` / `loadProduct(...)` / `deleteProduct(...)`
- `listProductIds()` / `listProductsByPackage(...)` / `listProductsByStatus(...)`
- `verifyPackageIntegrity(...)` / `verifyProductIntegrity(...)`
- `exportPackages()` / `importPackages(...)`
- `exportProducts()` / `importProducts(...)`
- `packageCount()` / `productCount()`

CRUD and import/export operate on this process-local store; they do not provide persistence or enforce lifecycle/deployment policies.

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

`DualReadValidator` accepts current and previous index path strings and uses a query-to-judgments map supplied through `SetGroundTruth()`. Its public API includes:

- constructor with current index path and previous index path
- `ValidateQuery(...)`
- `ValidateQueryBatch(...)`
- `GetGroundTruth(...)`
- `SetGroundTruth(...)`

`RetrievalMetrics` also exposes `HasRegression(...)` and `ComputeDeltas(...)`. The private `ExecuteQuery()` currently returns an empty vector. `ValidateQuery()` returns current and previous result vectors and metrics for the current results; it only logs the previous recall and does not return a comparison delta. Missing judgments return an empty vector. Empty results or ground truth produce zero-valued quality metrics. Batch metrics average per-query recall, nDCG, and MRR when the batch is nonempty. The `Compute*AtK` and `ComputeMetrics` helpers are private implementation methods.

## Edge cases and contract expectations

The source establishes the following behavior:

- `from_json()` requires selected identity fields but reads many remaining fields optionally; do not assume every absent field is rejected.
- empty ground-truth input produces zero-valued quality metrics.
- manifest verification fails for missing records, missing manifest hashes, or hash mismatches.
- the optional signature verifier runs only if a verifier callback is configured and the artifact has a non-empty signature. A missing callback or signature does not itself fail verification when the manifest hash matches.
- lifecycle enums are stored/serialized metadata; the store does not enforce transitions or deployment gates.

## Conclusion

The checked-in API is narrow: an in-memory LoRA artifact manifest helper and a dual-read metrics helper whose query executor remains a placeholder. It is not evidence of a complete retrieval runtime or production deployment system.
