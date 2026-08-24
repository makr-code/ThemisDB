## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 3.4 Integrity Model

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Integrity verification, Merkle structures, and receipt semantics.

## Scope

- Hash tree or receipt model for artifact verification
- Validation timing on ingest, query path, and background audit
- Planner-facing degradation and block rules on integrity failure

## Planned Repository Surfaces

- `src/distributed_tensor/include/integrity_verification.h`
- `src/distributed_tensor/src/integrity_verification.cc`
- `tests/epic3_distributed_tensor/integrity_verification_test.cc`
- `benchmarks/epic3_distributed_tensor/integrity_verification_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract ✅ COMPLETE
- [x] Define content-hash model (SHA-256, manifest-embedded)
- [x] Define Merkle tree fragment verification (O(log N) proofs)
- [x] Define receipt-chain semantics (tamper-evident linking)
- [x] Define verification state machine (UNVERIFIED → VERIFIED → CORRUPT/STALE)
- [x] Define provenance verification hooks and audit trail callbacks
- [x] Document integration with manifest, placement, and recovery subsystems

### Phase 2: Core implementation ✅ COMPLETE
- [x] Implement `integrity_verification.h/cc` with production-grade hashing, validation, and manifest-facing serialization
- [x] Document when queries may proceed with cached receipts versus fresh verification
- [x] Implement deterministic JSON serialization for proof computation

#### Cached receipts vs. fresh verification

- **Cached receipt is sufficient** when the query consumes immutable, previously verified artifact bytes and only needs audit metadata or historical lineage context.
- **Fresh full verification is required** when artifact bytes are reloaded from shard storage, when manifest content or placement metadata changed, or when any upstream recovery/rebuild event invalidated prior trust.
- **Fresh fragment verification is required** when the planner fetches only a subset of fragments from remote shards and the full-artifact content hash has not been recomputed in the current read path.
- **Receipt-chain verification stays off the hot path** unless the caller explicitly needs compliance or tamper-evidence guarantees beyond the current artifact payload.

### Phase 3: Error handling and edge cases ✅ COMPLETE
- [x] Implement corruption and tampering detection
- [x] Handle partial-receipt and stale-receipt edge cases
- [x] Implement recovery coordination with EPIC 3.5

### Phase 4: Tests ✅ COMPLETE
- [x] Write unit tests for SHA-256 computation and validation
- [x] Write tests for Merkle proof verification (happy path, invalid proofs, tampering)
- [x] Write tests for receipt chain verification (genesis, appends, tampering)
- [x] Write integration tests for verification workflow

### Phase 5: Performance and hardening
- [ ] Benchmark SHA-256 verification on various artifact sizes
- [ ] Profile Merkle proof generation and verification
- [ ] Optimize with hardware acceleration (SHA-NI, AVX-512)
- [ ] Ensure alignment with graph provenance performance targets

### Phase 6: Documentation and acceptance
- [ ] Tie acceptance to measured verification latencies
- [ ] Document compliance and audit trail support
- [ ] Complete example workflows and failure mode analysis

### Phase 7: Integration
- [ ] Wire integrity_verification.h/cc into CMakeLists.txt
- [ ] Integrate with ArtifactManifest (manifest_schema.h/cc)
- [ ] Integrate with ShardPlacementStrategy (shard_placement.h/cc)
- [ ] Integrate with RecoveryManager (recovery_manager.h/cc)
- [ ] Wire into distributed planner for query-path verification

## Acceptance Signals

Phase 1 is complete when:

- [x] The API contracts for ContentHash, MerkleProof, VerificationReceipt are stable
- [x] The verification state machine is fully documented
- [x] Provenance hook and audit trail interfaces are defined
- [x] Integration patterns with manifest, placement, and recovery are documented
- [x] Example verification workflows are provided
- [x] Failure modes and recovery strategies are enumerated
- [x] The following files are created and reviewed:
  - `src/distributed_tensor/include/integrity_verification.h`
  - `src/distributed_tensor/src/integrity_verification.cc`
  - `docs/EPIC3_INTEGRITY_MODEL_IMPLEMENTATION.md`
  - `docs/EPIC3_MERKLE_RECEIPT_INTEGRATION.md`

## References

- `docs/EPIC1_GRAPH_VALIDATION.md`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
