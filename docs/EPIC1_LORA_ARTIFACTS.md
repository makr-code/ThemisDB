# EPIC 1.4 LoRA Artifacts

<!-- Status: current | Phase 3 delivered | validated: 2026-07-16 -->

## Summary

LoRAPackage and PortableAdapterProduct artifact model for adapter lifecycle management.

## Scope

- Portable adapter package metadata
- Compatibility between base model, adapter, and retrieval path
- Artifact references consumed by model-switch workflow

## Repository Surfaces

- `src/retrieval/include/lora_package.h` — LoRAPackage + PortableAdapterProduct + LoRAManifestStore API
- `src/retrieval/src/lora_package.cc` — Implementation (serialization, integrity, store CRUD)
- `tests/epic1_retrieval/lora_package_test.cc` — 37 GTest cases
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
- [x] 37 GTest cases in `tests/epic1_retrieval/lora_package_test.cc`
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

## References

- `docs/EPIC1_MODEL_SWITCH.md`
- `docs/EPIC3_ARTIFACT_CLASSES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`

