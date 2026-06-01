# EPIC 3 Architecture

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Distributed tensor artifact system design for placement, integrity, and recovery.

## Scope

- Artifact taxonomy and manifest coordination
- Factorization-aware placement and storage/network constraints
- Integrity verification, receipts, and rebuild orchestration

## Planned Repository Surfaces

- `src/distributed_tensor/include/tensor_artifact_classes.h`
- `src/distributed_tensor/include/artifact_manifest.h`
- `src/distributed_tensor/include/recovery_manager.h`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define the distributed artifact vocabulary before encoding formats are fixed
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 2: Core implementation
- [ ] Document ownership boundaries between planner, storage, and recovery code
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 3: Error handling and edge cases
- [ ] Reserve integrity and recovery metadata needed by EPIC 1 and EPIC 2
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 4: Tests
- [ ] Describe how shard-local and federated retrieval consume manifests
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 5: Performance and hardening
- [ ] Record acceptance signals for survivability and portability
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 6: Documentation and acceptance
- [ ] Record acceptance signals for survivability and portability
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 3 Architecture`.

### Phase 7: Integration
- [ ] Record acceptance signals for survivability and portability
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 3 Architecture`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC2_QUERY_PLANNER.md`
