# EPIC 1.4 LoRA Artifacts

<!-- Status: current | Phase 3 delivered | validated: 2026-07-16 -->
<!-- Status: current | phase-4 provenance layer delivered | validated: 2026-07-16 -->

## Summary

LoRAPackage and PortableAdapterProduct artifact model for adapter lifecycle management.

Phase 4 (HashChain & Provenance Layer, issue #5417) has been delivered.
The implementation resides in:

- `include/llm/lora_framework/lora_package_provenance.h`
- `src/llm/lora_framework/lora_package_provenance.cpp`
- `tests/lora/test_lora_package_provenance.cpp`

## Scope

- Portable adapter package metadata
- Compatibility between base model, adapter, and retrieval path
- Artifact references consumed by model-switch workflow
- Hash-based ProvenanceHashLedger with parent-hash linkage (Phase 4)
- ReceiptChain and ReceiptManifest per distribution event (Phase 4)
- ShardLedgerEntry for RAID-Merkle-Proof integration (Phase 4)

## Repository Surfaces

- `src/retrieval/include/lora_package.h` — LoRAPackage + PortableAdapterProduct + LoRAManifestStore API
- `src/retrieval/src/lora_package.cc` — Implementation (serialization, integrity, store CRUD)
- `tests/epic1_retrieval/lora_package_test.cc` — 55 GTest cases
- `benchmarks/epic1_retrieval/lora_loading_bench.cc` — Google Benchmark suite (12 scenarios)

## Artifact Classes

### LoRAPackage
Source-of-truth artifact, rebuildable:
- Identity: `package_id`, `name`, `version`, `description`
- Architecture: `supported_architectures`, `lora_rank`, `lora_alpha`, `target_modules`
- Lineage: `parent_package_id`, `provenance` (LoRAPackageProvenance)
- Policy: `policy` (AdapterUsagePolicy — license, restrictions, allowed base models)
- Integrity: `integrity` (ArtifactIntegrity — SHA-256 hash + Ed25519/ECDSA signature)
- Status: `DRAFT → VALIDATED → DEPRECATED | REVOKED`

### PortableAdapterProduct
Deployable, model-bound artifact:
- Identity: `product_id`, `name`, `version`, `source_package_id`
- Binding: `target_base_model_id`, `target_model_architecture`, `quantization`
- Format: `format`, `file_path`, `file_size_bytes`
- Runtime: `max_context_length`, `memory_requirement_mb`
- Compatibility: `compatible_model_versions`
- Status: `BUILDING → READY → DEPLOYED → RETIRED | FAILED`

### LoRAManifestStore
Thread-safe in-memory manifest registry:
- CRUD for both artifact types (`storePackage`, `loadPackage`, `deletePackage`, etc.)
- Integrity verification via SHA-256 manifest hash
- Optional cryptographic signature callback (`setSignatureVerifier`)
- Bulk export/import via JSON arrays (`exportPackages`, `importPackages`)

## Phase 4 Deliverables (issue #5417) — COMPLETE

### Key data structures

| Type | Purpose |
|---|---|
| `LoRAPackage` | Source-oriented rebuildable adapter artifact with hash chain |
| `AdapterProduct` | Model-bound deployable product, linked to a LoRAPackage |
| `DistributionReceipt` | Immutable per-event receipt with parent_receipt_hash chain linkage |
| `ReceiptManifest` | Batch-event manifest with Merkle root over all receipt hashes |
| `ShardLedgerEntry` | Per-shard RAID-Merkle-Proof record with chain linkage |
| `ReceiptChain` | Ordered, tamper-evident chain of DistributionReceipts |
| `ProvenanceHashLedger` | Thread-safe manager for all Phase-4 provenance artefacts |

### Design properties

- All hash chains use SHA-256 (via OpenSSL).
- Every artifact carries `package_hash` / `product_hash` / `receipt_hash` /
  `entry_hash` computed from content fields; parent hashes link versions in
  a tamper-evident chain.
- `ReceiptManifest.merkle_root` is a standard binary Merkle tree root over all
  `receipt_hash` values, enabling efficient inclusion proofs.
- `verifyReceiptChain()` and `verifyShardLedger()` replay hash recomputation
  end-to-end; any break in the chain is detected immediately.
- `exportAuditPath()` produces a JSON document covering packages, products,
  receipt chain, manifests, and shard ledger — ready for CLI / REST surfaces.
- Thread-safe: ProvenanceHashLedger uses a single mutex; all public methods
  are safe to call from multiple threads concurrently.

### Test coverage

`tests/lora/test_lora_package_provenance.cpp` — 50+ GTest cases:
- JSON round-trips for all structs
- Content hash stability and sensitivity
- Multi-version package/product chain invariants
- Receipt chain linkage and verification
- ReceiptManifest Merkle root (0, 1, 2, N receipts)
- Tamper detection: Merkle root, manifest hash, receipt hash, shard hash
- ShardLedger chain linkage and verification
- Audit path export completeness
- Cross-artifact isolation
- Fuzzing-style sequential integrity: 50-step package chain, 30-step receipt
  chain, 32-receipt manifest, 16-shard ledger

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Separate packaging metadata from runtime loading behavior
- [x] Artifact taxonomy: LoRAPackage (source) vs. PortableAdapterProduct (deployable)

### Phase 2: Core implementation
- [x] Integrity, provenance, and compatibility requirements specified and implemented
- [x] JSON serialization and deserialization for all types
- [x] LoRAManifestStore CRUD with thread-safety

### Phase 3: Error handling and edge cases
- [x] Incompatible ranks, quantization, or base-model drift addressed via validation
- [x] from_json throws std::invalid_argument on missing required fields
- [x] verifyIntegrity returns false on empty hash, tampered fields, or bad signature
- [x] Malformed import entries are skipped gracefully

### Phase 4: Tests
- [x] 55 GTest cases in `tests/epic1_retrieval/lora_package_test.cc`
- [x] Coverage: serialization, status lifecycle, integrity hash, signature verifier, CRUD, bulk export/import, lifecycle integration

### Phase 5: Performance and hardening
- [x] Benchmark suite in `benchmarks/epic1_retrieval/lora_loading_bench.cc`
- [x] Portable SHA-256 (no crypto dependency) for manifest hashing
- [x] Thread-safe store using std::mutex

### Phase 6: Documentation and acceptance
- [x] Full Doxygen documentation on all public API surfaces
- [x] ROADMAP updated

### Phase 7: Integration
- [x] `src/retrieval/CMakeLists.txt` activates `themis_retrieval` library target
- [x] `tests/epic1_retrieval/CMakeLists.txt` activates GTest targets
- [x] `cmake/CMakeLists.txt` adds `src/retrieval` subdirectory

## Acceptance Signals

- [x] Repository surfaces implemented and documented
- [x] Tests cover serialization, integrity, lifecycle, and error cases
- [x] Benchmark suite covers all key operations
- [x] CMake targets enabled
- [x] Separate packaging metadata from runtime loading behavior (LoRAPackage vs AdapterProduct)
- [x] Hash-chain design reviewed; parent_hash linkage pattern established

### Phase 2: Core implementation
- [x] Integrity, provenance, and compatibility metadata in LoRAPackage / AdapterProduct
- [x] ProvenanceHashLedger with full chain management

### Phase 3: Error handling and edge cases
- [x] Document edge cases: empty artifact_id throws, genesis chain seeding,
      Merkle root for zero / one / N receipts

### Phase 4: Tests
- [x] Regression tests for all hash-chain invariants (see above)
- [x] Fuzzing-style integrity tests (long chains, large manifests)

### Phase 5: Performance and hardening
- [ ] Reserve performance checks for loading and activation costs
- [ ] Benchmark hash-chain operations for large receipt chains (Target: Q4 2026)

### Phase 6: Documentation and acceptance
- [x] Phase 4 provenance layer documented in this file
- [ ] Retrieval-layer integration documented once lora_package.h is implemented

### Phase 7: Integration
- [ ] Wire planned retrieval surfaces into nearest CMake and cross-epic integration
      checkpoints for `EPIC 1.4 LoRA Artifacts`.
- [ ] REST endpoint for `exportAuditPath()` JSON output

## References

- `docs/EPIC1_MODEL_SWITCH.md`
- `docs/EPIC3_ARTIFACT_CLASSES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
- Issue: `https://github.com/makr-code/ThemisDB/issues/5417`

