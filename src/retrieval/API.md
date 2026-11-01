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

`LoRAManifestStore(RuntimeConfig)` is the production RocksDB-backed registry.
The canonical runtime settings are loaded from `config/ai_ml/retrieval.yaml`
through `themis::config::ConfigPathResolver` and yaml-cpp. The YAML loader
requires a non-empty database path and at least one valid Ed25519 pin. Each pin
contains a fingerprint, the raw public key in hex, and an operator-readable
label. The fingerprint must match the key bytes; malformed, missing, duplicate,
or empty pins reject startup.

Production composition should call `LoRAManifestStore::openFromThemisConfig()`
for storage and artifact verification, or load `RuntimeConfig` first when it
must also attach the quality controller and owning-service authorization
callback. Those latter dependencies are not serialized into YAML. The checked-in
`config/ai_ml/retrieval.yaml` intentionally has no trusted keys and therefore
fails closed until an administrator provisions the real pins.

For source compatibility, `LoRAManifestStore()` remains an isolated
non-production fixture. It has no database, keys, or verifier and rejects every
artifact write/import and deployment.

### Migration from callback verification

The former `setSignatureVerifier()` injection API is intentionally removed:
accepting arbitrary verifier callbacks would let production callers bypass the
configured trust pins. Configure active Ed25519 public keys in
`config/ai_ml/retrieval.yaml` and construct the persistent store with
`openFromThemisConfig()` instead. Existing default-constructed test and
benchmark fixtures remain source-compatible, but their writes now fail closed;
migrate any fixture that needs valid artifact writes to a temporary RocksDB
path and a test-only Ed25519 key pair through `RuntimeConfig`.

The API includes:

- `RuntimeConfig::loadFromYaml(...)` / `RuntimeConfig::loadFromThemisConfig()`
- `openFromThemisConfig()`
- `storePackage(...)` / `loadPackage(...)` / `deletePackage(...)`
- `listPackageIds()` / `listPackagesByStatus(...)`
- `storeProduct(...)` / `loadProduct(...)` / `deleteProduct(...)`
- `listProductIds()` / `listProductsByPackage(...)` / `listProductsByStatus(...)`
- `verifyPackageIntegrity(...)` / `verifyProductIntegrity(...)`
- `deployProduct(...)`
- `exportPackages()` / `importPackages(...)`
- `exportProducts()` / `importProducts(...)`
- `packageCount()` / `productCount()`

Every production package/product write and import is accepted only when the
content hash matches and a non-empty Base64 Ed25519 signature verifies over the
exact `manifest_hash` using the active pinned public key whose fingerprint
matches `signer_id`. The store binds verification internally to the shared
`themis::plugins::pinned_ed25519` primitive used by `SignedPluginRepository`;
callers cannot replace or disable it. Invalid/unsigned persisted entries cause
store construction to fail on reopen.

`deployProduct()` accepts only a READY product with a VALIDATED source package,
valid signatures, non-empty finite quality metrics, and an explicitly supplied
quality controller that returns `kAllow`. It separately requires a non-empty
deployment authorization callback and its approval. Missing/throwing/denying
authorization, a quality warning/deny, a missing quality controller, invalid
metrics, or persistence errors fail closed. The callback is only an integration
boundary: this module does not define or implement owner/RBAC policy. Ordinary
`storeProduct()` and `importProducts()` reject the DEPLOYED status; the gated
deployment method is the sole module API that sets it.

Production callers must use the canonical YAML config and inject the quality
gate and independent deployment authorization callback through the typed
runtime config when deployment is required. Existing default-constructed
test/local callers remain source-compatible, but that overload has no trust
configuration and cannot accept artifacts.

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
- missing signatures, an absent verifier, an unpinned signer, a non-Ed25519 algorithm label, a hash mismatch, or verifier rejection fails artifact verification.
- lifecycle status/update timestamps do not alter the signed content hash. DEPLOYED is rejected by ordinary product CRUD/import and can only be set by the gated deploy method.
- a quality warning is not accepted as deployment allow; deployment also fails if the independent authorization callback or quality controller is absent.

## Conclusion

The checked-in API is a persistent LoRA artifact registry with fail-closed signed-artifact checks and a quality-plus-authorization deployment boundary, alongside a dual-read metrics helper whose query executor remains a placeholder. It is not evidence of an integrated deployment service, owner/RBAC policy, or complete retrieval runtime.
