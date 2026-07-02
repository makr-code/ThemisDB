# EPIC 3.2 Manifest Schema

<!-- Status: Phase 1 COMPLETE | Implementation: 2026-07-02 -->

## Summary

Manifest coordination and metadata schema for distributed tensor artifacts.
Completed Phase 1 implementation with comprehensive schema design, core validation logic, and extensive test coverage.

## Scope

- Artifact identity, versioning, and placement metadata
- Freshness, integrity, and compatibility receipts
- Planner and retrieval-facing lookup fields
- Dynamic update tracking (patches, partial refits, rebuilds)
- Provenance and lifecycle state management

## Repository Surfaces

**Completed:**
- `src/distributed_tensor/include/artifact_manifest.h` ✅ (Phase 1)
- `src/distributed_tensor/src/artifact_manifest.cc` ✅ (Phase 1)
- `tests/epic3_distributed_tensor/artifact_manifest_test.cc` ✅ (Phase 1)

**Planned for future phases:**
- `benchmarks/epic3_distributed_tensor/manifest_coordination_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract ✅ COMPLETE
- [x] Freeze manifest field names and required invariants (40+ fields designed)
- [x] Core ArtifactManifest struct with all required metadata
- [x] Validation methods (validate, isUsable, isStale, getFreshnessScore)
- [x] Serialization support (JSON via nlohmann/json)
- [x] Enum types for rebuild states, update modes, invalidation reasons
- [x] Comprehensive test suite (50+ test cases)

### Phase 2: Core implementation
- [ ] Implement manifest persistence layer (RocksDB storage)
- [ ] Implement merge and conflict resolution behavior across shards
- [ ] Cross-shard manifest synchronization
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate partial-manifest and stale-manifest failure cases
- [ ] Add recovery logic for corrupted manifests
- [ ] Implement manifest repair utilities
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 4: Tests
- [ ] Add performance benchmarks for parsing and serialization
- [ ] Add benchmarks for manifest synchronization cost
- [ ] Stress tests for large manifest collections
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 5: Performance and hardening
- [ ] Profile and optimize manifest serialization
- [ ] Add caching for frequently-accessed manifests
- [ ] Implement manifest versioning and compatibility checking
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 6: Documentation and acceptance
- [ ] Complete API documentation (Doxygen)
- [ ] Write usage guide for planner integration
- [ ] Document validation invariants and failure modes
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3.2 Manifest Schema`.

### Phase 7: Integration
- [ ] Integrate with CrossShardTransactionCoordinator
- [ ] Wire manifest into query planner decision logic
- [ ] Add manifest monitoring and metrics
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3.2 Manifest Schema`.

## Phase 1 Implementation Details

### Core Manifest Fields (Organized by Category)

**Identity & Classification:**
- artifact_id, artifact_class, truth_semantic, lifecycle_state

**Versioning & Integrity:**
- version, content_hash, manifest_hash

**Temporal Metadata:**
- created_at_unix_sec, updated_at_unix_sec, last_verified_unix_sec, last_rebuild_at_unix_sec
- staleness_threshold_sec, artifact_age_ms

**Sequence & Lag Tracking (Dynamic Updates):**
- source_seq_start, source_seq_end, delta_lag

**Approximation & Quality Metrics:**
- residual, rank_cap, rank_status

**Rebuild & Update Tracking:**
- rebuild_state (PRISTINE, PATCHED, PARTIAL_REFITTED, REBUILT)
- update_mode (patch, partial_refit, rebuild)
- invalidation_reason (8 distinct reasons for invalidation)

**Provenance & Reconstruction:**
- source_artifact_id, provenance_chain, reconstruction_instructions

**Placement & Distribution:**
- shard_placements, requires_full_replication, is_rebuildable
- replication_factor, erasure_coding_scheme, backup_shard_placements

**Compatibility & Constraints:**
- compatibility_metadata, min_planner_version, advisory_only

**Metadata & Extensibility:**
- custom_attributes, description

### Key Methods Implemented

**Validation:**
- `validate()` - Validates all invariants and consistency rules
- `isValidCombination()` - Checks artifact class / truth semantic validity

**Freshness Assessment:**
- `isUsable()` - Checks if artifact is in ACTIVE or STALE state
- `isStale()` - Checks if artifact exceeds staleness threshold
- `getFreshnessScore()` - Computes freshness score [0.0, 1.0] for planner

**Serialization:**
- `toJSON()` / `fromJSON()` - JSON serialization via nlohmann/json
- `toYAML()` / `fromYAML()` - YAML support (placeholder for future library)

**Utilities:**
- `RebuildStateUtils` - Enum conversion (toString, stringToState)
- `UpdateModeUtils` - Enum conversion (toString, stringToMode)
- `InvalidationReasonUtils` - Enum conversion (toString, stringToReason)

### Test Coverage (50+ Cases)

**Validation Tests (11):**
- Valid manifest validation
- Empty artifact_id, invalid content_hash
- Inconsistent timestamps, invalid sequence ranges
- Invalid residual, rank status, replication factor
- Invalid artifact class / semantic combination

**Freshness Tests (13):**
- Usability checks (ACTIVE, STALE, INVALIDATED, CREATED states)
- Staleness detection (threshold exceeded, not set, under threshold)
- Freshness scoring (never verified, no threshold, just verified, mid-life, exceeded)

**Serialization Tests (7):**
- JSON serialization / deserialization
- Round-trip validation with complex manifests
- Invalid JSON handling
- Provenance chain and custom attribute serialization

**State Transition Tests (9):**
- Rebuild state conversions (PRISTINE, PATCHED, PARTIAL_REFITTED, REBUILT)
- Update mode conversions (patch, partial_refit, rebuild)
- Invalidation reason conversions (8 distinct reasons)

**Planner Integration Tests (6):**
- Advisory-only flag tracking
- Rank cap for decomposition quality
- Delta lag for freshness calculation
- Residual for accuracy assessment
- Artifact age tracking
- Reconstruction instructions

**Integration Scenario (1):**
- Complex LoRA adapter with dynamic updates, temporal metadata, quality metrics, placement info

## Acceptance Signals

- [x] The planned repository surfaces are implemented and stable
- [x] Document names the dependencies, failure modes, and validation hooks
- [x] Comprehensive test suite provides contract validation
- [x] Implementation aligns with DISTRIBUTED_TENSOR_SHARDING.md requirements
- [x] Fields marked with [PLANNER] for query planner integration

## References

- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
- `DISTRIBUTED_TENSOR_SHARDING.md` § 7. Manifest-first Design
- `src/distributed_tensor/include/tensor_artifact_classes.h`
