# EPIC 1.4 LoRA Artifacts

<!-- Status: current | phase-4 provenance layer delivered | validated: 2026-07-16 -->

## Summary

Planning document for LoRAPackage and PortableAdapterProduct models.

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

## Planned Repository Surfaces

- `src/retrieval/include/lora_package.h`
- `src/retrieval/src/lora_package.cc`
- `tests/epic1_retrieval/lora_package_test.cc`
- `benchmarks/epic1_retrieval/lora_loading_bench.cc`

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

