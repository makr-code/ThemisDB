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

# Retrieval Implementation Notes

## Purpose

This document describes the implementation behavior that is actually present in `src/retrieval` today. It is intentionally grounded in the source and avoids claiming that the module is a complete retrieval platform.

## Implemented surfaces

### LoRA artifact lifecycle

`src/retrieval/include/lora_package.h` defines the artifact model exposed by the `themis_retrieval` target. It includes:

- `LoRAPackageStatus` with `DRAFT`, `VALIDATED`, `DEPRECATED`, and `REVOKED`
- `AdapterProductStatus` with `BUILDING`, `READY`, `DEPLOYED`, `RETIRED`, and `FAILED`
- `AdapterUsagePolicy`, `LoRAPackageProvenance`, and `ArtifactIntegrity`
- `LoRAManifestStore` CRUD and integrity verification APIs

`src/retrieval/src/lora_package.cc` implements the artifact store and serialization; the target also compiles the existing RAG deployment quality controller so the deployment method uses that implementation. The relevant details are:

- canonical JSON serialization using `nlohmann::json`
- SHA-256 hashing via an internal portable implementation
- deterministic field encoding and manifest hashing
- `from_json` checks selected required fields; many fields are optional and default-initialized
- verification helpers that compare computed hashes against expected values
- mutex-protected package/product caches backed by synchronous RocksDB CRUD/import/export
- reopen-time record recovery; malformed persisted records fail construction
- ConfigPathResolver/yaml-cpp loading from `config/ai_ml/retrieval.yaml`,
  including Ed25519 public-key pin validation and an internally bound shared
  `SignedPluginRepository` verifier
- production writes/imports and reopen recovery that reject unsigned, malformed,
  hash-mismatched, or cryptographically invalid artifacts
- product deployment requiring signed artifacts, a quality decision of `kAllow`, and an independent external authorization callback

### Dual-read regression validation

`src/retrieval/dual_read_validator.cpp` is a separate implementation, not listed in the `themis_retrieval` target. It defines metric helpers and `DualReadValidator`; its query execution does not connect to an index.

The validator computes:

- `ComputeRecallAtK`
- `ComputeNdcgAtK`
- `ComputeMrrAtK`
- per-query metrics, nonempty-batch average metrics, and separate `RetrievalMetrics` delta/regression helpers

`ExecuteQuery()` returns an empty result vector. `ValidateQuery()` calculates metrics for current results, logs previous-result recall, but does not calculate or return current-versus-previous deltas.

## Key invariants

The current implementation strongly implies the following correctness invariants:

1. Manifest hash verification recomputes the canonical JSON hash and compares it with a non-empty stored hash.
2. `from_json()` rejects some missing required fields, but does not require every metadata field.
3. Missing ground-truth entries return an empty vector; empty metric inputs produce zero-valued metrics.
4. Status metadata is not authorization. Ordinary CRUD/import cannot set DEPLOYED; only the gated deployment method does.
5. Signature callbacks, quality controllers, and authorization callbacks are explicit composition dependencies; absent deployment authorization denies deployment.

## Error-handling model

### Artifact handling

`from_json()` throws for missing fields it explicitly requires and for invalid typed/status data. Several metadata fields are optional. `storePackage()`/`storeProduct()` only reject empty IDs. Hash verification is an explicit caller-invoked operation, not an automatic CRUD gate.

### Retrieval comparison

The metric routines return zero for empty ground truth and empty result sets. Since `ExecuteQuery()` is a placeholder, this does not provide evidence about index quality.

## Source-level constraints

The actual code indicates that the module is intentionally conservative:

- `DualReadValidator::ExecuteQuery()` is a placeholder and returns empty result sets until a concrete index path is implemented.
- `GetGroundTruth()` warns and returns empty data when no judgment set is cached.
- tests and benchmarks in their similarly named directories include local mocks, stubs, and synthetic loops, not production hybrid retrieval or GPU execution.

This matters because the module's documented behavior is currently a measured, staged maturity level rather than a fully launched retrieval platform.

## Implementation recommendations

When touching this module, keep the following priorities:

- preserve integrity and serialization determinism,
- preserve explicit error handling,
- keep policy and verification state auditable,
- avoid claiming end-to-end retrieval readiness beyond the source-backed evidence,
- document any new “production” path with matching roadmap and runbook updates.

## Conclusion

The artifact library and metric helper are limited components. They do not constitute a complete retrieval engine, persistent registry, or production trust/deployment system.
